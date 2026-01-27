// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024, DapuStor Corporation.
 */
#include "dn200.h"
#include "dn200_ptp.h"

/**
 * dn200_adjust_freq
 *
 * @ptp: pointer to ptp_clock_info structure
 * @scaled_ppm: desired period change in scaled parts per million
 *
 * Description: this function will adjust the frequency of hardware clock.
 *
 * Scaled parts per million is ppm with a 16-bit binary fractional field.
 */
static int dn200_adjust_freq(struct ptp_clock_info *ptp, long scaled_ppm)
{
	struct dn200_priv *priv =
	    container_of(ptp, struct dn200_priv, ptp_clock_ops);
	unsigned long flags;
	u32 addend;

	addend = adjust_by_scaled_ppm(priv->default_addend, scaled_ppm);

	spin_lock_irqsave(&priv->ptp_lock, flags);
	dn200_config_addend(priv, priv->ptpaddr, addend);
	spin_unlock_irqrestore(&priv->ptp_lock, flags);

	return 0;
}

static struct timespec64 dn200_calc_tas_basetime(ktime_t old_base_time,
					  ktime_t current_time, u64 cycle_time)
{
	struct timespec64 time;

	if (ktime_after(old_base_time, current_time)) {
		time = ktime_to_timespec64(old_base_time);
	} else {
		s64 n;
		ktime_t base_time;
		ktime_t timetest;

		timetest = ktime_sub_ns(current_time, old_base_time);
		n = div64_s64(timetest, cycle_time);
		base_time = ktime_add_ns(old_base_time, (n + 1) * cycle_time);

		time = ktime_to_timespec64(base_time);
	}

	return time;
}

/**
 * dn200_adjust_time
 *
 * @ptp: pointer to ptp_clock_info structure
 * @delta: desired change in nanoseconds
 *
 * Description: this function will shift/adjust the hardware clock time.
 */
static int dn200_adjust_time(struct ptp_clock_info *ptp, s64 delta)
{
	struct dn200_priv *priv =
	    container_of(ptp, struct dn200_priv, ptp_clock_ops);
	unsigned long flags;
	u32 sec, nsec;
	u32 quotient, reminder;
	int neg_adj = 0;
	bool xmac, est_rst = false;
	int ret;

	xmac = priv->plat->has_gmac4 || priv->plat->has_xgmac;

	if (delta < 0) {
		neg_adj = 1;
		delta = -delta;
	}

	quotient = div_u64_rem(delta, 1000000000ULL, &reminder);
	sec = quotient;
	nsec = reminder;

	/* If EST is enabled, disabled it before adjust ptp time. */
	if (priv->plat->est && priv->plat->est->enable) {
		est_rst = true;
		mutex_lock(&priv->plat->est->lock);
		priv->plat->est->enable = false;
		dn200_est_configure(priv, priv->ioaddr, priv->plat->est,
				    priv->plat->clk_ptp_rate);
		mutex_unlock(&priv->plat->est->lock);
	}

	spin_lock_irqsave(&priv->ptp_lock, flags);
	dn200_adjust_systime(priv, priv->ptpaddr, sec, nsec, neg_adj, xmac);
	spin_unlock_irqrestore(&priv->ptp_lock, flags);

	/* Caculate new basetime and re-configured EST after PTP time adjust. */
	if (est_rst) {
		struct timespec64 current_time, time;
		ktime_t current_time_ns, basetime;
		u64 cycle_time;

		mutex_lock(&priv->plat->est->lock);
		priv->ptp_clock_ops.gettime64(&priv->ptp_clock_ops,
					      &current_time);
		current_time_ns = timespec64_to_ktime(current_time);
		time.tv_nsec = priv->plat->est->btr_reserve[0];
		time.tv_sec = priv->plat->est->btr_reserve[1];
		basetime = timespec64_to_ktime(time);
		cycle_time = (u64) priv->plat->est->ctr[1] * NSEC_PER_SEC +
		    priv->plat->est->ctr[0];
		time = dn200_calc_tas_basetime(basetime,
					       current_time_ns, cycle_time);

		priv->plat->est->btr[0] = (u32) time.tv_nsec;
		priv->plat->est->btr[1] = (u32) time.tv_sec;
		priv->plat->est->enable = true;
		ret = dn200_est_configure(priv, priv->ioaddr, priv->plat->est,
					  priv->plat->clk_ptp_rate);
		mutex_unlock(&priv->plat->est->lock);
		if (ret)
			netdev_err(priv->dev, "failed to configure EST\n");
	}

	return 0;
}

