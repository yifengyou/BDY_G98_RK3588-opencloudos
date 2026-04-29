/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_pktrx_cfg.c
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : zzh
* 完成日期 : 2015/02/06
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/

#include "zxic_common.h"
#include "dpp_type_api.h"
#include "dpp_reg.h"
#include "dpp_pktrx_api.h"
#include "dpp_dev.h"
#include "dpp_agent_channel.h"

#if ZXIC_REAL("mcode glb cfg ")

ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_set_0(DPP_DEV_T *dev,
					  ZXIC_UINT32 glb_cfg_data_0)
{
	ZXIC_UINT32 rc = 0;

	rc = dpp_reg_write(dev, NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_0r, 0, 0,
			   &glb_cfg_data_0);
	ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

	return DPP_OK;
}

ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_set_1(DPP_DEV_T *dev,
					  ZXIC_UINT32 glb_cfg_data_1)
{
	ZXIC_UINT32 rc = 0;

	rc = dpp_reg_write(dev, NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_1r, 0, 0,
			   &glb_cfg_data_1);
	ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

	return DPP_OK;
}

ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_set_2(DPP_DEV_T *dev,
					  ZXIC_UINT32 glb_cfg_data_2)
{
	ZXIC_UINT32 rc = 0;

	rc = dpp_reg_write(dev, NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_2r, 0, 0,
			   &glb_cfg_data_2);
	ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

	return DPP_OK;
}

ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_set_3(DPP_DEV_T *dev,
					  ZXIC_UINT32 glb_cfg_data_3)
{
	ZXIC_UINT32 rc = 0;

	rc = dpp_reg_write(dev, NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_3r, 0, 0,
			   &glb_cfg_data_3);
	ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

	return DPP_OK;
}
/***********************************************************/
/**
* @param   dev_id
* @param   p_mcode_glb_cfg
*
* @return
* @remark  无
* @see
* @author  czd      @date  2016/04/27
************************************************************/
ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_get_0(DPP_DEV_T *dev,
					  ZXIC_UINT32 *p_glb_cfg_data_0)
{
	ZXIC_UINT32 rc = 0;

	ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_glb_cfg_data_0);

	rc = dpp_reg_read(dev, NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_0r, 0, 0,
			  p_glb_cfg_data_0);
	ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

	return DPP_OK;
}

ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_get_1(DPP_DEV_T *dev,
					  ZXIC_UINT32 *p_glb_cfg_data_1)
{
	ZXIC_UINT32 rc = 0;

	ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_glb_cfg_data_1);

	rc = dpp_reg_read(dev, NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_1r, 0, 0,
			  p_glb_cfg_data_1);
	ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

	return DPP_OK;
}

ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_get_2(DPP_DEV_T *dev,
					  ZXIC_UINT32 *p_glb_cfg_data_2)
{
	ZXIC_UINT32 rc = 0;

	ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_glb_cfg_data_2);

	rc = dpp_reg_read(dev, NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_2r, 0, 0,
			  p_glb_cfg_data_2);
	ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

	return DPP_OK;
}

ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_get_3(DPP_DEV_T *dev,
					  ZXIC_UINT32 *p_glb_cfg_data_3)
{
	ZXIC_UINT32 rc = 0;

	ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_glb_cfg_data_3);

	rc = dpp_reg_read(dev, NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_3r, 0, 0,
			  p_glb_cfg_data_3);
	ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

	return DPP_OK;
}

ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_write_0(DPP_DEV_T *dev,
					    ZXIC_UINT32 start_bit_no,
					    ZXIC_UINT32 end_bit_no,
					    ZXIC_UINT32 glb_cfg_data_0)
{
	ZXIC_UINT32 rc = 0;
	ZXIC_UINT32 data = 0;
	ZXIC_MUTEX_T *p_mutex = NULL;

	ZXIC_COMM_CHECK_POINT(dev);
	ZXIC_COMM_CHECK_INDEX(start_bit_no, 0, 31);
	ZXIC_COMM_CHECK_INDEX(end_bit_no, start_bit_no, 31);

	rc = dpp_dev_opr_mutex_get(dev, DPP_DEV_MUTEX_T_PKTRX_MF_GLB_CFG_0,
				   &p_mutex);
	ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dev_opr_mutex_get");

	rc = zxic_comm_mutex_lock(p_mutex);
	ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_lock");

	rc = dpp_reg_read(dev, NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_0r, 0, 0, &data);
	ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_reg_read", p_mutex);

	ZXIC_COMM_UINT32_WRITE_BITS(data, glb_cfg_data_0, start_bit_no,
				    end_bit_no - start_bit_no + 1);

	rc = dpp_reg_write(dev, NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_0r, 0, 0, &data);
	ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_reg_write",
				      p_mutex);

	rc = zxic_comm_mutex_unlock(p_mutex);
	ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");

	return DPP_OK;
}

ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_write_1(DPP_DEV_T *dev,
					    ZXIC_UINT32 start_bit_no,
					    ZXIC_UINT32 end_bit_no,
					    ZXIC_UINT32 glb_cfg_data_1)
{
	ZXIC_UINT32 rc = 0;
	ZXIC_UINT32 data = 0;
	ZXIC_MUTEX_T *p_mutex = NULL;

	ZXIC_COMM_CHECK_POINT(dev);
	ZXIC_COMM_CHECK_INDEX(start_bit_no, 0, 31);
	ZXIC_COMM_CHECK_INDEX(end_bit_no, start_bit_no, 31);

	rc = dpp_dev_opr_mutex_get(dev, DPP_DEV_MUTEX_T_PKTRX_MF_GLB_CFG_1,
				   &p_mutex);
	ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dev_opr_mutex_get");

	rc = zxic_comm_mutex_lock(p_mutex);
	ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_lock");

	rc = dpp_reg_read(dev, NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_1r, 0, 0, &data);
	ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_reg_read", p_mutex);

	ZXIC_COMM_UINT32_WRITE_BITS(data, glb_cfg_data_1, start_bit_no,
				    end_bit_no - start_bit_no + 1);

	rc = dpp_reg_write(dev, NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_1r, 0, 0, &data);
	ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_reg_write",
				      p_mutex);

	rc = zxic_comm_mutex_unlock(p_mutex);
	ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");

	return DPP_OK;
}

