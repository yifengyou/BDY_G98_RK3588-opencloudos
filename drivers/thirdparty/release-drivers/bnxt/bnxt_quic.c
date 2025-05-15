// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Broadcom NetXtreme-C/E network driver.
 *
 * Copyright (c) 2024 Broadcom Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#ifdef HAVE_KTLS
#include <net/tls.h>
#endif
#include "bnxt_compat.h"
#include "bnxt.h"
#include "bnxt_mpc.h"
#include "bnxt_ktls.h"
#include "bnxt_quic.h"

#ifdef HAVE_BNXT_QUIC
#include "json/kjson.h"

bnxt_quic_crypto_info quic_flow;

struct sk_buff *bnxt_quic_xmit(struct bnxt *bp, struct bnxt_tx_ring_info *txr,
			       struct sk_buff *skb, __le32 *lflags, u32 *kid)
{
	struct bnxt_tls_info *quic = bp->quic_info;

	/* Check if flow is programmed. */
	if (quic && quic_flow.sport) {
		*kid = quic_flow.tx_kid;
		*lflags |= cpu_to_le32(TX_BD_FLAGS_CRYPTO_EN |
				       BNXT_TX_KID_LO(*kid));
		atomic64_inc(&quic->counters[BNXT_QUIC_TX_HW_PKT]);
	}
	return skb;
}

void bnxt_get_quic_dst_conect_id(struct bnxt *bp,
				 struct hwrm_cfa_tls_filter_alloc_input *req)
{
	req->quic_dst_connect_id = cpu_to_le64(quic_flow.rx_conn_id);
}

static u16 get_cipher_from_str(char *cipher_str)
{
	u16 cipher;

	if (strcasecmp(cipher_str, "aes_gcm_128") == 0)
		cipher = TLS_CIPHER_AES_GCM_128;
	else if (strcasecmp(cipher_str, "aes_gcm_256") == 0)
		cipher = TLS_CIPHER_AES_GCM_256;
	else if (strcasecmp(cipher_str, "chacha20_poly1305") == 0)
		cipher = TLS_CIPHER_CHACHA20_POLY1305;
	else
		cipher = TLS_CIPHER_CHACHA20_POLY1305;
	return cipher;
}

static void rev_cpy(u8 *dst, u8 *src, int n)
{
	int i;

	for (i = 0; i < n; ++i)
		dst[n - 1 - i] = src[i];
}

