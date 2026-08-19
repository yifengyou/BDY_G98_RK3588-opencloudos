// SPDX-License-Identifier: GPL-2.0
#include "yk3.h"
#include "yk3_ptp_priv.h"
#include "yk3_emp_priv.h"

static u32 yk3_ptp_get_rt_sec(struct yk3_ptp *ptp)
{
	return yk3_rd32(ptp->tod_ctrl, YK3_PTP_TOD_RT_M);
}

static u64 yk3_ptp_get_rt_timestamp(struct yk3_ptp *ptp)
{
	u32 s1, s2, ns;

	s1 = yk3_rd32(ptp->tod_ctrl, YK3_PTP_TOD_RT_M);
	ns = yk3_rd32(ptp->tod_ctrl, YK3_PTP_TOD_RT_L);
	s2 = yk3_rd32(ptp->tod_ctrl, YK3_PTP_TOD_RT_M);

	if (s1 != s2 && ns < NSEC_PER_MSEC)
		s1 = s2;

	return s1 * NSEC_PER_SEC + ns;
}

static u64 yk3_ptp_get_refl_ns(struct yk3_ptp *ptp)
{
	u64 time_ns = 0;
	int loop;

	for (loop = 10; loop > 0; loop--) {
		if (!YK3_PTP_REFL_VLD(ptp->tod_ctrl))
			continue;

		time_ns = yk3_rd32(ptp->tod_ctrl, YK3_PTP_TOD_REFL_M) * NSEC_PER_SEC +
			  yk3_rd32(ptp->tod_ctrl, YK3_PTP_TOD_REFL_L);
		break;
	}

	if (loop == 0)
		yk3_warn("ptp get refl timestamp timeout.");

	return time_ns;
}

static int yk3_ptp_config_addend_and_incr(struct yk3_ptp *ptp)
{
	u64 fractional = 0, ss_incr = 0;

	if (ptp->req_freq == 0 || ptp->hw_freq == 0) {
		yk3_err("invalid req freq /hw freq\n");
		return -EINVAL;
	}

	/* calculate addend */
	ptp->default_addend = div_u64((u64)(ptp->req_freq << 32), ptp->hw_freq);

	/* calculate ss_incr */
	fractional = (NSEC_PER_SEC % ptp->req_freq) * BIT(4) / ptp->req_freq;
	ss_incr = ((NSEC_PER_SEC / ptp->req_freq) << 4) | (fractional & 0xf);

	/* config registers */
	yk3_wr32(ptp->tod_ctrl, YK3_PTP_TOD_SS_INCR, ss_incr);
	yk3_wr32(ptp->tod_ctrl, YK3_PTP_TOD_ADDEND, ptp->default_addend);

	return 0;
}

static void yk3_ptp_config_ts(struct yk3_ptp *ptp, u64 time_ns)
{
	u32 val, timm, timl;

	val = YK3_PTP_TOD_CTRL_FIND_CORSE |
	      YK3_PTP_TOD_CTRL_TS_EN |
	      YK3_PTP_TOD_CTRL_ROLLOVER |
	      YK3_PTP_TOD_CTRL_INIT;

	if (!time_ns)
		time_ns = ktime_get_real_ns();

	timm = time_ns / NSEC_PER_SEC;
	timl = time_ns % NSEC_PER_SEC;

	yk3_wr32(ptp->tod_ctrl, YK3_PTP_TOD_CFG_L, timl);
	yk3_wr32(ptp->tod_ctrl, YK3_PTP_TOD_CFG_M, timm);
	yk3_wr32(ptp->tod_ctrl, YK3_PTP_TOD_CTRL, val);
}

static void yk3_ptp_clear_ts(struct yk3_ptp *ptp)
{
	yk3_wr32(ptp->tod_ctrl, YK3_PTP_TOD_CTRL, 0);
}

static int yk3_ptp_phc_adjfreq(struct ptp_clock_info *clock_info, s32 ppb)
{
	u64 addend;
	u32 diff;
	int neg_adj = 0;

	struct yk3_ptp *ptp = container_of(clock_info, struct yk3_ptp, ptp_caps);

	if (ppb == 0)
		return 0;

	if (ppb < 0) {
		neg_adj = 1;
		ppb = -ppb;
	}

	addend = ptp->default_addend;

	diff = div_u64(addend * ppb, NSEC_PER_SEC);

	if (neg_adj) {
		if (diff > addend) {
			yk3_warn("addend underflowed. ppb=0x%x, diff=0x%x, addend=0x%llx\n",
				 ppb, diff, addend);
			return -ERANGE;
		}

		addend -= diff;
	} else {
		addend += diff;
		if (addend > 0xFFFFFFFFULL) {
			yk3_warn("addend overflowed. ppb=0x%x, diff=0x%x, addend=0x%llx\n",
				 ppb, diff, addend);
			addend = 0xFFFFFFFFULL;
		}
	}

	yk3_wr32(ptp->tod_ctrl, YK3_PTP_TOD_ADDEND, addend);

	return 0;
}