/**
 * dn200_get_time
 *
 * @ptp: pointer to ptp_clock_info structure
 * @ts: pointer to hold time/result
 *
 * Description: this function will read the current time from the
 * hardware clock and store it in @ts.
 */
static int dn200_get_time(struct ptp_clock_info *ptp, struct timespec64 *ts)
{
	struct dn200_priv *priv =
	    container_of(ptp, struct dn200_priv, ptp_clock_ops);
	unsigned long flags;
	u64 ns = 0;

	spin_lock_irqsave(&priv->ptp_lock, flags);
	dn200_get_systime(priv, priv->ptpaddr, &ns);
	spin_unlock_irqrestore(&priv->ptp_lock, flags);

	*ts = ns_to_timespec64(ns);

	return 0;
}

/**
 * dn200_set_time
 *
 * @ptp: pointer to ptp_clock_info structure
 * @ts: time value to set
 *
 * Description: this function will set the current time on the
 * hardware clock.
 */
static int dn200_set_time(struct ptp_clock_info *ptp,
			  const struct timespec64 *ts)
{
	struct dn200_priv *priv =
	    container_of(ptp, struct dn200_priv, ptp_clock_ops);
	unsigned long flags;

	spin_lock_irqsave(&priv->ptp_lock, flags);
	dn200_init_systime(priv, priv->ptpaddr, ts->tv_sec, ts->tv_nsec);
	spin_unlock_irqrestore(&priv->ptp_lock, flags);

	return 0;
}

static int dn200_enable(struct ptp_clock_info *ptp,
			struct ptp_clock_request *rq, int on)
{
	struct dn200_priv *priv =
	    container_of(ptp, struct dn200_priv, ptp_clock_ops);
	void __iomem *ptpaddr = priv->ptpaddr;
	void __iomem *ioaddr = priv->hw->pcsr;
	struct dn200_pps_cfg *cfg;
	u32 intr_value, acr_value;
	int ret = -EOPNOTSUPP;
	unsigned long flags;

	switch (rq->type) {
	case PTP_CLK_REQ_PEROUT:
		/* Reject requests with unsupported flags */
		if (rq->perout.flags)
			return -EOPNOTSUPP;

		cfg = &priv->pps[rq->perout.index];

		cfg->start.tv_sec = rq->perout.start.sec;
		cfg->start.tv_nsec = rq->perout.start.nsec;
		cfg->period.tv_sec = rq->perout.period.sec;
		cfg->period.tv_nsec = rq->perout.period.nsec;

		spin_lock_irqsave(&priv->ptp_lock, flags);
		ret = dn200_flex_pps_config(priv, priv->ioaddr,
					    rq->perout.index, cfg, on,
					    priv->sub_second_inc,
					    priv->systime_flags);
		spin_unlock_irqrestore(&priv->ptp_lock, flags);
		break;
	case PTP_CLK_REQ_EXTTS:
		priv->plat->ext_snapshot_en = on;
		mutex_lock(&priv->aux_ts_lock);
		acr_value = readl(ptpaddr + PTP_ACR);
		acr_value &= ~PTP_ACR_MASK;
		if (on) {
			/* Enable External snapshot trigger */
			acr_value |= priv->plat->ext_snapshot_num;
			acr_value |= PTP_ACR_ATSFC;
			netdev_dbg(priv->dev,
				   "Auxiliary Snapshot %d enabled.\n",
				   priv->plat->ext_snapshot_num >> PTP_ACR_ATSEN_SHIFT);
			/* Enable Timestamp Interrupt */
			intr_value = readl(ioaddr + XGMAC_INT_EN);
			intr_value |= XGMAC_TSIE;
			writel(intr_value, ioaddr + XGMAC_INT_EN);

		} else {
			netdev_dbg(priv->dev,
				   "Auxiliary Snapshot %d disabled.\n",
				   priv->plat->ext_snapshot_num >> PTP_ACR_ATSEN_SHIFT);
			/* Disable Timestamp Interrupt */
			intr_value = readl(ioaddr + XGMAC_INT_EN);
			intr_value &= ~XGMAC_TSIE;
			writel(intr_value, ioaddr + XGMAC_INT_EN);
		}
		writel(acr_value, ptpaddr + PTP_ACR);
		mutex_unlock(&priv->aux_ts_lock);
		ret = 0;
		break;

	default:
		break;
	}