static int bnxt_quic_info_parse(const char *buf)
{
	struct kjson_object_t *obj1, *obj2, *obj3, *tx_data_key, *tx_header_key, *tx_iv,
			     *rx_data_key, *rx_header_key, *rx_iv;
	uint8_t tmp[BNXT_MAX_KEY_SIZE * 2];
	struct kjson_container *my_json;
	int ret;

	my_json = kjson_parse(buf);
	if (!my_json) {
		pr_err("bnxt_quic: parse error\n");
		return -1;
	}

	obj1 = kjson_lookup_object(my_json, "cipher");
	obj2 = kjson_lookup_object(my_json, "tx_conn_id");
	obj3 = kjson_lookup_object(my_json, "rx_conn_id");
	tx_data_key = kjson_lookup_object(my_json, "tx_data_key");
	tx_header_key = kjson_lookup_object(my_json, "tx_header_key");
	tx_iv = kjson_lookup_object(my_json, "tx_iv");
	rx_data_key = kjson_lookup_object(my_json, "rx_data_key");
	rx_header_key = kjson_lookup_object(my_json, "rx_header_key");
	rx_iv = kjson_lookup_object(my_json, "rx_iv");

	quic_flow.cipher = get_cipher_from_str(kjson_as_string(obj1));

	quic_flow.rx_conn_id = kjson_as_integer(kjson_lookup_object(my_json, "rx_conn_id"));

	ret = hex2bin(quic_flow.tx_data_key, kjson_as_string(tx_data_key), BNXT_MAX_KEY_SIZE * 2);
	memcpy(tmp, quic_flow.tx_data_key, sizeof(quic_flow.tx_data_key));
	rev_cpy(quic_flow.tx_data_key, tmp, sizeof(quic_flow.tx_data_key));

	ret = hex2bin(quic_flow.tx_hdr_key, kjson_as_string(tx_header_key), BNXT_MAX_KEY_SIZE * 2);
	memcpy(tmp, quic_flow.tx_hdr_key, sizeof(quic_flow.tx_hdr_key));
	rev_cpy(quic_flow.tx_hdr_key, tmp, sizeof(quic_flow.tx_hdr_key));

	ret = hex2bin(quic_flow.tx_iv, kjson_as_string(tx_iv), BNXT_IV_SIZE * 2);
	memcpy(tmp, quic_flow.tx_iv, sizeof(quic_flow.tx_iv));
	rev_cpy(quic_flow.tx_iv, tmp, sizeof(quic_flow.tx_iv));

	ret = hex2bin(quic_flow.rx_data_key, kjson_as_string(rx_data_key), BNXT_MAX_KEY_SIZE * 2);
	memcpy(tmp, quic_flow.rx_data_key, sizeof(quic_flow.rx_data_key));
	rev_cpy(quic_flow.rx_data_key, tmp, sizeof(quic_flow.rx_data_key));

	ret = hex2bin(quic_flow.rx_hdr_key, kjson_as_string(rx_header_key), BNXT_MAX_KEY_SIZE * 2);
	memcpy(tmp, quic_flow.rx_hdr_key, sizeof(quic_flow.rx_hdr_key));
	rev_cpy(quic_flow.rx_hdr_key, tmp, sizeof(quic_flow.rx_hdr_key));

	ret = hex2bin(quic_flow.rx_iv, kjson_as_string(rx_iv), BNXT_IV_SIZE * 2);
	memcpy(tmp, quic_flow.rx_iv, sizeof(quic_flow.rx_iv));
	rev_cpy(quic_flow.rx_iv, tmp, sizeof(quic_flow.rx_iv));

	quic_flow.daddr = kjson_as_integer(kjson_lookup_object(my_json, "dst_ipaddr"));
	quic_flow.sport = kjson_as_integer(kjson_lookup_object(my_json, "dst_port"));
	quic_flow.saddr = kjson_as_integer(kjson_lookup_object(my_json, "sin_addr"));
	quic_flow.dport = kjson_as_integer(kjson_lookup_object(my_json, "sin_port"));
	quic_flow.pkt_number = kjson_as_integer(kjson_lookup_object(my_json, "tx_pkt_num"));
	quic_flow.dst_conn_id_width = sizeof(kjson_as_string(obj3));

	kjson_delete_container(my_json);
	return 0;
}

static int bnxt_quic_crypto_tx_add(struct bnxt *bp, u32 kid)
{
	struct bnxt_mpc_info *mpc = bp->mpc_info;
	struct bnxt_tx_ring_info *txr;
	struct quic_ce_add_cmd cmd = {0};
	u32 data;

	if (!mpc)
		return 0;

	txr = &mpc->mpc_rings[BNXT_MPC_TCE_TYPE][0];

	data = CE_ADD_CMD_OPCODE_ADD | (kid << CE_ADD_CMD_KID_SFT) | CE_ADD_CMD_VERSION_QUIC;
	switch (quic_flow.cipher) {
	case TLS_CIPHER_AES_GCM_128: {
		data |= CE_ADD_CMD_ALGORITHM_AES_GCM_128;
		if (quic_flow.version == TLS_1_3_VERSION)
			data |= CE_ADD_CMD_VERSION_TLS1_3;
		memcpy(&cmd.session_key, &quic_flow.tx_data_key, sizeof(quic_flow.tx_data_key));
		memcpy(&cmd.hp_key, quic_flow.tx_hdr_key, sizeof(quic_flow.tx_hdr_key));
		memcpy(&cmd.iv, quic_flow.tx_iv, sizeof(cmd.iv));
		break;
	}
	case TLS_CIPHER_AES_GCM_256: {
		data |= CE_ADD_CMD_ALGORITHM_AES_GCM_256;
		if (quic_flow.version == TLS_1_3_VERSION)
			data |= CE_ADD_CMD_VERSION_TLS1_3;
		memcpy(&cmd.session_key, quic_flow.tx_data_key, sizeof(quic_flow.tx_data_key));
		memcpy(&cmd.hp_key, quic_flow.tx_hdr_key, sizeof(quic_flow.tx_hdr_key));
		memcpy(&cmd.iv, quic_flow.tx_iv, sizeof(cmd.iv));
		break;
	}
	}

	cmd.ver_algo_kid_opcode = cpu_to_le32(data);
	cmd.pkt_number = cpu_to_le32(quic_flow.pkt_number);
	data = QUIC_CE_ADD_CMD_CTX_KIND_CK_TX << QUIC_CE_ADD_CMD_CTX_KIND_SFT;
	data |= quic_flow.dst_conn_id_width << QUIC_CE_ADD_CMD_DST_CID_SFT;
	cmd.ctx_kind_dst_cid_width_key_phase = cpu_to_le32(data);
	return bnxt_xmit_crypto_cmd(bp, txr, &cmd, sizeof(cmd),
				    BNXT_MPC_TMO_MSECS, bp->quic_info);
}