static int yk3_ptp_phc_adjtime(struct ptp_clock_info *clock_info, s64 delta)
{
	u64 time_ns;
	u64 abs_delta;

	struct yk3_ptp *ptp =
		container_of(clock_info, struct yk3_ptp, ptp_caps);

	time_ns = yk3_ptp_get_rt_timestamp(ptp);
	if (delta < 0) {
		if (delta == LLONG_MIN)
			return -EINVAL;
		abs_delta = (u64)(-delta);
		if (abs_delta > time_ns)
			return -EINVAL;
		time_ns -= abs_delta;
	} else {
		time_ns += (u64)delta;
	}

	yk3_ptp_config_ts(ptp, time_ns);

	return 0;
}

#ifndef YK3_HAVE_PTP_GETTIMEX
static int yk3_ptp_phc_gettime64(struct ptp_clock_info *clock_info, struct timespec64 *ts)
{
	u64 ts_ns;

	struct yk3_ptp *ptp = container_of(clock_info, struct yk3_ptp, ptp_caps);

	ts_ns = yk3_ptp_get_rt_timestamp(ptp);

	*ts = ns_to_timespec64(ts_ns);

	return 0;
}
#else
static int yk3_ptp_phc_gettimex64(struct ptp_clock_info *clock_info,
				  struct timespec64 *ts,
				  struct ptp_system_timestamp *sts)
{
	struct yk3_ptp *ptp = container_of(clock_info, struct yk3_ptp, ptp_caps);
	u64 ts_ns;

	if (sts)
		ptp_read_system_prets(sts);

	ts_ns = yk3_ptp_get_rt_timestamp(ptp);
	*ts = ns_to_timespec64(ts_ns);

	if (sts)
		ptp_read_system_postts(sts);

	return 0;
}
#endif /* YK3_HAVE_PTP_GETTIMEX */

static int yk3_ptp_phc_getcrosststamp(struct ptp_clock_info *clock_info,
				      struct system_device_crosststamp *cts)
{
	return -EOPNOTSUPP;
}

static int yk3_ptp_phc_settime64(struct ptp_clock_info *clock_info,
				 const struct timespec64 *ts)
{
	u64 time_ns;
	struct yk3_ptp *ptp = container_of(clock_info, struct yk3_ptp, ptp_caps);

	time_ns = timespec64_to_ns(ts);

	yk3_ptp_config_ts(ptp, time_ns);

	return 0;
}

static int yk3_ptp_phc_enable(struct ptp_clock_info *clock_info,
			      struct ptp_clock_request *request, int on)
{
	return -EOPNOTSUPP;
}

static int yk3_ptp_phc_verify(struct ptp_clock_info *clock_info, unsigned int pin,
			      enum ptp_pin_function func, unsigned int chan)
{
	return -EOPNOTSUPP;
}

#ifdef YK3_HAVE_PTP_ADJFINE
static int yk3_ptp_phc_adjfine(struct ptp_clock_info *clock_info, long scaled_ppm)
{
	/*
	 * The 'freq' field in the 'struct timex' is in parts per
	 * million, but with a 16 bit binary fractional field.
	 *
	 * We want to calculate
	 *
	 *    ppb = scaled_ppm * 1000 / 2^16
	 *
	 * which simplifies to
	 *
	 *    ppb = scaled_ppm * 125 / 2^13
	 */
	s64 ppb = scaled_ppm;

	ppb *= 125;
	ppb >>= 13;

	return yk3_ptp_phc_adjfreq(clock_info, (s32)ppb);
}
#endif /* YK3_HAVE_PTP_ADJFINE */