	return ret;
}

/**
 * dn200_get_syncdevicetime
 * @device: current device time
 * @system: system counter value read synchronously with device time
 * @ctx: context provided by timekeeping code
 * Description: Read device and system clock simultaneously and return the
 * corrected clock values in ns.
 **/
static int dn200_get_syncdevicetime(ktime_t *device,
				    struct system_counterval_t *system,
				    void *ctx)
{
	return -EOPNOTSUPP;
}

static int dn200_getcrosststamp(struct ptp_clock_info *ptp,
				struct system_device_crosststamp *xtstamp)
{
	struct dn200_priv *priv =
	    container_of(ptp, struct dn200_priv, ptp_clock_ops);

	return get_device_system_crosststamp(dn200_get_syncdevicetime,
					     priv, NULL, xtstamp);
}

/* structure describing a PTP hardware clock */
static struct ptp_clock_info dn200_ptp_clock_ops = {
	.owner = THIS_MODULE,
	.name = "dn200 ptp",
	.max_adj = 62500000,
	.n_alarm = 0,
	.n_ext_ts = 0,		/* will be overwritten in dn200_ptp_register */
	.n_per_out = 0,		/* will be overwritten in dn200_ptp_register */
	.n_pins = 0,
	.pps = 0,
	.adjfine = dn200_adjust_freq,
	.adjtime = dn200_adjust_time,
	.gettime64 = dn200_get_time,
	.settime64 = dn200_set_time,
	.enable = dn200_enable,
	.getcrosststamp = dn200_getcrosststamp,
};

/**
 * dn200_ptp_register
 * @priv: driver private structure
 * Description: this function will register the ptp clock driver
 * to kernel. It also does some house keeping work.
 */
void dn200_ptp_register(struct dn200_priv *priv)
{
	int i;

	for (i = 0; i < priv->dma_cap.pps_out_num; i++) {
		if (i >= DN200_PPS_MAX)
			break;
		priv->pps[i].available = true;
	}

	if (priv->plat->ptp_max_adj)
		dn200_ptp_clock_ops.max_adj = priv->plat->ptp_max_adj;

	dn200_ptp_clock_ops.n_per_out = priv->dma_cap.pps_out_num;
	dn200_ptp_clock_ops.n_ext_ts = priv->dma_cap.aux_snapshot_n;

	spin_lock_init(&priv->ptp_lock);
	mutex_init(&priv->aux_ts_lock);
	priv->ptp_clock_ops = dn200_ptp_clock_ops;

	priv->ptp_clock = ptp_clock_register(&priv->ptp_clock_ops,
					     priv->device);
	if (IS_ERR(priv->ptp_clock)) {
		netdev_err(priv->dev, "register PTP clock failed\n");
		priv->ptp_clock = NULL;
	}
}

/**
 * dn200_ptp_unregister
 * @priv: driver private structure
 * Description: this function will remove/unregister the ptp clock driver
 * from the kernel.
 */
void dn200_ptp_unregister(struct dn200_priv *priv)
{
	if (priv->ptp_clock) {
		ptp_clock_unregister(priv->ptp_clock);
		priv->ptp_clock = NULL;
		pr_debug("Removed PTP HW clock successfully on %s\n",
			 priv->dev->name);
	}

	mutex_destroy(&priv->aux_ts_lock);
}
