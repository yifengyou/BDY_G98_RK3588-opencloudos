/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024, DapuStor Corporation.
 */

#ifndef __DN200_HWIF_H__
#define __DN200_HWIF_H__

#include <linux/netdevice.h>
#include "dn200_cfg.h"
#include "common.h"
#define dn200_do_void_callback(__priv, __module, __cname,  __arg0, __args...) \
({ \
	int __result = -EINVAL; \
	if ((__priv)->hw->__module && (__priv)->hw->__module->__cname) { \
		(__priv)->hw->__module->__cname((__arg0), ##__args); \
		__result = 0; \
	} \
	__result; \
})
#define dn200_do_callback(__priv, __module, __cname,  __arg0, __args...) \
({ \
	int __result = -EINVAL; \
	if ((__priv)->hw->__module && (__priv)->hw->__module->__cname) \
		__result = (__priv)->hw->__module->__cname((__arg0), ##__args); \
	__result; \
})

struct dn200_extra_stats;
struct dn200_safety_stats;
struct dma_desc;
struct dn200_fdir_filter;

/* Descriptors helpers */
struct dn200_desc_ops {
	/* DMA RX descriptor ring initialization */
	void (*init_rx_desc)(struct dma_desc *p, int disable_rx_ic, int mode,
			     int end, int bfsize);
	/* DMA TX descriptor ring initialization */
	void (*init_tx_desc)(struct dma_desc *p, int mode, int end);
	/* Invoked by the xmit function to prepare the tx descriptor */
	void (*prepare_tx_desc)(struct dma_desc *p, int is_fs, int len,
				bool csum_flag, int mode, bool tx_own, bool ls,
				unsigned int tot_pkt_len);
	void (*prepare_tso_tx_desc)(struct dma_desc *p, int is_fs, int len1,
				    int len2, bool tx_own, bool ls,
				    unsigned int tcphdrlen,
				    unsigned int tcppayloadlen);
	/* Set/get the owner of the descriptor */
	void (*set_tx_owner)(struct dma_desc *p);
	int (*get_tx_owner)(struct dma_desc *p);
	/* Clean the tx descriptor as soon as the tx irq is received */
	void (*release_tx_desc)(struct dma_desc *p, int mode);
	/* Clear interrupt on tx frame completion. When this bit is
	 * set an interrupt happens as soon as the frame is transmitted
	 */
	void (*set_tx_ic)(struct dma_desc *p);
	/* Last tx segment reports the transmit status */
	int (*get_tx_ls)(struct dma_desc *p);
	/* Return the transmit status looking at the TDES1 */
	int (*tx_status)(void *data, struct dn200_extra_stats *x,
			 struct dma_desc *p, void __iomem *ioaddr);
	/* Get the buffer size from the descriptor */
	int (*get_tx_len)(struct dma_desc *p);
	/* Handle extra events on specific interrupts hw dependent */
	void (*set_rx_owner)(struct dma_desc *p, int disable_rx_ic);
	/* Get the receive frame size */
	int (*get_rx_frame_len)(struct dma_desc *p, int rx_coe_type);
	/* Return the reception status looking at the RDES1 */
	int (*rx_status)(void *data, struct dn200_extra_stats *x,
			 struct dma_desc *p, bool rec_all);
	/* Set tx timestamp enable bit */
	void (*enable_tx_timestamp)(struct dma_desc *p);
	/* get tx timestamp status */
	int (*get_tx_timestamp_status)(struct dma_desc *p);
	/* get timestamp value */
	void (*get_timestamp)(void *desc, u32 ats, u64 *ts);
	/* get rx timestamp status */
	int (*get_rx_timestamp_status)(void *desc, void *next_desc, u32 ats);
	/* Display ring */
	void (*display_ring)(void *head, unsigned int size, bool not_tbl,
			     dma_addr_t dma_rx_phy, unsigned int desc_size,
			     struct mac_device_info *hw);
	/* set MSS via context descriptor */
	void (*set_mss)(struct dma_desc *p, unsigned int mss);
	/* get descriptor skbuff address */
	void (*get_addr)(struct dma_desc *p, unsigned int *addr);
	/* set descriptor skbuff address */
	void (*set_addr)(struct dma_desc *p, dma_addr_t addr,
			 struct mac_device_info *hw);
	/* clear descriptor */
	void (*clear)(struct dma_desc *p);
	/* RSS */
	int (*get_rx_hash)(struct dma_desc *p, u32 *hash,
			   enum pkt_hash_types *type);
	void (*get_rx_header_len)(struct dma_desc *p, unsigned int *len);
	void (*set_sec_addr)(struct dma_desc *p, dma_addr_t addr,
			     bool buf2_valid, struct mac_device_info *hw);
	void (*set_sarc)(struct dma_desc *p, u32 sarc_type);
	void (*set_vlan_tag)(struct dma_desc *p, u16 tag, u16 inner_tag,
			     u32 inner_type);
	void (*set_vlan)(struct dma_desc *p, u32 type);
	int (*get_ovt)(struct dma_desc *p);
	void (*set_vxlan)(struct dma_desc *p);
};

#define dn200_init_rx_desc(__priv, __args...) \
	dn200_do_void_callback(__priv, desc, init_rx_desc, __args)
#define dn200_init_tx_desc(__priv, __args...) \
	dn200_do_void_callback(__priv, desc, init_tx_desc, __args)
#define dn200_prepare_tx_desc(__priv, __args...) \
	dn200_do_void_callback(__priv, desc, prepare_tx_desc, __args)
#define dn200_prepare_tso_tx_desc(__priv, __args...) \
	dn200_do_void_callback(__priv, desc, prepare_tso_tx_desc, __args)
#define dn200_set_tx_owner(__priv, __args...) \
	dn200_do_void_callback(__priv, desc, set_tx_owner, __args)
#define dn200_get_tx_owner(__priv, __args...) \
	dn200_do_callback(__priv, desc, get_tx_owner, __args)
#define dn200_release_tx_desc(__priv, __args...) \
	dn200_do_void_callback(__priv, desc, release_tx_desc, __args)
#define dn200_set_tx_ic(__priv, __args...) \
	dn200_do_void_callback(__priv, desc, set_tx_ic, __args)
#define dn200_get_tx_ls(__priv, __args...) \
	dn200_do_callback(__priv, desc, get_tx_ls, __args)
#define dn200_tx_status(__priv, __args...) \
	dn200_do_callback(__priv, desc, tx_status, __args)
#define dn200_get_tx_len(__priv, __args...) \
	dn200_do_callback(__priv, desc, get_tx_len, __args)
#define dn200_set_rx_owner(__priv, __args...) \
	dn200_do_void_callback(__priv, desc, set_rx_owner, __args)
#define dn200_get_rx_frame_len(__priv, __args...) \
	dn200_do_callback(__priv, desc, get_rx_frame_len, __args)
#define dn200_rx_status(__priv, __args...) \
	dn200_do_callback(__priv, desc, rx_status, __args)
#define dn200_enable_tx_timestamp(__priv, __args...) \
	dn200_do_void_callback(__priv, desc, enable_tx_timestamp, __args)
#define dn200_get_tx_timestamp_status(__priv, __args...) \
	dn200_do_callback(__priv, desc, get_tx_timestamp_status, __args)
#define dn200_get_timestamp(__priv, __args...) \
	dn200_do_void_callback(__priv, desc, get_timestamp, __args)
#define dn200_get_rx_timestamp_status(__priv, __args...) \
	dn200_do_callback(__priv, desc, get_rx_timestamp_status, __args)
#define dn200_display_ring(__priv, __args...) \
	dn200_do_void_callback(__priv, desc, display_ring, __args)
#define dn200_set_mss(__priv, __args...) \
	dn200_do_void_callback(__priv, desc, set_mss, __args)
#define dn200_get_desc_addr(__priv, __args...) \
	dn200_do_void_callback(__priv, desc, get_addr, __args)
#define dn200_set_desc_addr(__priv, __args...) \
	dn200_do_void_callback(__priv, desc, set_addr, __args)
#define dn200_clear_desc(__priv, __args...) \
	dn200_do_void_callback(__priv, desc, clear, __args)
#define dn200_get_rx_hash(__priv, __args...) \
	dn200_do_callback(__priv, desc, get_rx_hash, __args)
#define dn200_get_rx_header_len(__priv, __args...) \
	dn200_do_void_callback(__priv, desc, get_rx_header_len, __args)
#define dn200_set_desc_sec_addr(__priv, __args...) \
	dn200_do_void_callback(__priv, desc, set_sec_addr, __args)
#define dn200_set_desc_sarc(__priv, __args...) \
	dn200_do_void_callback(__priv, desc, set_sarc, __args)
#define dn200_set_desc_vlan_tag(__priv, __args...) \
	dn200_do_void_callback(__priv, desc, set_vlan_tag, __args)
#define dn200_set_desc_vlan(__priv, __args...) \
	dn200_do_void_callback(__priv, desc, set_vlan, __args)
#define dn200_get_ovt(__priv, __args...) \
	dn200_do_callback(__priv, desc, get_ovt, __args)
#define dn200_set_vxlan(__priv, __args...) \
	dn200_do_void_callback(__priv, desc, set_vxlan, __args)
struct dn200_dma_cfg;
struct dma_features;

/* Specific DMA helpers */
struct dn200_dma_ops {
	/* DMA core initialization */
	int (*reset)(void __iomem *ioaddr, struct mac_device_info *hw);
	void (*init)(void __iomem *ioaddr, struct dn200_dma_cfg *dma_cfg,
		     int atds, struct mac_device_info *hw);
	void (*init_chan)(void __iomem *ioaddr,
			  struct dn200_dma_cfg *dma_cfg, u32 chan,
			  struct mac_device_info *hw);
	void (*init_rx_chan)(void __iomem *ioaddr,
			     struct dn200_dma_cfg *dma_cfg, dma_addr_t phy,
			     u32 chan, struct mac_device_info *hw);
	void (*init_tx_chan)(void __iomem *ioaddr,
			     struct dn200_dma_cfg *dma_cfg, dma_addr_t phy,
			     u32 chan, struct mac_device_info *hw);
	/* Configure the AXI Bus Mode Register */
	void (*axi)(void __iomem *ioaddr, struct dn200_axi *axi,
		    struct mac_device_info *hw);
	/* Dump DMA registers */
	void (*dump_regs)(void __iomem *ioaddr, u32 *reg_space);
	void (*dma_rx_mode)(void __iomem *ioaddr, int mode, u32 channel,
			    int fifosz, u8 qmode, struct mac_device_info *hw);
	void (*dma_tx_mode)(void __iomem *ioaddr, int mode, u32 channel,
			    int fifosz, u8 qmode, u8 tc,
			    struct mac_device_info *hw);
	void (*dma_rx_all_set)(void __iomem *ioaddr, u32 channel, u8 enable,
			       struct mac_device_info *hw);
	void (*dma_mode_reset)(void __iomem *ioaddr, u8 channel,
			       struct mac_device_info *hw);
	/* To track extra statistic (if supported) */
	void (*dma_diagnostic_fr)(void *data, struct dn200_extra_stats *x,
				  void __iomem *ioaddr,
				  struct mac_device_info *hw);
	void (*enable_dma_transmission)(void __iomem *ioaddr);
	void (*enable_dma_irq)(void __iomem *ioaddr, u32 chan,
			       bool rx, bool tx, struct mac_device_info *hw);
	void (*disable_dma_irq)(void __iomem *ioaddr, u32 chan,
				bool rx, bool tx, struct mac_device_info *hw);
	void (*start_tx)(void __iomem *ioaddr, u32 chan,
			 struct mac_device_info *hw);
	void (*stop_tx)(void __iomem *ioaddr, u32 chan,
			struct mac_device_info *hw);
	void (*start_rx)(void __iomem *ioaddr, u32 chan,
			 struct mac_device_info *hw);
	void (*stop_rx)(void __iomem *ioaddr, u32 chan,
			struct mac_device_info *hw);
	int (*dma_interrupt)(void __iomem *ioaddr,
			     struct dn200_extra_stats *x, u32 chan, u32 dir,
			     struct mac_device_info *hw);
	/* If supported then get the optional core features */
	int (*get_hw_feature)(void __iomem *ioaddr,
			      struct dma_features *dma_cap);
	/* Program the HW RX Watchdog */
	void (*rx_watchdog)(void __iomem *ioaddr, u32 riwt, u32 queue,
			    struct mac_device_info *hw);
	void (*set_tx_ring_len)(void __iomem *ioaddr, u32 len, u32 chan,
				struct mac_device_info *hw);
	void (*set_rx_ring_len)(void __iomem *ioaddr, u32 len, u32 chan,
				struct mac_device_info *hw);
	void (*set_rx_tail_ptr)(void __iomem *ioaddr, u32 tail_ptr, u32 chan,
				struct mac_device_info *hw);
	void (*set_tx_tail_ptr)(void __iomem *ioaddr, u32 tail_ptr, u32 chan,
				struct mac_device_info *hw);
	u32 (*get_rx_curr_ptr)(void __iomem *ioaddr, u32 chan,
				struct mac_device_info *hw);
	void (*enable_tso)(void __iomem *ioaddr, bool en, u32 chan,
			   struct mac_device_info *hw);
	void (*qmode)(void __iomem *ioaddr, u32 channel, u8 qmode,
		      struct mac_device_info *hw);
	void (*set_bfsize)(void __iomem *ioaddr, int bfsize, u32 chan,
			   struct mac_device_info *hw);
	void (*enable_sph)(void __iomem *ioaddr, bool en, u32 chan,
			   struct mac_device_info *hw);
	int (*enable_tbs)(void __iomem *ioaddr, bool en, u32 chan,
			  struct mac_device_info *hw);
	void (*dma_reset_chan)(void __iomem *ioaddr, u32 chan,
			       struct mac_device_info *hw);
	int (*check_chan_status)(void __iomem *ioaddr, u32 chan,
				 struct mac_device_info *hw, bool is_tx);
};

#define dn200_dma_reset(__priv, __args...) \
	dn200_do_callback(__priv, dma, reset, __args)
#define dn200_dma_init(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, init, __args)
#define dn200_init_chan(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, init_chan, __args)
#define dn200_init_rx_chan(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, init_rx_chan, __args)
#define dn200_init_tx_chan(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, init_tx_chan, __args)
#define dn200_axi(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, axi, __args)
#define dn200_dump_dma_regs(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, dump_regs, __args)
#define dn200_dma_rx_mode(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, dma_rx_mode, __args)
#define dn200_dma_tx_mode(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, dma_tx_mode, __args)
#define dn200_dma_rx_all_set(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, dma_rx_all_set, __args)
#define dn200_dma_mode_reset(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, dma_mode_reset, __args)
#define dn200_dma_diagnostic_fr(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, dma_diagnostic_fr, __args)
#define dn200_enable_dma_transmission(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, enable_dma_transmission, __args)
#define dn200_enable_dma_irq(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, enable_dma_irq, __args)
#define dn200_disable_dma_irq(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, disable_dma_irq, __args)
#define dn200_start_tx(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, start_tx, __args)
#define dn200_stop_tx(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, stop_tx, __args)
#define dn200_start_rx(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, start_rx, __args)
#define dn200_stop_rx(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, stop_rx, __args)
#define dn200_dma_interrupt_status(__priv, __args...) \
	dn200_do_callback(__priv, dma, dma_interrupt, __args)
#define dn200_get_hw_feature(__priv, __args...) \
	dn200_do_callback(__priv, dma, get_hw_feature, __args)
#define dn200_rx_watchdog(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, rx_watchdog, __args)
#define dn200_set_tx_ring_len(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, set_tx_ring_len, __args)
#define dn200_set_rx_ring_len(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, set_rx_ring_len, __args)
#define dn200_set_rx_tail_ptr(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, set_rx_tail_ptr, __args)
#define dn200_set_tx_tail_ptr(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, set_tx_tail_ptr, __args)
#define dn200_get_rx_curr_ptr(__priv, __args...) \
	dn200_do_callback(__priv, dma, get_rx_curr_ptr, __args)
#define dn200_enable_tso(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, enable_tso, __args)
#define dn200_dma_qmode(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, qmode, __args)
#define dn200_set_dma_bfsize(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, set_bfsize, __args)
#define dn200_enable_sph(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, enable_sph, __args)
#define dn200_enable_tbs(__priv, __args...) \
	dn200_do_callback(__priv, dma, enable_tbs, __args)
#define dn200_dma_reset_chan(__priv, __args...) \
	dn200_do_void_callback(__priv, dma, dma_reset_chan, __args)
#define dn200_check_chan_status(__priv, __args...) \
	dn200_do_callback(__priv, dma, check_chan_status, __args)

struct mac_device_info;
struct net_device;
struct rgmii_adv;
struct dn200_tc_entry;
struct dn200_pps_cfg;
struct dn200_rss;
struct dn200_est;

/* Helpers to program the MAC core */
struct dn200_ops {
	/* MAC core initialization */
	void (*core_init)(struct mac_device_info *hw, struct net_device *dev);
	/* Enable the MAC RX/TX */
	void (*set_mac)(void __iomem *ioaddr, bool enable,
			struct mac_device_info *hw);
	/* Enable the MAC RX */
	void (*set_mac_rx)(void __iomem *ioaddr, bool enable);
	/* Get the MAC RX */
	int (*get_mac_rx)(void __iomem *ioaddr);
	/* Enable and verify that the IPC module is supported */
	int (*rx_ipc)(struct mac_device_info *hw);
	/* Enable RX Queues */
	void (*rx_queue_enable)(struct mac_device_info *hw, u8 mode,
				u32 queue);
	/* Disable RX Queues */
	void (*rx_queue_disable)(struct mac_device_info *hw, u32 queue);
	/* RX Queues Priority */
	void (*rx_queue_prio)(struct mac_device_info *hw, u32 prio, u32 queue);
	/* TX Queues Priority */
	void (*tx_queue_prio)(struct mac_device_info *hw, u32 prio, u32 queue);
	/* RX Queues Routing */
	void (*rx_queue_routing)(struct mac_device_info *hw, u8 packet,
				 u32 queue);
	/* Program RX Algorithms */
	void (*prog_mtl_rx_algorithms)(struct mac_device_info *hw, u32 rx_alg);
	/* Program TX Algorithms */
	void (*prog_mtl_tx_algorithms)(struct mac_device_info *hw, u32 tx_alg);
	/* Set MTL TX queues weight */
	void (*set_mtl_tx_queue_weight)(struct mac_device_info *hw,
					u32 weight, u32 queue);
	/* Set MTL RX queues weight */
	void (*set_mtl_rx_queue_weight)(struct mac_device_info *hw,
					u32 weight, u32 queue);
	/* RX MTL queue to RX dma mapping */
	void (*map_mtl_to_dma)(struct mac_device_info *hw, u32 queue,
			       u32 chan);
	/* Set RX MTL queue to RX dma mapping as dynamic selection */
	void (*mtl_dynamic_chan_set)(struct mac_device_info *hw,
				     u32 queue, bool dynamic);
	/* Configure AV Algorithm */
	void (*config_cbs)(struct mac_device_info *hw, u32 send_slope,
			   u32 idle_slope, u32 high_credit, u32 low_credit,
			   u32 queue);
	/* Dump MAC registers */
	void (*dump_regs)(struct mac_device_info *hw, u32 *reg_space);
	/* Handle extra events on specific interrupts hw dependent */
	int (*host_irq_status)(struct mac_device_info *hw,
			       struct dn200_extra_stats *x);
	/* Handle MTL interrupts */
	int (*host_mtl_irq_status)(struct mac_device_info *hw, u32 chan);
	/* Multicast filter setting */
	void (*set_filter)(struct mac_device_info *hw,
			   struct net_device *dev, u8 *wakeup_wq);
	void (*wq_set_filter)(struct mac_device_info *hw,
				      struct net_device *dev, bool is_vf,
					  struct dn200_vf_rxp_async_info *async_info);
	/* Flow control setting */
	void (*flow_ctrl)(struct mac_device_info *hw, unsigned int duplex,
			  unsigned int fc, unsigned int pause_time, u32 tx_cnt);
	/* Set/Get Unicast MAC addresses */
	int (*set_umac_addr)(struct mac_device_info *hw, unsigned char *addr,
			      unsigned int reg_n, u8 *wakeup_wq);
	int (*wq_set_umac_addr)(struct mac_device_info *hw,
				   unsigned char *addr, unsigned int reg_n,
				   struct dn200_vf_rxp_async_info *async_info);
	void (*get_umac_addr)(struct mac_device_info *hw, unsigned char *addr,
			      unsigned int reg_n);
	void (*set_eee_mode)(struct mac_device_info *hw,
			     bool en_tx_lpi_clockgating, bool en_tx_lpi_auto_timer);
	void (*reset_eee_mode)(struct mac_device_info *hw);
	void (*set_eee_lpi_entry_timer)(struct mac_device_info *hw, int et);
	void (*set_eee_timer)(struct mac_device_info *hw, int ls, int tw);
	void (*set_eee_pls)(struct mac_device_info *hw, int link);
	void (*debug)(void __iomem *ioaddr, struct dn200_extra_stats *x,
		      u32 rx_queues, u32 tx_queues);
	/* PCS calls */
	void (*pcs_ctrl_ane)(void __iomem *ioaddr, bool ane, bool srgmi_ral,
			     bool loopback);
	void (*pcs_rane)(void __iomem *ioaddr, bool restart);
	void (*pcs_get_adv_lp)(void __iomem *ioaddr, struct rgmii_adv *adv);
	/* Safety Features */
	int (*safety_feat_config)(void __iomem *ioaddr, unsigned int asp,
				  struct dn200_safety_feature_cfg *safety_cfg,
				  struct mac_device_info *hw);
	int (*safety_feat_irq_status)(struct net_device *ndev,
				      void __iomem *ioaddr, unsigned int asp,
				      struct dn200_safety_stats *stats);
	int (*safety_feat_dump)(struct dn200_safety_stats *stats, int index,
				unsigned long *count, const char **desc);
	/* Flexible RX Parser */
	int (*rxp_config)(struct mac_device_info *hw,
			  struct dn200_tc_entry *entries, unsigned int count);
	/* Flexible PPS */
	int (*flex_pps_config)(void __iomem *ioaddr, int index,
			       struct dn200_pps_cfg *cfg, bool enable,
			       u32 sub_second_inc, u32 systime_flags);
	/* Loopback for selftests */
	int (*set_mac_loopback)(void __iomem *ioaddr, bool enable);
	/* RSS */
	int (*rss_configure)(struct mac_device_info *hw,
			     struct dn200_rss *cfg, u32 num_rxq);
	/* VLAN */
	void (*update_vlan_hash)(struct mac_device_info *hw, u32 hash,
				 __le16 perfect_match, bool is_double);
	void (*enable_vlan)(struct mac_device_info *hw, u32 type);
	void (*init_hw_vlan_rx_fltr)(struct mac_device_info *hw);
	int (*add_hw_vlan_rx_fltr)(struct net_device *dev,
				   struct mac_device_info *hw,
				   __be16 proto, u16 vid, uint8_t off,
				   bool is_last);
	int (*del_hw_vlan_rx_fltr)(struct net_device *dev,
				   struct mac_device_info *hw, __be16 proto,
				   u16 vid, uint8_t off, bool is_last);
	void (*config_vlan_rx_fltr)(struct mac_device_info *hw, bool enable);
	void (*rx_vlan_stripping_config)(struct mac_device_info *hw,
					 bool enable);
	void (*restore_hw_vlan_rx_fltr)(struct net_device *dev,
					struct mac_device_info *hw);
	/* TX Timestamp */
	int (*get_mac_tx_timestamp)(struct mac_device_info *hw, u64 *ts);
	/* Source Address Insertion / Replacement */
	void (*sarc_configure)(void __iomem *ioaddr, int val);
	/* Filtering */
	int (*config_l3_filter)(struct mac_device_info *hw, u32 filter_no,
				bool en, bool ipv6, bool sa, bool inv,
				u32 match);
	int (*config_ntuple_filter)(struct mac_device_info *hw, u32 filter_no,
				    struct dn200_fdir_filter *input, bool en);
	int (*config_l4_filter)(struct mac_device_info *hw, u32 filter_no,
				bool en, bool udp, bool sa, bool inv,
				u32 match);
	void (*l3_l4_filter_config)(struct mac_device_info *hw, bool en);
	void (*set_arp_offload)(struct mac_device_info *hw, bool en, u32 addr);
	int (*est_configure)(void __iomem *ioaddr, struct dn200_est *cfg,
			     unsigned int ptp_rate);
	void (*est_irq_status)(void __iomem *ioaddr, struct net_device *dev,
			       struct dn200_extra_stats *x, u32 txqcnt);
	void (*fpe_configure)(void __iomem *ioaddr, u32 num_txq, u32 num_rxq,
			      bool enable);
	void (*fpe_send_mpacket)(void __iomem *ioaddr,
				 enum dn200_mpacket_type type);
	int (*fpe_irq_status)(void __iomem *ioaddr, struct net_device *dev);
	void (*rx_dds_config)(struct mac_device_info *hw, bool enable);
	int (*rxp_broadcast)(struct mac_device_info *hw);
	void (*rxp_filter_get)(struct mac_device_info *hw,
			       struct seq_file *seq);
	void (*rxp_clear)(struct mac_device_info *hw);
	void (*vf_del_rxp)(struct mac_device_info *hw);
	void (*wq_vf_del_rxp)(struct mac_device_info *hw, int offset, u8 rxq_start);
	void (*clear_vf_rxp)(struct mac_device_info *hw, u8 vf_off);
	void (*vf_append_rxp_bc)(struct mac_device_info *hw, u16 channel);
	void (*mtl_reset)(struct mac_device_info *hw, u32 queue, u32 chan,
			  u8 mode);
	int (*tx_queue_flush)(struct mac_device_info *hw, u32 queue);
	int (*reset_rxp)(struct mac_device_info *hw);
	int (*rxf_and_acl_mem_reset)(struct mac_device_info *hw);
};

#define dn200_core_init(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, core_init, __args)
#define dn200_mac_set(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, set_mac, __args)
#define dn200_mac_rx_set(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, set_mac_rx, __args)
#define dn200_mac_rx_get(__priv, __args...) \
	dn200_do_callback(__priv, mac, get_mac_rx, __args)
#define dn200_rx_ipc(__priv, __args...) \
	dn200_do_callback(__priv, mac, rx_ipc, __args)
#define dn200_rx_queue_enable(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, rx_queue_enable, __args)
#define dn200_rx_queue_disable(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, rx_queue_disable, __args)
#define dn200_rx_dds_config(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, rx_dds_config, __args)
#define dn200_rx_queue_prio(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, rx_queue_prio, __args)
#define dn200_tx_queue_prio(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, tx_queue_prio, __args)
#define dn200_rx_queue_routing(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, rx_queue_routing, __args)
#define dn200_prog_mtl_rx_algorithms(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, prog_mtl_rx_algorithms, __args)
#define dn200_prog_mtl_tx_algorithms(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, prog_mtl_tx_algorithms, __args)
#define dn200_set_mtl_tx_queue_weight(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, set_mtl_tx_queue_weight, __args)
#define dn200_set_mtl_rx_queue_weight(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, set_mtl_rx_queue_weight, __args)
#define dn200_map_mtl_to_dma(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, map_mtl_to_dma, __args)
#define dn200_mtl_dynamic_chan_set(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, mtl_dynamic_chan_set, __args)
#define dn200_config_cbs(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, config_cbs, __args)
#define dn200_dump_mac_regs(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, dump_regs, __args)
#define dn200_host_irq_status(__priv, __args...) \
	dn200_do_callback(__priv, mac, host_irq_status, __args)
#define dn200_host_mtl_irq_status(__priv, __args...) \
	dn200_do_callback(__priv, mac, host_mtl_irq_status, __args)
#define dn200_set_filter(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, set_filter, __args)
#define dn200_wq_set_filter(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, wq_set_filter, __args)
#define dn200_flow_ctrl(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, flow_ctrl, __args)
#define dn200_set_umac_addr(__priv, __args...) \
	dn200_do_callback(__priv, mac, set_umac_addr, __args)
#define dn200_wq_set_umac_addr(__priv, __args...) \
	dn200_do_callback(__priv, mac, wq_set_umac_addr, __args)
#define dn200_get_umac_addr(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, get_umac_addr, __args)
#define dn200_set_eee_mode(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, set_eee_mode, __args)
#define dn200_reset_eee_mode(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, reset_eee_mode, __args)
#define dn200_set_eee_lpi_timer(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, set_eee_lpi_entry_timer, __args)
#define dn200_set_eee_timer(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, set_eee_timer, __args)
#define dn200_set_eee_pls(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, set_eee_pls, __args)
#define dn200_mac_debug(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, debug, __args)
#define dn200_pcs_ctrl_ane(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, pcs_ctrl_ane, __args)
#define dn200_pcs_rane(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, pcs_rane, __args)
#define dn200_pcs_get_adv_lp(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, pcs_get_adv_lp, __args)
#define dn200_safety_feat_config(__priv, __args...) \
	dn200_do_callback(__priv, mac, safety_feat_config, __args)
#define dn200_safety_feat_irq_status(__priv, __args...) \
	dn200_do_callback(__priv, mac, safety_feat_irq_status, __args)
#define dn200_safety_feat_dump(__priv, __args...) \
	dn200_do_callback(__priv, mac, safety_feat_dump, __args)
#define dn200_rxp_config(__priv, __args...) \
	dn200_do_callback(__priv, mac, rxp_config, __args)
#define dn200_flex_pps_config(__priv, __args...) \
	dn200_do_callback(__priv, mac, flex_pps_config, __args)
#define dn200_set_mac_loopback(__priv, __args...) \
	dn200_do_callback(__priv, mac, set_mac_loopback, __args)
#define dn200_rss_configure(__priv, __args...) \
	dn200_do_callback(__priv, mac, rss_configure, __args)
#define dn200_update_vlan_hash(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, update_vlan_hash, __args)
#define dn200_enable_vlan(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, enable_vlan, __args)
#define dn200_add_hw_vlan_rx_fltr(__priv, __args...) \
	dn200_do_callback(__priv, mac, add_hw_vlan_rx_fltr, __args)
#define dn200_del_hw_vlan_rx_fltr(__priv, __args...) \
	dn200_do_callback(__priv, mac, del_hw_vlan_rx_fltr, __args)
#define dn200_restore_hw_vlan_rx_fltr(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, restore_hw_vlan_rx_fltr, __args)
#define dn200_get_mac_tx_timestamp(__priv, __args...) \
	dn200_do_callback(__priv, mac, get_mac_tx_timestamp, __args)
#define dn200_sarc_configure(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, sarc_configure, __args)
#define dn200_config_l3_filter(__priv, __args...) \
	dn200_do_callback(__priv, mac, config_l3_filter, __args)
#define dn200_config_l4_filter(__priv, __args...) \
	dn200_do_callback(__priv, mac, config_l4_filter, __args)
#define dn200_config_ntuple_filter(__priv, __args...) \
	dn200_do_callback(__priv, mac, config_ntuple_filter, __args)
#define dn200_l3_l4_filter_config(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, l3_l4_filter_config, __args)
#define dn200_set_arp_offload(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, set_arp_offload, __args)
#define dn200_est_configure(__priv, __args...) \
	dn200_do_callback(__priv, mac, est_configure, __args)
#define dn200_est_irq_status(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, est_irq_status, __args)
#define dn200_fpe_configure(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, fpe_configure, __args)
#define dn200_fpe_send_mpacket(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, fpe_send_mpacket, __args)
#define dn200_fpe_irq_status(__priv, __args...) \
	dn200_do_callback(__priv, mac, fpe_irq_status, __args)
#define dn200_rxp_broadcast(__priv, __args...) \
	dn200_do_callback(__priv, mac, rxp_broadcast, __args)
#define dn200_rxp_filter_get(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, rxp_filter_get, __args)
#define dn200_init_hw_vlan_rx_fltr(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, init_hw_vlan_rx_fltr, __args)
#define dn200_config_vlan_rx_fltr(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, config_vlan_rx_fltr, __args)
#define dn200_rx_vlan_stripping_config(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, rx_vlan_stripping_config, __args)
#define dn200_rxp_clear(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, rxp_clear, __args)
#define dn200_vf_del_rxp(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, vf_del_rxp, __args)
#define dn200_wq_vf_del_rxp(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, wq_vf_del_rxp, __args)
#define dn200_clear_vf_rxp(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, clear_vf_rxp, __args)
#define dn200_vf_append_rxp_bc(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, vf_append_rxp_bc, __args)
#define dn200_mtl_reset(__priv, __args...) \
	dn200_do_void_callback(__priv, mac, mtl_reset, __args)
#define dn200_tx_queue_flush(__priv, __args...) \
	dn200_do_callback(__priv, mac, tx_queue_flush, __args)
#define dn200_reset_rxp(__priv, __args...) \
	dn200_do_callback(__priv, mac, reset_rxp, __args)
#define dn200_rxf_and_acl_mem_reset(__priv, __args...) \
	dn200_do_callback(__priv, mac, rxf_and_acl_mem_reset, __args)
struct dn200_priv;

/* PTP and HW Timer helpers */
struct dn200_hwtimestamp {
	void (*config_hw_tstamping)(void __iomem *ioaddr, u32 data);
	void (*config_sub_second_increment)(void __iomem *ioaddr,
					    u32 ptp_clock, int gmac4,
					    u32 *ssinc);
	int (*init_systime)(void __iomem *ioaddr, u32 sec, u32 nsec);
	int (*config_addend)(void __iomem *ioaddr, u32 addend);
	int (*adjust_systime)(void __iomem *ioaddr, u32 sec, u32 nsec,
			      int add_sub, int gmac4);
	void (*get_systime)(void __iomem *ioaddr, u64 *systime);
	void (*get_ptptime)(void __iomem *ioaddr, u64 *ptp_time);
	void (*timestamp_interrupt)(struct dn200_priv *priv);
};

#define dn200_config_hw_tstamping(__priv, __args...) \
	dn200_do_void_callback(__priv, ptp, config_hw_tstamping, __args)
#define dn200_config_sub_second_increment(__priv, __args...) \
	dn200_do_void_callback(__priv, ptp, config_sub_second_increment, __args)
#define dn200_init_systime(__priv, __args...) \
	dn200_do_callback(__priv, ptp, init_systime, __args)
#define dn200_config_addend(__priv, __args...) \
	dn200_do_callback(__priv, ptp, config_addend, __args)
#define dn200_adjust_systime(__priv, __args...) \
	dn200_do_callback(__priv, ptp, adjust_systime, __args)
#define dn200_get_systime(__priv, __args...) \
	dn200_do_void_callback(__priv, ptp, get_systime, __args)
#define dn200_get_ptptime(__priv, __args...) \
	dn200_do_void_callback(__priv, ptp, get_ptptime, __args)
#define dn200_timestamp_interrupt(__priv, __args...) \
	dn200_do_void_callback(__priv, ptp, timestamp_interrupt, __args)

/* Helpers to manage the descriptors for chain and ring modes */
struct dn200_mode_ops {
	void (*init)(void *des, dma_addr_t phy_addr, unsigned int size);
	unsigned int (*is_jumbo_frm)(int len, int ehn_desc);
	int (*jumbo_frm)(void *priv, struct sk_buff *skb, int csum);
	int (*set_16kib_bfsize)(int mtu);
	void (*init_desc3)(struct dma_desc *p);
	void (*refill_desc3)(void *priv, struct dma_desc *p);
	void (*clean_desc3)(void *priv, struct dma_desc *p);
};

#define dn200_mode_init(__priv, __args...) \
	dn200_do_void_callback(__priv, mode, init, __args)
#define dn200_is_jumbo_frm(__priv, __args...) \
	dn200_do_callback(__priv, mode, is_jumbo_frm, __args)
#define dn200_jumbo_frm(__priv, __args...) \
	dn200_do_callback(__priv, mode, jumbo_frm, __args)
#define dn200_set_16kib_bfsize(__priv, __args...) \
	dn200_do_callback(__priv, mode, set_16kib_bfsize, __args)
#define dn200_init_desc3(__priv, __args...) \
	dn200_do_void_callback(__priv, mode, init_desc3, __args)
#define dn200_refill_desc3(__priv, __args...) \
	dn200_do_void_callback(__priv, mode, refill_desc3, __args)
#define dn200_clean_desc3(__priv, __args...) \
	dn200_do_void_callback(__priv, mode, clean_desc3, __args)

struct tc_cls_u32_offload;
struct tc_cbs_qopt_offload;
struct flow_cls_offload;
struct tc_taprio_qopt_offload;
struct tc_etf_qopt_offload;

struct dn200_tc_ops {
	int (*init)(struct dn200_priv *priv);
	int (*setup_cls_u32)(struct dn200_priv *priv,
			     struct tc_cls_u32_offload *cls);
	int (*setup_cbs)(struct dn200_priv *priv,
			 struct tc_cbs_qopt_offload *qopt);
	int (*setup_cls)(struct dn200_priv *priv,
			 struct flow_cls_offload *cls);
	int (*setup_taprio)(struct dn200_priv *priv,
			    struct tc_taprio_qopt_offload *qopt);
	int (*setup_etf)(struct dn200_priv *priv,
			 struct tc_etf_qopt_offload *qopt);
};

#define dn200_tc_init(__priv, __args...) \
	dn200_do_callback(__priv, tc, init, __args)
#define dn200_tc_setup_cls_u32(__priv, __args...) \
	dn200_do_callback(__priv, tc, setup_cls_u32, __args)
#define dn200_tc_setup_cbs(__priv, __args...) \
	dn200_do_callback(__priv, tc, setup_cbs, __args)
#define dn200_tc_setup_cls(__priv, __args...) \
	dn200_do_callback(__priv, tc, setup_cls, __args)
#define dn200_tc_setup_taprio(__priv, __args...) \
	dn200_do_callback(__priv, tc, setup_taprio, __args)
#define dn200_tc_setup_etf(__priv, __args...) \
	dn200_do_callback(__priv, tc, setup_etf, __args)

struct dn200_counters;

struct dn200_mmc_ops {
	void (*ctrl)(void __iomem *ioaddr, unsigned int mode);
	void (*intr_all_mask)(void __iomem *ioaddr);
	void (*read)(void __iomem *ioaddr, struct dn200_counters *mmc);
	void (*err_clear)(void __iomem *ioaddr);
};

#define dn200_mmc_ctrl(__priv, __args...) \
	dn200_do_void_callback(__priv, mmc, ctrl, __args)
#define dn200_mmc_intr_all_mask(__priv, __args...) \
	dn200_do_void_callback(__priv, mmc, intr_all_mask, __args)
#define dn200_mmc_read(__priv, __args...) \
	dn200_do_void_callback(__priv, mmc, read, __args)
#define dn200_mmc_err_clear(__priv, __args...) \
	dn200_do_void_callback(__priv, mmc, err_clear, __args)

struct dn200_regs_off {
	u32 ptp_off;
	u32 mmc_off;
};

extern const struct dn200_ops dwmac100_ops;
extern const struct dn200_dma_ops dwmac100_dma_ops;
extern const struct dn200_ops dwmac1000_ops;
extern const struct dn200_dma_ops dwmac1000_dma_ops;
extern const struct dn200_ops dwmac4_ops;
extern const struct dn200_dma_ops dwmac4_dma_ops;
extern const struct dn200_ops dwmac410_ops;
extern const struct dn200_dma_ops dwmac410_dma_ops;
extern const struct dn200_ops dwmac510_ops;
extern const struct dn200_ops dwxgmac210_ops;
extern const struct dn200_ops dwxgmac_sriov_ops;
extern const struct dn200_ops dwxgmac_purepf_ops;
extern const struct dn200_dma_ops dwxgmac_dma_ops;

extern const struct dn200_desc_ops dwxgmac210_desc_ops;
extern const struct dn200_mmc_ops dwmac_mmc_ops;
extern const struct dn200_mmc_ops dwxgmac_mmc_ops;

#define GMAC_VERSION		0x00000020	/* GMAC CORE Version */
#define GMAC4_VERSION		0x00000110	/* GMAC4+ CORE Version */

int dn200_hwif_init(struct dn200_priv *priv);
bool dn200_hwif_id_check(void __iomem *ioaddr);
bool dn200_dp_hwif_id_check(void __iomem *ioaddr);
#endif