static const struct ptp_clock_info yk3_ptp_clock_info = {
	.owner = THIS_MODULE,
	.max_adj = 100000000,
	.n_alarm = 0,
	.n_ext_ts = 0,
	.n_per_out = 0,
	.n_pins = 0,
	.pps = 0,
#ifdef YK3_HAVE_PTP_ADJFREQ
	.adjfreq = yk3_ptp_phc_adjfreq,
#endif /* YK3_HAVE_PTP_ADJFREQ */
	.adjtime = yk3_ptp_phc_adjtime,
#ifndef YK3_HAVE_PTP_GETTIMEX
	.gettime64 = yk3_ptp_phc_gettime64,
#else
	.gettimex64 = yk3_ptp_phc_gettimex64,
#endif /* YK3_HAVE_PTP_GETTIMEX */
	.settime64 = yk3_ptp_phc_settime64,
	.getcrosststamp = yk3_ptp_phc_getcrosststamp,
	.enable = yk3_ptp_phc_enable,
	.verify = yk3_ptp_phc_verify,
#ifdef YK3_HAVE_PTP_ADJFINE
	.adjfine = yk3_ptp_phc_adjfine,
#endif /* YK3_HAVE_PTP_ADJFINE*/
};

static void yk3_ptp_hw_init(struct yk3_ptp *ptp)
{
	u32 val;
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ptp->pdev);

	ptp->idx = pdev_priv->pf_id;
	ptp->tod_mode = YK3_PTP_TOD_APP;
	ptp->ptp_mode = YK3_PTP_TWOSTEP;

	ptp->glb_ctrl = pdev_priv->bar_addr[0] + YK3_PTP_GLB_REG;
	ptp->ch_ctrl = pdev_priv->bar_addr[0] + YK3_PTP_CH_REG(ptp->idx);
	ptp->tod_ctrl = pdev_priv->bar_addr[0] + YK3_PTP_TOD_BASE(ptp->idx);

	val = YK3_PTP_GLB_APP_MODE;
	yk3_wr32(ptp->glb_ctrl, 0, val);
}

static int yk3_ptp_config_ch(struct yk3_ptp *ptp)
{
	u32 val;

	val = YK3_PTP_CH_FLAG_EN | YK3_PTP_CH_RX_TOD_EN | YK3_PTP_CH_IDX(ptp->idx);
	if (ptp->tod_mode == YK3_PTP_TOD_APP) {
		val |= YK3_PTP_CH_RX_APP_EN;
	} else if (ptp->tod_mode == YK3_PTP_TOD_MAC) {
		val |= YK3_PTP_CH_REFL_EN;
		if (ptp->ptp_mode == YK3_PTP_TWOSTEP)
			val |= YK3_PTP_CH_TS_EN;
		else
			val |= (YK3_PTP_CH_OS_EN | YK3_PTP_CH_CSUM_EN);
	} else {
		yk3_err("unsupported tod mode: %d", ptp->tod_mode);
		return -EINVAL;
	}

	yk3_wr32(ptp->ch_ctrl, 0, val);

	return 0;
}

int yk3_ptp_init(struct yk3_pdev_priv *pdev_priv)
{
	int ret = 0;
	u8 val = 5;
	struct yk3_ptp *ptp;
	u64 hw_bits = pdev_priv->card->hw_bits;
	struct yk3_vpd_info *vpd;

	if (yk3_pdev_is_vf(pdev_priv))
		return 0;

	ptp = kzalloc(sizeof(*ptp), GFP_KERNEL);
	if (!ptp)
		return -ENOMEM;

	ptp->pdev = pdev_priv->pdev;
	ptp->idx = pdev_priv->pf_id;
	ptp->ptp_caps = yk3_ptp_clock_info;
	snprintf(ptp->ptp_caps.name, sizeof(ptp->ptp_caps.name), "yk3_ptp%u",
		 ptp->idx);
	ptp->ptp_clock = ptp_clock_register(&ptp->ptp_caps, &ptp->pdev->dev);
	if (IS_ERR(ptp->ptp_clock)) {
		yk3_err("Register PTP hardware clock error");
		ret = PTR_ERR(ptp->ptp_clock);
		goto err_register_ptp_clock;
	}

	yk3_debug("register ptp clock success. name=%s\n",
		  ptp->ptp_caps.name);
	yk3_ptp_hw_init(ptp);

	vpd = &pdev_priv->card->emp_info->vpd;
	if (vpd->chip_type == 0) { /* ASIC */
		ptp->req_freq = 600000000ULL;
		ptp->hw_freq = 800000000ULL;
	} else {
		if (hw_bits & YK3_HW_BIT_UMAC) {
			ptp->req_freq = 80000000ULL;
			ptp->hw_freq = 82350000ULL;
		} else if (hw_bits & YK3_HW_BIT_XMAC) {
			ptp->req_freq = 150000000;	//150m
			ptp->hw_freq = 160000000ULL;
		} else {
			yk3_err("unsupported mac type for ptp");
			ret = -EINVAL;
			goto err_mac_type;
		}
	}

	yk3_ppp_action_modify(pdev_priv, YK3_PPP_PTP_ACTION, 0, &val);

	pdev_priv->ptp = ptp;

	return 0;

err_mac_type:
	ptp_clock_unregister(ptp->ptp_clock);
err_register_ptp_clock:
	kfree(ptp);

	return ret;
}