static int bnxt_quic_crypto_rx_add(struct bnxt *bp,
				   u32 kid)
{
	struct bnxt_mpc_info *mpc = bp->mpc_info;
	struct quic_ce_add_cmd cmd = {0};
	struct bnxt_tx_ring_info *txr;
	u32 data = 0, data1 = 0;

	if (!mpc)
		return 0;

	txr = &mpc->mpc_rings[BNXT_MPC_RCE_TYPE][0];

	data = CE_ADD_CMD_OPCODE_ADD | (kid << CE_ADD_CMD_KID_SFT) | CE_ADD_CMD_VERSION_QUIC;

	data1 = QUIC_CE_ADD_CMD_CTX_KIND_CK_RX << QUIC_CE_ADD_CMD_CTX_KIND_SFT;
	data1 |= quic_flow.dst_conn_id_width << QUIC_CE_ADD_CMD_DST_CID_SFT;

	switch (quic_flow.cipher) {
	case TLS_CIPHER_AES_GCM_128: {
		data |= CE_ADD_CMD_ALGORITHM_AES_GCM_128;
		if (quic_flow.version == TLS_1_3_VERSION)
			data |= CE_ADD_CMD_VERSION_TLS1_3;
		break;
	}
	case TLS_CIPHER_AES_GCM_256: {
		data |= CE_ADD_CMD_ALGORITHM_AES_GCM_256;
		if (quic_flow.version == TLS_1_3_VERSION)
			data |= CE_ADD_CMD_VERSION_TLS1_3;
		break;
	}
	default:
		return -EINVAL;
	}
	cmd.ver_algo_kid_opcode = cpu_to_le32(data);
	cmd.ctx_kind_dst_cid_width_key_phase = cpu_to_le32(data1);
	memcpy(&cmd.session_key, quic_flow.rx_data_key, sizeof(cmd.session_key));
	memcpy(&cmd.hp_key, quic_flow.rx_hdr_key, sizeof(quic_flow.rx_hdr_key));
	memcpy(&cmd.iv, quic_flow.rx_iv, sizeof(cmd.iv));

	cmd.pkt_number = cpu_to_le32(quic_flow.pkt_number);

	return bnxt_xmit_crypto_cmd(bp, txr, &cmd, sizeof(cmd),
				    BNXT_MPC_TMO_MSECS, bp->quic_info);
}

static int bnxt_quic_crypto_add(struct bnxt *bp, enum tls_offload_ctx_dir dir,
				u32 kid)
{
	if (dir == TLS_OFFLOAD_CTX_DIR_RX)
		return bnxt_quic_crypto_rx_add(bp, kid);
	else
		return bnxt_quic_crypto_tx_add(bp, kid);
}

