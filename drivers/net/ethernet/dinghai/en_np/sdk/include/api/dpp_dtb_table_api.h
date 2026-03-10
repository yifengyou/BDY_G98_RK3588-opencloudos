#ifndef _DPP_DTB_TABLE_API_H_
#define _DPP_DTB_TABLE_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dpp_dev.h"
#include "zxic_common.h"
#include "dpp_stat_api.h"

#define DPP_DTB_DUMP_ZCAM_TYPE ((ZXIC_UINT32)(0))
#define DPP_DTB_DUMP_DDR_TYPE ((ZXIC_UINT32)(1))

typedef struct dpp_dtb_user_entry_t {
	ZXIC_UINT32 sdt_no; /*流表sdt号*/
	ZXIC_VOID *p_entry_data; /*该流表的数据，数据结构见各表的结构体定义*/
} DPP_DTB_USER_ENTRY_T;

typedef struct dpp_dtb_eram_entry_info_t {
	ZXIC_UINT32
	index; /*条目index，根据wrt_mode模式为单位的的index，支持1/64/128bit */
	ZXIC_UINT32 *p_data; /*写入的表项信息 */
} DPP_DTB_ERAM_ENTRY_INFO_T;

typedef struct dpp_dtb_ddr_entry_info_t {
	ZXIC_UINT32
	index; /*条目index，根据wrt_mode模式定义的index，支持128/256/384/512bit*/
	ZXIC_UINT32 *p_data; /*写入的表项信息*/
} DPP_DTB_DDR_ENTRY_INFO_T;

typedef struct dpp_dtb_hash_entry_info_t {
	ZXIC_UINT8 *
		p_actu_key; /*实际的键值，对于一种表来说键值长度是固定的，在初始化时，会通过dpp_hash_tbl_id_info_init进行配置*/
	ZXIC_UINT8 *p_rst; /*hash表结果 result的长度由当前业务结果位宽决定*/
} DPP_DTB_HASH_ENTRY_INFO_T;

typedef struct dpp_dtb_acl_entry_info_t {
	ZXIC_UINT32 handle; /*条目索引*/
	ZXIC_UINT8 *
		key_data; /*键值data部分 按mode的长度，支持640bit/320bit/160bit/80bit*/
	ZXIC_UINT8 *key_mask; /*键值mask部分 长度与data相同*/
	ZXIC_UINT8
	*p_as_rslt; /*关联结果，仅使能关联查找情况有效 支持1/64/128bit*/
} DPP_DTB_ACL_ENTRY_INFO_T;

typedef struct dpp_dtb_dump_index_t {
	ZXIC_UINT32 index; /*index*/
	ZXIC_UINT32 index_type; /*index类型 */
} DPP_DTB_DUMP_INDEX_T;

typedef struct dtb_queue_dma_addr_info {
	ZXIC_UINT32 slot_id; /*np所在的槽位号*/
	ZXIC_UINT32 queue_id; /*队列号*/
	ZXIC_UINT32 dma_size; /*该队列申请的dma大小*/
	ZXIC_UINT64 dma_phy_addr; /* dma 物理地址*/
	ZXIC_UINT64 dma_vir_addr; /* dma 内核虚拟地址*/
} DTB_QUEUE_DMA_ADDR_INFO;

/*dump地址信息获取*/
ZXIC_UINT32 dpp_dtb_dump_sdt_addr_get(DPP_DEV_T *dev, ZXIC_UINT32 queue_id,
				      ZXIC_UINT32 sdt_no, ZXIC_UINT64 *phy_addr,
				      ZXIC_UINT64 *vir_addr, ZXIC_UINT32 *size);

/***********************************************************/
/** DTB通道申请
* @param   devId       NP设备号
* @param   pName       申请DTB通道的唯一设备名(最大32字符)
* @param   pQueueId    申请到的DTB通道编号
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/07/03
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_requst(DPP_DEV_T *dev, ZXIC_CONST ZXIC_UINT8 *pName,
				 ZXIC_UINT16 vPort, ZXIC_UINT32 *pQueueId);

/***********************************************************/
/** DTB通道队列申请(申请到的队列在软件维护和硬件维护上都处于初态)
* @param   devId       NP设备号
* @param   pName       申请DTB通道的唯一设备名(最大32字符)
* @param   pQueueId    申请到的DTB通道编号
* @return
* @remark  无
* @see
* @author  cq      @date  2025/06/05
************************************************************/
DPP_STATUS dpp_dtb_queue_requst_ex(DPP_DEV_T *dev, ZXIC_CONST ZXIC_UINT8 *pName,
				   ZXIC_UINT32 *p_queue_id);