void yk3_ptp_exit(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_ptp *ptp = pdev_priv->ptp;

	if (!ptp)
		return;

	if (ptp->ptp_clock) {
		ptp_clock_unregister(ptp->ptp_clock);
		ptp->ptp_clock = NULL;
	}

	yk3_ptp_disable(pdev_priv);

	pdev_priv->ptp = NULL;
	kfree(ptp);
}

int yk3_ptp_enable(struct yk3_pdev_priv *pdev_priv)
{
	int ret = 0;
	struct yk3_ptp *ptp = pdev_priv->ptp;

	if (!ptp)
		return -EOPNOTSUPP;

	if (ptp->tod_mode == YK3_PTP_TOD_APP && ptp->ptp_mode == YK3_PTP_ONESTEP) {
		yk3_err("app mode is not support onestep");
		return -EINVAL;
	}

	/* 1. config channel control registers */
	ret = yk3_ptp_config_ch(ptp);
	if (ret)
		return ret;

	/* 2. config addend and ss_incr */
	ret = yk3_ptp_config_addend_and_incr(ptp);
	if (ret)
		return ret;

	/* 3. config tod control register */
	yk3_ptp_config_ts(ptp, 0);

	return ret;
}

void yk3_ptp_disable(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_ptp *ptp = pdev_priv->ptp;

	if (!ptp)
		return;

	yk3_ptp_clear_ts(ptp);
}

void yk3_ptp_tod_refl_clear(struct yk3_pdev_priv *pdev_priv)
{
	u32 val;
	struct yk3_ptp *ptp = pdev_priv->ptp;

	if (IS_ERR_OR_NULL(ptp) || !ptp->tx_hw_tstamp)
		return;

	val = yk3_rd32(ptp->tod_ctrl, YK3_PTP_TOD_REFL_CTRL);
	val |= YK3_PTP_TOD_REFL_CTRL_CLR;

	yk3_wr32(ptp->tod_ctrl, YK3_PTP_TOD_REFL_CTRL, val);
}

void yk3_ptp_set_tx_ts(struct yk3_pdev_priv *pdev_priv, struct sk_buff *skb)
{
	struct skb_shared_hwtstamps hwts;
	u64 time_ns;
	struct yk3_ptp *ptp = pdev_priv->ptp;

	if (!ptp)
		return;

	memset(&hwts, 0, sizeof(hwts));
	if (ptp->ptp_mode == YK3_PTP_ONESTEP)
		return;

	time_ns = yk3_ptp_get_refl_ns(ptp);
	if (!time_ns)
		return;

	hwts.hwtstamp = ktime_set(time_ns / NSEC_PER_SEC, time_ns % NSEC_PER_SEC);
	skb_tstamp_tx(skb, &hwts);

	yk3_debug("ptp set tx ts. pf:%d relf time ns=0x%llx", ptp->idx, time_ns);
}

void yk3_ptp_set_rx_ts(struct yk3_pdev_priv *pdev_priv, struct sk_buff *skb, u64 rx_ns)
{
	u32 sec = 0, nsec = 0;
	u32 rt_sec, rt_sec_high, rt_sec_low, rx_sec_low;
	struct yk3_ptp *ptp = pdev_priv->ptp;

	if (!ptp || !ptp->rx_hw_tstamp)
		return;

	rt_sec = yk3_ptp_get_rt_sec(ptp);
	rt_sec_high = rt_sec & 0xFFFF0000;
	rt_sec_low = rt_sec & 0x0000FFFF;
	rx_sec_low = (rx_ns >> 32) & 0x0000FFFF;

	/* Fix rollover mismatch between sampled RT seconds and RX timestamp low16 */
	if (rt_sec_low < rx_sec_low) {
		if ((rx_sec_low - rt_sec_low) > 0x8000)
			rt_sec_high -= 0x00010000;
	}

	sec = rt_sec_high + rx_sec_low;
	nsec = rx_ns & 0xFFFFFFFF;

	if (!sec && !nsec) {
		yk3_err("error rx timestamp. time=%llx\n", rx_ns);
		return;
	}

	skb_hwtstamps(skb)->hwtstamp = ktime_set(sec, nsec);
	yk3_debug("ptp set rx ts. pf:%d sysmeta ts=0x%llx, rt sec=0x%08x!!!\n",
		  ptp->idx, rx_ns, sec);
}

