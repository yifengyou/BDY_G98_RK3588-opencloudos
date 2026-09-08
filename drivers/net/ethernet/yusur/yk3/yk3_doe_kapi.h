/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_DOE_KAPI_H
#define _YK3_DOE_KAPI_H
#include "yk3_base.h"
#include "yk3_doe_errno.h"
/******************************************************************************/
#define YK3_DOE_CALL_MASK   0x7 // low 3 bit is yk3_doe_call_mode
#define YK3_DOE_USER_CALL   0x80 // user call mask it
#define YK3_DOE_CALL_MODE(call_mode)	((call_mode) & YK3_DOE_CALL_MASK)

#define IS_YK3_DOE_KERNEL_CALL(call_mode) (!((call_mode) & YK3_DOE_USER_CALL))
#define IS_YK3_DOE_USER_CALL(call_mode)   (!IS_YK3_DOE_KERNEL_CALL(call_mode))

#define IS_YK3_DOE_CALL(call_mode, call)	(YK3_DOE_CALL_MODE(call_mode) == (call))
#define IS_YK3_DOE_SYNC_CALL(call_mode)		IS_YK3_DOE_CALL(call_mode, YK3_DOE_SYNC_CALL)
#define IS_YK3_DOE_FAST_CALL(call_mode)		IS_YK3_DOE_CALL(call_mode, YK3_DOE_FAST_CALL)
#define IS_YK3_DOE_ASYNC_CALL(call_mode)	IS_YK3_DOE_CALL(call_mode, YK3_DOE_ASYNC_CALL)

#define IS_YK3_DOE_WAIT_SLEEP(call_mode, flags) \
	(IS_YK3_DOE_SYNC_CALL(call_mode) && ((flags) & YK3_DOE_F_WAIT_SLEEP))
/******************************************************************************/
int yk3_doe_create_tbl(struct yk3_pdev_priv *pdev_priv, struct yk3_tbl_cfg *param);
int yk3_doe_delete_tbl(struct yk3_pdev_priv *pdev_priv, int tbl_id);

int yk3_doe_entry_add(struct yk3_pdev_priv *pdev_priv,
		      struct yk3_tbl_entry *param, u8 call_mode);

int yk3_doe_entry_delete(struct yk3_pdev_priv *pdev_priv,
			 struct yk3_tbl_entry *param, u8 call_mode);

int yk3_doe_entry_update(struct yk3_pdev_priv *pdev_priv,
			 struct yk3_tbl_entry *param, u8 call_mode);

int yk3_doe_entry_query(struct yk3_pdev_priv *pdev_priv,
			struct yk3_tbl_entry *param, u8 call_mode);

// YK3_DOE_TLB_EXISTED
//	 1: exist
//	 0: NOT exist
//	<0: error
// YK3_DOE_GET_PROTECT
//	 1: protect open
//	 0: protect close
//	<0: error
// YK3_DOE_GET_CACHE_INFO
//	=0: ok
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
// YK3_DOE_GET_MEASURE
//	 =0: ok
//	 <0: error
int yk3_doe_ctl_get(struct yk3_pdev_priv *pdev_priv, struct yk3_tbl_ctl *param);

// return
//	 0: ok
//	<0: error
int yk3_doe_ctl_set(struct yk3_pdev_priv *pdev_priv, struct yk3_tbl_ctl *param);
/******************************************************************************/
#endif /* _YK3_DOE_KAPI_H */