/***********************************************************/
/** DTB通道释放
* @param   devId       NP设备号
* @param   pName       要释放DTB通道的唯一设备名(最大32字符)
* @param   pQueueId    要释放的DTB通道编号
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/07/03
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_release(DPP_DEV_T *dev, ZXIC_CONST ZXIC_UINT8 *pName,
				  ZXIC_UINT32 queueId);

/***********************************************************/
/** DTB通道释放(增加锁保护)
* @param   devId       NP设备号
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/07/03
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_release_ex(DPP_DEV_T *dev);

/***********************************************************/
/** 通知固件通道信息，保证固件与驱动配置一致
* @param   devId       NP设备号
* @param   pName       要释放DTB通道的唯一设备名(最大32字符)
* @param   vPort       端口号
* @param   pQueueId    同步队列
* @return
* @remark  无
* @see
* @author  cq      @date  2024/11/06
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_sync_cfg(DPP_DEV_T *dev, ZXIC_CONST ZXIC_UINT8 *pName,
				   ZXIC_UINT16 vPort, ZXIC_UINT32 queueId);

/***********************************************************/
/** DTB队列软件资源释放
* @param   dev       NP设备
* @return
* @remark  无
* @see
* @author  cq      @date  2025/06/06
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_release_soft(DPP_DEV_T *dev);

/***********************************************************/
/** DTB通道用户信息配置
* @param   devId      NP设备号
* @param   queueId    DTB通道编号
* @param   vPort      vport信息
* @param   vector     中断号
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/07/03
************************************************************/
ZXIC_UINT32 dpp_dtb_user_info_set(DPP_DEV_T *dev, ZXIC_UINT32 queueId,
				  ZXIC_UINT16 vPort, ZXIC_UINT32 vector);

/***********************************************************/
/** DTB通道下表空间地址设置,空间大小[32*(16+16*1024)B]
* @param   devId      NP设备号
* @param   queueId    DTB通道编号
* @param   phyAddr    物理地址
* @param   virAddr    虚拟地址
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/07/03
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_down_table_addr_set(DPP_DEV_T *dev,
					      ZXIC_UINT32 queueId,
					      ZXIC_UINT64 phyAddr,
					      ZXIC_UINT64 virAddr);

/***********************************************************/
/** DTB通道dump空间地址设置,空间大小[32*(16+16*1024)B]
* @param   devId      NP设备号
* @param   pName      要释放DTB通道的设备名
* @param   phyAddr    物理地址
* @param   virAddr    虚拟地址
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/07/03
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_dump_table_addr_set(DPP_DEV_T *dev,
					      ZXIC_UINT32 queueId,
					      ZXIC_UINT64 phyAddr,
					      ZXIC_UINT64 virAddr);

/***********************************************************/
/** 大批量dump一个流表使用的地址空间配置
* @param   devId       NP设备号
* @param   queueId     DTB队列编号
* @param   sdtNo       流表std号
* @param   phyAddr     物理地址
* @param   virAddr     虚拟地址
* @param   size        (最大64MB)
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/07/03
************************************************************/
ZXIC_UINT32 dpp_dtb_dump_sdt_addr_set(DPP_DEV_T *dev, ZXIC_UINT32 queueId,
				      ZXIC_UINT32 sdtNo, ZXIC_UINT64 phyAddr,
				      ZXIC_UINT64 virAddr, ZXIC_UINT32 size);

/***********************************************************/
/** 清除大批量dump一个流表使用的地址空间配置
* @param   devId        NP设备号
* @param   queueId      DTB队列编号
* @param   sdtNo        流表std号
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/07/03
************************************************************/
ZXIC_UINT32 dpp_dtb_dump_sdt_addr_clear(DPP_DEV_T *dev, ZXIC_UINT32 queueId,
					ZXIC_UINT32 sdtNo);

/***********************************************************/
/** 释放当前sdt下的所有流表(硬件方式)
* (适用于进程启动后，仅配置流表资源，软件未配置流表，但需要删除硬件上已配置流表的场景)
* @param   dev_id  设备号
* @param   queue_id  队列号
* @param   sdt_no  sdt号
* @return
* @remark  无
* @see
* @author  cq      @date  2023/12/04
************************************************************/
DPP_STATUS dpp_dtb_hash_offline_delete(DPP_DEV_T *dev, ZXIC_UINT32 queue_id,
				       ZXIC_UINT32 sdt_no);

