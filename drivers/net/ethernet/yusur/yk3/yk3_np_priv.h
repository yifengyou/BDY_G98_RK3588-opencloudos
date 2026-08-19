/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __YK3_NP_PRIV_H_
#define __YK3_NP_PRIV_H_

#include "yk3_base.h"
#include "yk3_np.h"

#ifndef YK3_SKIP_NP
#define YK3_SKIP_NP 0
#endif

enum ysc_np_cfg_type {
	YK3_NP_CFG_DOE_TBL_READY      = 0,
	YK3_NP_CFG_FCS_ERR_DROP       = 1,
	YK3_NP_CFG_TM_TRUST_PRI       = 2,
	YK3_NP_CFG_LRO                = 3,
	YK3_NP_CFG_LAN_NUM            = 4,
	YK3_NP_CFG_IGN_PPP            = 14,
	YK3_NP_CFG_TRUST_PPP          = 15,
	YK3_NP_CFG_BYPASS_OFFLOAD     = 16,
	YK3_NP_CFG_IGN_TNL_V4_ID      = 17,
	YK3_NP_CFG_IGN_FRAG_L4_PORT   = 18,
	YK3_NP_CFG_TBL_CACHE_MISS     = 19,
	YK3_NP_CFG_MA_DISPATCH_POLICY = 20,
	YK3_NP_CFG_EMP_METER,
	YK3_NP_CFG_SIMPLE_FORWARD,
	YK3_NP_CFG_MAX,
};

struct yk3_np_ops;

int yk3_np_doe_tbl_init(struct yk3_pdev_priv *pdev_priv);
void yk3_np_doe_tbl_fini(struct yk3_pdev_priv *pdev_priv);

#define YK3_NP_REGS_BAR              0
#define YK3_NP_BASE                  (0x1000000)
#define YK3_NP_PPE_CLUSTE_NUM        (24)
#define YK3_NP_REG_MAGIC             (0xdeadbadb)
#define YK3_NP_REG_OFFSET_MAX        (0x540000)

enum yk3_np_cls_role_type {
	YK3_NP_CLS_ROLE_DISABLED       = 0,
	YK3_NP_CLS_ROLE_PARSER         = 1,
	YK3_NP_CLS_ROLE_MIRROR_METER   = 2,
	YK3_NP_CLS_ROLE_LRO            = 3,
	YK3_NP_CLS_ROLE_MAX,
};

struct yk3_np {
	struct yk3_pdev_priv         *pdev_priv;
	const struct yk3_np_ops      *ops;
	struct yk3_lag               *lag;
	struct workqueue_struct      *wq;
	struct dentry                *debugfs_root;
	enum yk3_np_cls_role_type    cls_role[YK3_NP_PPE_CLUSTE_NUM];
	struct list_head             table_head;
	spinlock_t                   cfg_lock; /* for cfg update. */
};

int yk3_np_set_ign_frag_l4_port(struct yk3_pdev_priv *pdev_priv, bool ignore);
int yk3_np_set_tbl_cache_miss(struct yk3_pdev_priv *pdev_priv, bool cache_miss);

int yk3_doe_tbl_exist(struct yk3_pdev_priv *pdev_priv, u8 tbl_id, bool *exist);
int yk3_doe_counter_enable(struct yk3_pdev_priv *pdev_priv, u8 tbl_id,
			   u32 idx,
			   bool op_enable,
			   bool pri,
			   bool clear_data);
int yk3_doe_counter_load(struct yk3_pdev_priv *pdev_priv, u8 tbl_id,
			 u32 idx, u64 *pkts, u64 *bytes,
			 bool allow_sleep);
#endif /* __YK3_NP_PRIV_H_ */
