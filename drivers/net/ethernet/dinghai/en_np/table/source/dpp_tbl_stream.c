#include "dpp_drv_init.h"
#include "dpp_drv_eram.h"
#include "dpp_drv_sdt.h"
#include "dpp_dev.h"
#include "dpp_dtb.h"
#include "dpp_dtb_table.h"
#include "dpp_tbl_api.h"

ZXIC_UINT32 dpp_eram_entry_insert(DPP_PF_INFO_T *pf_info, ZXIC_UINT32 sdt_no,
				  ZXIC_UINT32 index, ZXIC_UINT8 *p_data)
{
	DPP_DEV_T dev = { 0 };
	DPP_DTB_ERAM_ENTRY_INFO_T eram_entry = { 0 };

	ZXIC_UINT32 queue = 0;
	ZXIC_UINT32 element_id = 0;
	ZXIC_UINT32 rc = DPP_OK;

	ZXIC_COMM_CHECK_POINT(pf_info);
	ZXIC_COMM_CHECK_POINT(p_data);

	ZXIC_COMM_MEMSET_S(&eram_entry, sizeof(DPP_DTB_ERAM_ENTRY_INFO_T), 0,
			   sizeof(DPP_DTB_ERAM_ENTRY_INFO_T));
	eram_entry.index = index;
	eram_entry.p_data = (ZXIC_UINT32 *)p_data;

	rc = dpp_dev_get(pf_info, &dev);
	ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

	rc = dpp_dtb_queue_id_get(&dev, &queue);
	ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

	rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
	ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
	ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

	rc = dpp_dtb_eram_dma_write(&dev, queue, sdt_no, 1, &eram_entry,
				    &element_id);
	ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_dma_insert",
				  DEV_PCIE_LOCK(&dev));

	rc = dpp_vport_table_unlock(pf_info, sdt_no);
	ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

	return DPP_OK;
}
EXPORT_SYMBOL(dpp_eram_entry_insert);

ZXIC_UINT32 dpp_eram_entry_delete(DPP_PF_INFO_T *pf_info, ZXIC_UINT32 sdt_no,
				  ZXIC_UINT32 index)
{
	DPP_DEV_T dev = { 0 };
	ZXIC_UINT8 data[DPP_SMMU0_READ_REG_MAX_NUM * 4] = { 0 };
	DPP_DTB_ERAM_ENTRY_INFO_T eram_entry = { 0 };

	ZXIC_UINT32 queue = 0;
	ZXIC_UINT32 element_id = 0;
	ZXIC_UINT32 rc = DPP_OK;

	ZXIC_COMM_CHECK_POINT(pf_info);

	ZXIC_COMM_MEMSET_S(&eram_entry, sizeof(DPP_DTB_ERAM_ENTRY_INFO_T), 0,
			   sizeof(DPP_DTB_ERAM_ENTRY_INFO_T));
	ZXIC_COMM_MEMSET_S(data, sizeof(data), 0, sizeof(data));
	eram_entry.index = index;
	eram_entry.p_data = (ZXIC_UINT32 *)data;

	rc = dpp_dev_get(pf_info, &dev);
	ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

	rc = dpp_dtb_queue_id_get(&dev, &queue);
	ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

	rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
	ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
	ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

	rc = dpp_dtb_eram_dma_write(&dev, queue, sdt_no, 1, &eram_entry,
				    &element_id);
	ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_dma_insert",
				  DEV_PCIE_LOCK(&dev));

	rc = dpp_vport_table_unlock(pf_info, sdt_no);
	ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

	return DPP_OK;
}
EXPORT_SYMBOL(dpp_eram_entry_delete);

ZXIC_UINT32 dpp_eram_entry_get(DPP_PF_INFO_T *pf_info, ZXIC_UINT32 sdt_no,
			       ZXIC_UINT32 index, ZXIC_UINT8 *p_data)
{
	DPP_DEV_T dev = { 0 };
	ZXIC_UINT32 queue = 0;
	ZXIC_UINT32 rc = DPP_OK;
	DPP_DTB_ERAM_ENTRY_INFO_T eram_entry = { 0 };

	ZXIC_COMM_CHECK_POINT(pf_info);
	ZXIC_COMM_CHECK_POINT(p_data);

	ZXIC_COMM_MEMSET_S(&eram_entry, sizeof(DPP_DTB_ERAM_ENTRY_INFO_T), 0,
			   sizeof(DPP_DTB_ERAM_ENTRY_INFO_T));
	eram_entry.index = index;
	eram_entry.p_data = (ZXIC_UINT32 *)p_data;

	rc = dpp_dev_get(pf_info, &dev);
	ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

	rc = dpp_dtb_queue_id_get(&dev, &queue);
	ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

	rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
	ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
	ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

	rc = dpp_dtb_eram_data_get(&dev, queue, sdt_no, &eram_entry);
	ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_dma_insert",
				  DEV_PCIE_LOCK(&dev));

	rc = dpp_vport_table_unlock(pf_info, sdt_no);
	ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

	return DPP_OK;
}
EXPORT_SYMBOL(dpp_eram_entry_get);