/***********************************************************/
/** 释放当前sdt下的所有流表(硬件方式)
* (适用于进程正常退出前删除表项，软件上有存储表项)
* @param   dev_id  设备号
* @param   queue_id  队列号
* @param   sdt_no  sdt号
* @return
* @remark  无
* @see
* @author  cq      @date  2023/12/04
************************************************************/
DPP_STATUS dpp_dtb_hash_online_delete(DPP_DEV_T *dev, ZXIC_UINT32 queue_id,
				      ZXIC_UINT32 sdt_no);

/***********************************************************/
/** acl index资源申请
* @param   dev             NP设备
* @param   sdt_no          流表sdt号(0~255)
* @param   vport           端口号
* @param   p_index         申请到的索引值，acl下表时使用
* @return
* @remark  无
* @see
* @author  cq      @date  2024/09/14
************************************************************/
DPP_STATUS dpp_dtb_acl_index_request(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no,
				     ZXIC_UINT32 vport, ZXIC_UINT32 *p_index);

/***********************************************************/
/** acl index资源释放
* @param   dev             NP设备
* @param   sdt_no          流表sdt号(0~255)
* @param   vport           端口号
* @param   index           需要释放的索引值
* @return
* @remark  无
* @see
* @author  cq      @date  2024/09/14
************************************************************/
DPP_STATUS dpp_dtb_acl_index_release(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no,
				     ZXIC_UINT32 vport, ZXIC_UINT32 index);

/***********************************************************/
/** 离线删除与vport关联的acl表项和索引值
* @param   dev             NP设备
* @param   queue_id        dtb通道队列号(0~127)
* @param   sdt_no          流表sdt号(0~255)
* @param   vport           端口号
* @param   counter_id      统计编号，对应微码中的address
* @param   rd_mode         统计读取方式 0:64bit 1:128bit
* @return
* @remark  无
* @see
* @author  cq      @date  2024/09/14
************************************************************/
DPP_STATUS dpp_dtb_acl_offline_delete(DPP_DEV_T *dev, ZXIC_UINT32 queue_id,
				      ZXIC_UINT32 sdt_no, ZXIC_UINT32 vport,
				      ZXIC_UINT32 counter_id,
				      ZXIC_UINT32 rd_mode);

/***********************************************************/
/** 统计计数读清
* @param   dev              NP设备
* @param   queue_id         队列号
* @param   rd_mode          读取位宽模式，参见STAT_CNT_MODE_E，0-64bit，1-128bit
* @param   start_count_id   统计起始编号，对应微码中的address
* @param   num              统计项个数
* @return
* @remark  无
* @see
* @author  cq      @date  2024/09/14
************************************************************/
DPP_STATUS dpp_dtb_stat_ppu_cnt_clr(DPP_DEV_T *dev, ZXIC_UINT32 queue_id,
				    STAT_CNT_MODE_E rd_mode,
				    ZXIC_UINT32 start_count_id,
				    ZXIC_UINT32 num);

/***********************************************************/
/** 清除指定vport的对应的stat统计(dtb方式)
* @param   dev             NP设备
* @param   queue_id        队列号
* @param   sdt_no          流表sdt号(0~255)
* @param   vport           端口号
* @param   rd_mode         读取位宽模式，参见STAT_CNT_MODE_E，0-64bit，1-128bit
* @param   start_counter_id 统计起始编号，对应微码中的address
* @return
* @remark  无
* @see
* @author  cq      @date  2024/09/14
************************************************************/
DPP_STATUS dpp_dtb_acl_stat_clr_by_vport(DPP_DEV_T *dev, ZXIC_UINT32 queue_id,
					 ZXIC_UINT32 sdt_no, ZXIC_UINT32 vport,
					 STAT_CNT_MODE_E rd_mode,
					 ZXIC_UINT32 start_counter_id);

/***********************************************************/
/** 消息通道获取pcie bar消息数目
* @param   dev               NP设备
* @param   p_bar_msg_num     出参，获取pcie bar数目
* @return
* @remark  无
* @see
* @author  cq      @date  2024/11/16
************************************************************/
ZXIC_UINT32 dpp_pcie_bar_msg_num_get(DPP_DEV_T *dev,
				     ZXIC_UINT32 *p_bar_msg_num);

#ifdef __cplusplus
}
#endif

#endif