int yk3_ptp_hwtstamp_set(struct yk3_pdev_priv *pdev_priv, struct hwtstamp_config *hwts_config)
{
	struct yk3_ptp *ptp = pdev_priv->ptp;

	if (!ptp)
		return -EOPNOTSUPP;

	switch (hwts_config->tx_type) {
	case HWTSTAMP_TX_OFF:
		ptp->tx_hw_tstamp = false;
		break;
	case HWTSTAMP_TX_ON:
		ptp->tx_hw_tstamp = true;
		break;
	case HWTSTAMP_TX_ONESTEP_SYNC:
	default:
		return -ERANGE;
	}

	switch (hwts_config->rx_filter) {
	case HWTSTAMP_FILTER_NONE:
		ptp->rx_hw_tstamp = false;
		break;
	case HWTSTAMP_FILTER_ALL:
	case HWTSTAMP_FILTER_SOME:
	case HWTSTAMP_FILTER_PTP_V1_L4_EVENT:
	case HWTSTAMP_FILTER_PTP_V1_L4_SYNC:
	case HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ:
	case HWTSTAMP_FILTER_PTP_V2_L4_EVENT:
	case HWTSTAMP_FILTER_PTP_V2_L4_SYNC:
	case HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ:
	case HWTSTAMP_FILTER_PTP_V2_L2_EVENT:
	case HWTSTAMP_FILTER_PTP_V2_L2_SYNC:
	case HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ:
	case HWTSTAMP_FILTER_PTP_V2_EVENT:
	case HWTSTAMP_FILTER_PTP_V2_SYNC:
	case HWTSTAMP_FILTER_PTP_V2_DELAY_REQ:
	case HWTSTAMP_FILTER_NTP_ALL:
		hwts_config->rx_filter = HWTSTAMP_FILTER_ALL;
		ptp->rx_hw_tstamp = true;
		break;
	default:
		return -ERANGE;
	}

	memcpy(&ptp->hwts_config, hwts_config, sizeof(*hwts_config));

	if (!ptp->tx_hw_tstamp && !ptp->rx_hw_tstamp)
		yk3_ptp_disable(pdev_priv);
	else
		yk3_ptp_enable(pdev_priv);

	return 0;
}

int yk3_ptp_hwtstamp_get(struct yk3_pdev_priv *pdev_priv, struct hwtstamp_config *hwts_config)
{
	struct yk3_ptp *ptp = pdev_priv->ptp;

	if (!ptp)
		return -EOPNOTSUPP;

	memcpy(hwts_config, &ptp->hwts_config, sizeof(*hwts_config));

	return 0;
}

#ifdef YK3_HAVE_KERNEL_ETHTOOL_TS_INFO
int yk3_ptp_get_ts_info(struct yk3_pdev_priv *pdev_priv, struct kernel_ethtool_ts_info *eti)
#else
int yk3_ptp_get_ts_info(struct yk3_pdev_priv *pdev_priv, struct ethtool_ts_info *eti)
#endif
{
	struct yk3_ptp *ptp = pdev_priv->ptp;

	if (!ptp)
		return -EOPNOTSUPP;

	if (!IS_ERR_OR_NULL(ptp->ptp_clock)) {
		eti->so_timestamping = SOF_TIMESTAMPING_TX_HARDWARE |
				       SOF_TIMESTAMPING_RX_HARDWARE |
				       SOF_TIMESTAMPING_RAW_HARDWARE;
		eti->phc_index = ptp_clock_index(ptp->ptp_clock);
		eti->tx_types = (1 << HWTSTAMP_TX_OFF) | (1 << HWTSTAMP_TX_ON);
		eti->rx_filters = (1 << HWTSTAMP_FILTER_NONE) |
				  (1 << HWTSTAMP_FILTER_ALL);
	} else {
		eti->so_timestamping = SOF_TIMESTAMPING_RX_SOFTWARE |
				       SOF_TIMESTAMPING_TX_SOFTWARE |
				       SOF_TIMESTAMPING_SOFTWARE;
		eti->phc_index = -1;
		eti->tx_types = 0;
		eti->rx_filters = 0;
	}

	return 0;
}
