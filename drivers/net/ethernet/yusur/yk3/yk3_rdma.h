/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_RDMA_H
#define _YK3_RDMA_H
#include "yk3_common.h"

#ifndef USING_YK3_DRIVER
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <net/dcbnl.h>

/* PPP */
enum yk3_ppp_action_type {
	YK3_PPP_PTP_ACTION     = 0,
	YK3_PPP_RDMA_RTDEST    = 1,
	YK3_PPP_RDMA_QBASE     = 2,
};

struct yk3_ppp_rdma_val {
	u8 vport;
	union {
		u8 rtdest;
		u8 qbase;
	};
} __packed;

enum YK3_PPP_EME_HW_ID {
	YK3_PPP_EME_HW_ID_MAC0,
	YK3_PPP_EME_HW_ID_MAC1,
	YK3_PPP_EME_HW_ID_HOST,
	YK3_PPP_EME_HW_ID_SOC,
	YK3_PPP_EME_HW_ID_MAX,
};
#else
struct yk3_tbl_cfg;
struct yk3_tbl_entry;
struct yk3_tbl_ctl;
struct yk3_pdev_priv;
struct yk3_ppp_rdma_val;
#endif

#define YK3_RDMA_DEV_NAME "yk3_rdma_dev"

struct yk3_rdma_ops {
	// doe api begin {
	int (*create_tbl)(struct yk3_pdev_priv *pdev_priv, struct yk3_tbl_cfg *param);
	int (*delete_tbl)(struct yk3_pdev_priv *pdev_priv, int tbl_id);

	// insert table entry, only support hash/small-hash
	int (*entry_add)(struct yk3_pdev_priv *pdev_priv,
			 struct yk3_tbl_entry *param, u8 call_mode);

	// delete table entry, only support hash/small-hash
	int (*entry_delete)(struct yk3_pdev_priv *pdev_priv,
			    struct yk3_tbl_entry *param, u8 call_mode);

	// update table entry, support all table
	int (*entry_update)(struct yk3_pdev_priv *pdev_priv,
			    struct yk3_tbl_entry *param, u8 call_mode);

	// query table entry, support all table
	//
	//      0: ok
	//     <0: error
	int (*entry_query)(struct yk3_pdev_priv *pdev_priv,
			   struct yk3_tbl_entry *param, u8 call_mode);

	// Not supported
	int (*entry_batch)(struct yk3_pdev_priv *pdev_priv,
			   struct yk3_doe_batch *param, u8 call_mode);

	// YK3_DOE_TLB_EXISTED
	//	 1: exist
	//	 0: NOT exist
	//	<0: error
	// YK3_DOE_GET_PROTECT
	//	 1: protect open
	//	 0: protect close
	//	<0: error
	// YK3_DOE_GET_CACHE_INFO
	//	 0: ok
	//	<0: error
	// YK3_DOE_GET_COUNTER_ZIP
	//	 >0: zip enable
	//	 =0: zip disable
	//       <0: error
	// YK3_DOE_GET_HASH_TABLE_MAX
	//	 >0: limit
	//	<=0: bug
	// YK3_DOE_GET_HASH_ENTRY_COUNT
	//	 =0: ok
	//	 <0: error
	int (*ctl_get)(struct yk3_pdev_priv *pdev_priv, struct yk3_tbl_ctl *param);
	// return
	//	 0: ok
	//	<0: error
	int (*ctl_set)(struct yk3_pdev_priv *pdev_priv, struct yk3_tbl_ctl *param);
	// doe api end }

	/* ppp eme */
	int (*ppp_rdma_enable)(struct yk3_pdev_priv *pdev_priv);
	int (*ppp_rdma_disable)(struct yk3_pdev_priv *pdev_priv);
	int (*ppp_action_modify)(struct yk3_pdev_priv *pdev_priv,
				 u8 act_type, u16 act_id, void *val);
	int (*ppp_action_query)(struct yk3_pdev_priv *pdev_priv,
				u8 act_type, u16 act_id, void *val);
	// 创建ppp精确表
	// hw_id:
	//	enum YK3_PPP_EME_HW_ID:
	//		YK3_PPP_EME_HW_ID_MAC0,
	//		YK3_PPP_EME_HW_ID_MAC1,
	//		YK3_PPP_EME_HW_ID_HOST,
	//		YK3_PPP_EME_HW_ID_SOC,
	int (*ppp_eme_create_table)(struct yk3_pdev_priv *pdev_priv, int hw_id,
				    u32 table_id, u32 key_size, u32 value_size);