static void bnxt_quic_fill_sk(struct sock *sk)
{
	struct inet_sock *inet = inet_sk(sk);

	sk->sk_protocol = IPPROTO_UDP;
	sk->sk_family = AF_INET;	/* Only AF_INET. No AF_INET6 yet! */
	inet->inet_dport = htons(quic_flow.dport);
	inet->inet_sport = htons(quic_flow.sport);
	inet->inet_daddr = htonl(quic_flow.daddr);
	inet->inet_saddr = htonl(quic_flow.saddr);
}

int bnxt_quic_crypto_del(struct bnxt *bp,
			 enum tls_offload_ctx_dir direction, u32 kid)
{
	struct bnxt_mpc_info *mpc = bp->mpc_info;
	struct quic_ce_delete_cmd cmd = {0};
	struct bnxt_tx_ring_info *txr;
	u32 data;

	if (direction == TLS_OFFLOAD_CTX_DIR_RX) {
		txr = &mpc->mpc_rings[BNXT_MPC_RCE_TYPE][0];
		data = CE_DELETE_CMD_CTX_KIND_QUIC_RX;
	} else {
		txr = &mpc->mpc_rings[BNXT_MPC_TCE_TYPE][0];
		data = CE_DELETE_CMD_CTX_KIND_QUIC_TX;
	}

	data |= CE_DELETE_CMD_OPCODE_DEL | (kid << CE_DELETE_CMD_KID_SFT);

	cmd.ctx_kind_kid_opcode = cpu_to_le32(data);
	return bnxt_xmit_crypto_cmd(bp, txr, &cmd, sizeof(cmd),
				    BNXT_MPC_TMO_MSECS, bp->quic_info);
}

static int _bnxt_quic_dev_add(struct bnxt *bp, struct sock *sk,
			      enum tls_offload_ctx_dir direction)
{
	struct bnxt_tls_info *quic;
	struct bnxt_kctx *kctx;
	u32 kid;
	int rc;

	if (!bp->quic_info)
		return -EINVAL;

	quic = bp->quic_info;
	atomic_inc(&quic->pending);
	/* Make sure bnxt_close_nic() sees pending before we check the
	 * BNXT_STATE_OPEN flag.
	 */
	smp_mb__after_atomic();
	if (!test_bit(BNXT_STATE_OPEN, &bp->state)) {
		atomic_dec(&quic->pending);
		return -ENODEV;
	}

	if (direction == TLS_OFFLOAD_CTX_DIR_RX)
		kctx = &quic->rck;
	else
		kctx = &quic->tck;

	rc = bnxt_key_ctx_alloc_one(bp, kctx, &kid, BNXT_CRYPTO_TYPE_QUIC);
	if (rc)
		goto exit;

	rc = bnxt_quic_crypto_add(bp, direction, kid);
	if (rc)
		goto bnxt_quic_dev_add_err;

	if (direction == TLS_OFFLOAD_CTX_DIR_RX) {
		quic_flow.rx_kid = kid;
		rc = bnxt_hwrm_cfa_tls_filter_alloc(bp, sk, kid, BNXT_CRYPTO_TYPE_QUIC);
		if (!rc)
			atomic64_inc(&quic->counters[BNXT_QUIC_RX_ADD]);
		else
			bnxt_quic_crypto_del(bp, direction, kid);
	} else {
		quic_flow.tx_kid = kid;
		atomic64_inc(&quic->counters[BNXT_QUIC_TX_ADD]);
	}

bnxt_quic_dev_add_err:
	if (rc)
		bnxt_free_one_kctx(kctx, kid);
exit:
	atomic_dec(&quic->pending);
	return rc;
}