DPP_STATUS dpp_pktrx_ind_rd(DPP_DEV_T *dev, ZXIC_UINT32 mem_addr,
			    ZXIC_UINT32 mem_id, ZXIC_UINT32 len,
			    ZXIC_UINT32 *p_data)
{
	DPP_STATUS rtn = DPP_OK;

	ZXIC_COMM_CHECK_POINT(dev);
	ZXIC_COMM_CHECK_POINT(p_data);
	ZXIC_COMM_CHECK_INDEX(mem_id, 0, (MEM_ID_MUX_NUM - 1));

	ZXIC_COMM_TRACE_NOTICE("dpp_pktrx_ind_rd start\n");

	rtn = dpp_agent_channel_pktrx_ind_reg_rw(
		dev, mem_addr, mem_id, DPP_PKTRX_IND_REG_RD, len, p_data);
	ZXIC_COMM_CHECK_RC(rtn, "dpp_agent_channel_pktrx_ind_reg_rw");

	ZXIC_COMM_PRINT("dpp_pktrx_ind_rd success\n");

	return DPP_OK;
}

DPP_STATUS
dpp_pktrx_udf_table_get(DPP_DEV_T *dev, ZXIC_UINT32 index,
			DPP_PKTRX_PHYPORT_UDF_TABLE_T *p_phyport_user_info)
{
	DPP_STATUS rc = 0;

	ZXIC_COMM_CHECK_INDEX(index, 0, (DPP_PHYPORT_NUM - 1));
	ZXIC_COMM_CHECK_POINT(dev);
	ZXIC_COMM_CHECK_POINT(p_phyport_user_info);

	/* read phyport_udf_tbl item */
	rc = dpp_pktrx_ind_rd(dev, index, PHYPORT_TAB_2_MEM_ID, 16,
			      &(p_phyport_user_info->port_based_user_data[0]));
	ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_ind_rd");

	return DPP_OK;
}

#endif