	// 删除ppp精确表
	int (*ppp_eme_delete_table)(struct yk3_pdev_priv *pdev_priv, int hw_id,
				    u32 table_id);

	// 添加ppp精确表规则
	int (*ppp_eme_add_rule)(struct yk3_pdev_priv *pdev_priv, int hw_id,
				u32 table_id, u8 *key, u8 *value);

	// 删除ppp精确表规则
	int (*ppp_eme_del_rule)(struct yk3_pdev_priv *pdev_priv, int hw_id,
				u32 table_id, u8 *key);

	// 修改ppp精确表规则action
	int (*ppp_eme_mod_rule)(struct yk3_pdev_priv *pdev_priv, int hw_id,
				u32 table_id, u8 *key, u8 *value);

	// 查找ppp精确表规则action
	int (*ppp_eme_get_rule)(struct yk3_pdev_priv *pdev_priv, int hw_id,
				u32 table_id, u8 *key, u8 *value);

	// 配置PFC使能开关
	int (*rdma_set_pfc)(struct yk3_pdev_priv *pdev_priv, struct ieee_pfc *pfc);

	// 查询PFC当前配置
	int (*rdma_get_pfc)(struct yk3_pdev_priv *pdev_priv, struct ieee_pfc *pfc);

	/* qos */
	int (*set_qp_tc)(struct yk3_pdev_priv *pdev_priv, int qp, int tc);
	int (*set_qp_rate)(struct yk3_pdev_priv *pdev_priv, int qp, u32 maxrate);
	int (*set_rt_dst)(struct yk3_pdev_priv *pdev_priv,
			  u8 qrange_id, u32 qrange, u8 rt_dst_id, u32 rt_dst);
};

/*
 * #include "yk3_rdma.h"
 *
 * static irqreturn_t irq_handler_callback(int irqn, void *data)
 * {
 *         return IRQ_HANDLED;
 * }
 *
 * int rdma_test_probe(struct platform_device *pdev)
 * {
 *         struct yk3_rdma_data *rdma_data = dev_get_platdata(&pdev->dev);
 *         int irqn;
 *
 *         pr_info("version major %d, minor %d\n",
 *                 rdma_data->version_major, rdma_data->version_minor);
 *         pr_info("vector start %d, num %d\n",
 *                 rdma_data->vector_start, rdma_data->vector_num);
 *
 *         irqn = pci_irq_vector(pdev, rdma_data->vector_start + i); // i >= 0 && i < vector_num
 *         ret = request_irq(irqn, irq_handler_callback, 0, name, pointer);
 *
 *         return 0;
 * }
 *
 * int rdma_test_remove(struct platform_device *pdev)
 * {
 *         return 0;
 * }
 *
 * static struct platform_driver rdma_test_driver = {
 *         .probe = rdma_test_probe,
 *         .remove = rdma_test_remove,
 *         .driver = {
 *                 .name = YK3_RDMA_DEV_NAME,
 *         },
 * };
 *
 * static int __init rdma_test_init(void)
 * {
 *         return platform_driver_register(&rdma_test_driver);
 * }
 *
 * static void __exit rdma_test_exit(void)
 * {
 *         platform_driver_unregister(&rdma_test_driver);
 * }
 *
 * module_init(rdma_test_init);
 * module_exit(rdma_test_exit);
 *
 * MODULE_DESCRIPTION("Yusur K3 and K3MAX Pcie Device Driver");
 * MODULE_AUTHOR("YUSUR Technology Co., Ltd.");
 * MODULE_LICENSE("GPL");
 * MODULE_VERSION(YK3_GIT_VERSION);
 */

struct yk3_rdma_data {
	int                   version_major; /* big change, cannot compat */
	int                   version_minor; /* little change, compat */
	int                   vector_start;  /* irq vector start id for rdma */
	int                   vector_num;    /* irq vector num for rdma */
	unsigned char         pf_num;        /* pf num for rdma */
	struct pci_dev       *pdev;
	struct yk3_pdev_priv *pdev_priv;
	struct net_device    *ndev;
	void                 *rdma_priv; /* rdma private pointer, yk3 not use */
	struct yk3_rdma_ops   ops;
};

#endif /* _YK3_RDMA_H */