static ssize_t add_show(struct config_item *item, char *buf)
{
	int count = 0;

	count += sprintf(buf, "cipher: %d\n", quic_flow.cipher);
	buf += count;
	count += sprintf(buf, " tx_conn_id: 0x%llx\n", quic_flow.tx_conn_id);
	buf += count;
	count += sprintf(buf, " rx_conn_id: 0x%llx\n", quic_flow.rx_conn_id);
	buf += count;
	count += sprintf(buf, " tx_data_key: %s\n", quic_flow.tx_data_key);
	buf += count;
	count += sprintf(buf, " tx_header_key: %s\n", quic_flow.tx_hdr_key);
	buf += count;
	count += sprintf(buf, " tx_iv: %s\n", quic_flow.tx_iv);
	buf += count;
	count += sprintf(buf, " rx_data_key: %s\n", quic_flow.rx_data_key);
	buf += count;
	count += sprintf(buf, " rx_header_key: %s\n", quic_flow.rx_hdr_key);
	buf += count;
	count += sprintf(buf, " rx_iv: %s\n", quic_flow.rx_iv);
	return count;
}

static void bnxt_quic_dev_add(void)
{
	struct sock *sk;
	int rc;

	sk = kmalloc(sizeof(*sk), GFP_KERNEL);
	if (!sk) {
		netdev_err(quic_flow.bp->dev, "%s failed to alloc\n", __func__);
		return;
	}

	bnxt_quic_fill_sk(sk);
	_bnxt_quic_dev_add(quic_flow.bp, sk, TLS_OFFLOAD_CTX_DIR_RX);
	if (rc)
		netdev_err(quic_flow.bp->dev, "%s failed\n", __func__);
	rc = _bnxt_quic_dev_add(quic_flow.bp, sk, TLS_OFFLOAD_CTX_DIR_TX);
	if (rc)
		netdev_err(quic_flow.bp->dev, "%s Tx failed\n", __func__);

	kfree(sk);
	return;
}

#define QUIC_RETRY_MAX	20
static int _bnxt_quic_dev_del(struct bnxt *bp, enum tls_offload_ctx_dir dir)
{
	struct bnxt_tls_info *quic;
	struct bnxt_kctx *kctx;
	int retry_cnt = 0;
	u32 kid;
	int rc;

	if (!bp->quic_info)
		return -EINVAL;

	quic = bp->quic_info;
	if (dir == TLS_OFFLOAD_CTX_DIR_RX) {
		kctx = &quic->rck;
		kid = quic_flow.rx_kid;
	} else {
		kctx = &quic->tck;
		kid = quic_flow.tx_kid;
	}

retry:
	atomic_inc(&quic->pending);
	/* Make sure bnxt_close_nic() sees pending before we check the
	 * BNXT_STATE_OPEN flag.
	 */
	smp_mb__after_atomic();
	while (!test_bit(BNXT_STATE_OPEN, &bp->state)) {
		atomic_dec(&quic->pending);
		if (!netif_running(bp->dev))
			return 0;
		if (retry_cnt > QUIC_RETRY_MAX) {
			netdev_warn(bp->dev, "%s retry max %d exceeded, state %lx\n",
				    __func__, retry_cnt, bp->state);
			return 0;
		}
		retry_cnt++;
		msleep(100);
		goto retry;
	}

	rc = bnxt_quic_crypto_del(bp, dir, kid);
	if (dir == TLS_OFFLOAD_CTX_DIR_RX)
		atomic64_inc(&quic->counters[BNXT_QUIC_RX_DEL]);
	else
		atomic64_inc(&quic->counters[BNXT_QUIC_TX_DEL]);

	atomic_dec(&quic->pending);

	bnxt_free_one_kctx(kctx, kid);
	return rc;
}

static void bnxt_quic_dev_del(void)
{
	struct bnxt *bp = quic_flow.bp;

	if (!bp->quic_info)
		return;

	_bnxt_quic_dev_del(bp, TLS_OFFLOAD_CTX_DIR_TX);
	_bnxt_quic_dev_del(bp, TLS_OFFLOAD_CTX_DIR_RX);
	bnxt_hwrm_cfa_tls_filter_free(bp, quic_flow.rx_kid, BNXT_CRYPTO_TYPE_QUIC);

	return;
}

static ssize_t add_store(struct config_item *item, const char *buf, size_t count)
{
	bnxt_quic_info_parse(buf);
	bnxt_quic_dev_add();
	return strnlen(buf, count);
}

