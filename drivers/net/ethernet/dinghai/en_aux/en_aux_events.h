#ifndef __EN_AUX_EVENTS_H__
#define __EN_AUX_EVENTS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/dinghai/driver.h>
#include <net/ip.h>
#include <net/vxlan.h>
#include <linux/ip.h>
#include "en_aux.h"
#include "../en_np/table/include/dpp_tbl_comm.h"

#define MULTI_FLAG (0x01)
#define IPV4_TYPE_FLAG (0x00)
#define GLOBAL_FLAG (0x5E)
#define BIT16 (16)
#define BIT8 (8)
#define BIT_23_L (0x7F)
#define BIT_15_L (0xFF)
#define BIT_7_L (0xFF)

enum {
	ZXDH_RDMA_HEALTH_EVENT = 1,
	ZXDH_RDMA_SRIOV_EVENT = 2,
};

struct zxdh_rdma_sriov_event_info {
	struct pci_dev *pdev;
	uint64_t bar0_virt_addr;
	uint16_t vport_id;
	uint16_t num_vfs;
};

int32_t dh_aux_events_init(struct zxdh_en_priv *en_priv);
void dh_aux_events_uninit(struct zxdh_en_priv *en_priv);
int32_t dh_aux_msg_recv_func_register(void);
void dh_aux_msg_recv_func_unregister(void);
int32_t dh_aux_ipv6_notifier_init(struct zxdh_en_priv *en_priv);
int32_t dh_aux_vxlan_netdev_notifier_init(struct zxdh_en_priv *en_priv);
int32_t dh_ip_mac_init(struct zxdh_en_priv *en_priv);
int32_t zxdh_rdma_events_call(struct net_device *netdev, uint8_t event_type,
			      void *data);
void zxdh_cap_pkt_uninit(struct zxdh_en_device *en_dev, bool offload_mode);
#ifdef __cplusplus
}
#endif

#endif
