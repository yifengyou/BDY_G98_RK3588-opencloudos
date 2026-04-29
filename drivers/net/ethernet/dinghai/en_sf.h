#ifndef __ZXDH_PF_EN_SF_H__
#define __ZXDH_PF_EN_SF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/dinghai/driver.h>
#include <linux/dinghai/zxdh_auxiliary_bus.h>
#include <linux/dinghai/en_sf.h>
#include <linux/types.h>
#include <linux/dinghai/queue.h>
#include <linux/dinghai/pci_irq.h>

#define ZXDH_MAJOR_VER 10
#define ZXDH_MINOR_VER 1
#define ZXDH_NET_MAJOR_VER 0 //0-255, bit0-7
#define ZXDH_NET_MINOR_VER 0 //0-255, bit8-15
#define ZXDH_RDMA_MINOR_VER 0 //0-255, bit16-23
#define ZXDH_HIGH_8BIT 8
#define ZXDH_HIGH_16BIT 16
#define ZXDH_SF_ADEV_NUM 32

struct zxdh_rdma_dev_info;

enum AUX_DEVICE_TYPE {
	NET_AUX_DEVICE,
	RDMA_AUX_DEVICE,
	SEC_AUX_DEVICE,
};

struct zxdh_adev_handle_table {
	enum AUX_DEVICE_TYPE adev_type;
	int32_t (*cb_fn)(struct dh_core_dev *dh_dev,
			 struct zxdh_auxiliary_device *adev);
};

struct zxdh_ver_info {
	uint16_t major;
	uint16_t minor;
	uint64_t support;
};

enum zxdh_function_type {
	ZXDH_FUNCTION_TYPE_PF,
	ZXDH_FUNCTION_TYPE_VF,
};

enum zxdh_rdma_protocol {
	ZXDH_RDMA_PROTOCOL_IWARP = BIT(0),
	ZXDH_RDMA_PROTOCOL_ROCEV2 = BIT(1),
};

struct zxdh_rdma_qos_params {
	uint8_t reserve;
};

enum zxdh_rdma_reset_type {
	ZXDH_RESET_MTU_CHANGE,
	ZXDH_RESET_HW_ERROR,
};

struct zxdh_rdma_dev_ops {
	int32_t (*request_reset)(struct zxdh_rdma_dev_info *rdma_infos,
				 enum zxdh_rdma_reset_type reset_type);
};

/* auxiliary driver tailored information about the core PCI dev */
struct zxdh_rdma_dev_info {
	struct pci_dev *pdev;
	struct zxdh_auxiliary_device *adev;

	uint8_t __iomem *hw_addr;
	int32_t adev_info_id;
	struct zxdh_ver_info ver;

	void *auxiliary_priv;

	enum zxdh_function_type ftype;
	uint16_t vport_id;
	uint16_t slot_id;
	/* Current active RDMA protocol */
	enum zxdh_rdma_protocol rdma_protocol;

	struct zxdh_rdma_qos_params qos_info;

	struct msix_entry msix_entries;
	/* How many vectors are reserved for this device */
	uint16_t msix_count;
	/* function pointers to be initialized by core PCI driver and called by auxiliary driver */
	struct zxdh_rdma_dev_ops *ops;
};

struct zxdh_en_sf_device {
	int32_t max_channels;
	struct zxdh_en_sf_if *sf_ops;
	void *netdev;
	void *sec_info;

	struct zxdh_auxiliary_device *adev[ZXDH_SF_ADEV_NUM];
	int32_t aux_idx;
};

#ifdef __cplusplus
}
#endif

#endif