CONFIGFS_ATTR(, add);

static struct configfs_attribute *bnxt_en_quic_group_attr[] = {
	CONFIGFS_ATTR_ADD(attr_add),
	NULL,
};

static void bnxt_en_release_device_group(struct config_item *item)
{
	struct config_group *group = container_of(item, struct config_group, cg_item);
	struct bnxt_en_dev_group *devgrp = container_of(group, struct bnxt_en_dev_group,
							dev_group);

	bnxt_quic_dev_del();
	kfree(devgrp);
}

static struct config_item_type bnxt_en_quic_group_type = {
	.ct_attrs = bnxt_en_quic_group_attr,
	.ct_owner = THIS_MODULE,
};

static struct configfs_item_operations bnxt_en_dev_item_ops = {
	.release = bnxt_en_release_device_group
};

static struct config_item_type bnxt_en_dev_group_type = {
	.ct_item_ops = &bnxt_en_dev_item_ops,
	.ct_owner = THIS_MODULE,
};

static bool is_bnxt_ndev(struct net_device *netdev)
{
	struct bnxt *bp;

	if (netdev->netdev_ops != bnxt_get_netdev_ops_address())
		return false;
	bp = netdev_priv(netdev);

	if (bp->pdev->vendor != PCI_VENDOR_ID_BROADCOM) {
		pr_err("bnxt_quic: %s is not a Broadcom network device\n", netdev->name);
		return false;
	}
	return true;
}

static struct net_device *get_bnxt_ndev(const char *name)
{
	struct net_device *netdev = NULL;

	rtnl_lock();
	for_each_netdev(&init_net, netdev) {
		if (strcmp(name, netdev->name) == 0) {
			if (!is_bnxt_ndev(netdev))
				continue;
			rtnl_unlock();
			return netdev;
		}
	}
	rtnl_unlock();
	return NULL;
}

static u8 bnxt_is_quic_supported(struct net_device *netdev)
{
	struct bnxt *bp = netdev_priv(netdev);

	return bp->quic_info ? 1 : 0;
}

static struct config_group *make_bnxt_en_dev(struct config_group *group, const char *name)
{
	struct bnxt_en_dev_group *devgrp;
	struct net_device *netdev;

	netdev = get_bnxt_ndev(name);
	if (!netdev) {
		pr_info("bnxt_quic: %s is not a Broadcom network device\n", name);
		return ERR_PTR(-EINVAL);
	}

	if (!bnxt_is_quic_supported(netdev)) {
		pr_info("bnxt_quic: QUIC is not supported for  device %s\n", name);
		return ERR_PTR(-EINVAL);
	}

	devgrp = kzalloc(sizeof(*devgrp), GFP_KERNEL);
	if (!devgrp)
		return ERR_PTR(-ENOMEM);

	quic_flow.bp = netdev_priv(netdev);
	config_group_init_type_name(&devgrp->dev_group, name, &bnxt_en_dev_group_type);
	config_group_init_type_name(&devgrp->quic_group, "quic", &bnxt_en_quic_group_type);
	configfs_add_default_group(&devgrp->quic_group, &devgrp->dev_group);
	return &devgrp->dev_group;
}

static void drop_bnxt_en_dev(struct config_group *group, struct config_item *item)
{
	config_item_put(item);
}

static struct configfs_group_operations bnxt_en_group_ops = {
	.make_group = &make_bnxt_en_dev,
	.drop_item = &drop_bnxt_en_dev
};

static struct config_item_type bnxt_en_subsys_type = {
	.ct_group_ops	= &bnxt_en_group_ops,
	.ct_owner	= THIS_MODULE,
};

static struct configfs_subsystem bnxt_en_subsys = {
	.su_group	= {
		.cg_item	= {
			.ci_namebuf	= "bnxt_en",
			.ci_type	= &bnxt_en_subsys_type,
		},
	},
};

int bnxt_en_configfs_init(void)
{
	config_group_init(&bnxt_en_subsys.su_group);
	mutex_init(&bnxt_en_subsys.su_mutex);
	return configfs_register_subsystem(&bnxt_en_subsys);
}

void bnxt_en_configfs_exit(void)
{
	configfs_unregister_subsystem(&bnxt_en_subsys);
}

void bnxt_alloc_quic_info(struct bnxt *bp, struct hwrm_func_qcaps_output *resp)
{
	u16 max_keys = le16_to_cpu(resp->max_key_ctxs_alloc);
	struct bnxt_tls_info *quic = bp->quic_info;

	if (BNXT_VF(bp))
		return;

	if (!quic) {
		bool partition_mode = false;
		struct bnxt_kctx *kctx;
		u16 batch_sz = 0;
		int i;

		quic = kzalloc(sizeof(*quic), GFP_KERNEL);
		if (!quic)
			return;

		quic->counters = kzalloc(sizeof(atomic64_t) * BNXT_QUIC_MAX_COUNTERS,
					 GFP_KERNEL);
		if (!quic->counters) {
			kfree(quic);
			return;
		}

		if (BNXT_PARTITION_CAP(resp)) {
			batch_sz = le16_to_cpu(resp->ctxs_per_partition);
			if (batch_sz && batch_sz <= BNXT_KID_BATCH_SIZE)
				partition_mode = true;
		}
		for (i = 0; i < BNXT_MAX_CRYPTO_KEY_TYPE; i++) {
			kctx = &quic->kctx[i];
			kctx->type = i + FUNC_KEY_CTX_ALLOC_REQ_KEY_CTX_TYPE_QUIC_TX;
			if (i == BNXT_TX_CRYPTO_KEY_TYPE)
				kctx->max_ctx = BNXT_MAX_QUIC_TX_CRYPTO_KEYS;
			else
				kctx->max_ctx = BNXT_MAX_QUIC_RX_CRYPTO_KEYS;
			INIT_LIST_HEAD(&kctx->list);
			spin_lock_init(&kctx->lock);
			atomic_set(&kctx->alloc_pending, 0);
			init_waitqueue_head(&kctx->alloc_pending_wq);
			if (partition_mode) {
				int bmap_sz;

				bmap_sz = DIV_ROUND_UP(kctx->max_ctx, batch_sz);
				kctx->partition_bmap = bitmap_zalloc(bmap_sz, GFP_KERNEL);
				if (!kctx->partition_bmap)
					partition_mode = false;
			}
		}
		quic->partition_mode = partition_mode;
		quic->ctxs_per_partition = batch_sz;

		hash_init(quic->filter_tbl);
		spin_lock_init(&quic->filter_lock);

		atomic_set(&quic->pending, 0);

		bp->quic_info = quic;
	}
	quic->max_key_ctxs_alloc = max_keys;
}

void bnxt_free_quic_info(struct bnxt *bp)
{
	struct bnxt_tls_info *quic = bp->quic_info;
	struct bnxt_kid_info *kid, *tmp;
	struct bnxt_kctx *kctx;
	int i;

	if (!quic)
		return;

	/* Shutting down, no need to protect the lists. */
	for (i = 0; i < BNXT_MAX_CRYPTO_KEY_TYPE; i++) {
		kctx = &quic->kctx[i];
		list_for_each_entry_safe(kid, tmp, &kctx->list, list) {
			list_del(&kid->list);
			kfree(kid);
		}
		bitmap_free(kctx->partition_bmap);
	}
	bnxt_clear_cfa_tls_filters_tbl(bp);
	kmem_cache_destroy(quic->mpc_cache);
	kfree(quic->counters);
	kfree(quic);
	bp->quic_info = NULL;
}

int bnxt_quic_init(struct bnxt *bp)
{
	struct bnxt_hw_resc *hw_resc = &bp->hw_resc;
	struct bnxt_tls_info *quic = bp->quic_info;
	struct bnxt_hw_tls_resc *tls_resc;
	int rc;

	if (!quic)
		return 0;

	tls_resc = &hw_resc->tls_resc[BNXT_CRYPTO_TYPE_QUIC];
	quic->tck.max_ctx = tls_resc->resv_tx_key_ctxs;
	quic->rck.max_ctx = tls_resc->resv_rx_key_ctxs;

	if (!quic->tck.max_ctx || !quic->rck.max_ctx)
		return 0;

	if (quic->partition_mode) {
		rc = bnxt_set_partition_mode(bp);
		if (rc)
			quic->partition_mode = false;
	}

	rc = bnxt_hwrm_key_ctx_alloc(bp, &quic->tck, BNXT_KID_BATCH_SIZE, NULL,
				     BNXT_CRYPTO_TYPE_QUIC);
	if (rc)
		return rc;

	rc = bnxt_hwrm_key_ctx_alloc(bp, &quic->rck, BNXT_KID_BATCH_SIZE, NULL,
				     BNXT_CRYPTO_TYPE_QUIC);
	if (rc)
		return rc;

	quic->mpc_cache = kmem_cache_create("bnxt_quic",
					    sizeof(struct bnxt_crypto_cmd_ctx),
					    0, 0, NULL);
	if (!quic->mpc_cache)
		return -ENOMEM;
	return 0;
}

void bnxt_quic_rx(struct bnxt *bp, struct sk_buff *skb, u8 *data_ptr,
		  unsigned int len, struct rx_cmp *rxcmp,
		  struct rx_cmp_ext *rxcmp1)
{
	struct bnxt_tls_info *quic = bp->quic_info;
	unsigned int off = BNXT_METADATA_OFF(len);
	struct quic_metadata_msg *quic_md;
	u32 qmd;

	quic_md = (struct quic_metadata_msg *)(data_ptr + off);
	qmd = le32_to_cpu(quic_md->md_type_link_flags_kid_lo);

	if (qmd & QUIC_METADATA_MSG_FLAGS_PAYLOAD_DECRYPTED)
		atomic64_inc(&quic->counters[BNXT_QUIC_RX_PAYLOAD_DECRYPTED]);
	if (qmd & QUIC_METADATA_MSG_FLAGS_HDR_DECRYPTED)
		atomic64_inc(&quic->counters[BNXT_QUIC_RX_HDR_DECRYPTED]);
	if (qmd & QUIC_METADATA_MSG_FLAGS_HEADER_TYPE)
		atomic64_inc(&quic->counters[BNXT_QUIC_RX_LONG_HDR]);
	else
		atomic64_inc(&quic->counters[BNXT_QUIC_RX_SHORT_HDR]);
	if (qmd & QUIC_METADATA_MSG_FLAGS_KEY_PHASE_MISMATCH)
		atomic64_inc(&quic->counters[BNXT_QUIC_RX_KEY_PHASE_MISMATCH]);
	if (qmd & QUIC_METADATA_MSG_FLAGS_RUNT)
		atomic64_inc(&quic->counters[BNXT_QUIC_RX_RUNT]);

	atomic64_inc(&quic->counters[BNXT_QUIC_RX_HW_PKT]);
}
#else
int bnxt_en_configfs_init(void)
{
	return 0;
}

void bnxt_en_configfs_exit(void)
{
}

int bnxt_quic_init(struct bnxt *bp)
{
	return 0;
}

void bnxt_alloc_quic_info(struct bnxt *bp, struct hwrm_func_qcaps_output *resp)
{
}

void bnxt_free_quic_info(struct bnxt *bp)
{
}

void bnxt_get_quic_dst_conect_id(struct bnxt *bp,
				 struct hwrm_cfa_tls_filter_alloc_input *req)
{
}

void bnxt_quic_rx(struct bnxt *bp, struct sk_buff *skb, u8 *data_ptr,
		  unsigned int len, struct rx_cmp *rxcmp,
		  struct rx_cmp_ext *rxcmp1)
{
}

struct sk_buff *bnxt_quic_xmit(struct bnxt *bp, struct bnxt_tx_ring_info *txr,
			       struct sk_buff *skb, __le32 *lflags, u32 *kid)
{
	return skb;
}
#endif
