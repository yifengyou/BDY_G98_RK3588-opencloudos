// SPDX-License-Identifier: BSD-3-Clause
/* Copyright(c) 2014-2025 Broadcom
 * All rights reserved.
 */

#include "ulp_template_db_enum.h"
#include "ulp_template_db_field.h"
#include "ulp_template_struct.h"
#include "ulp_template_db_tbl.h"

/* Mapper templates for header class list */
struct bnxt_ulp_mapper_tmpl_info ulp_thor2_class_tmpl_list[] = {
	/* class_tid: 1, ingress */
	[1] = {
	.device_name = BNXT_ULP_DEVICE_ID_THOR2,
	.num_tbls = 50,
	.start_tbl_idx = 0,
	.reject_info = {
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 0,
		.cond_nums = 3 }
	},
	/* class_tid: 2, egress */
	[2] = {
	.device_name = BNXT_ULP_DEVICE_ID_THOR2,
	.num_tbls = 25,
	.start_tbl_idx = 50,
	.reject_info = {
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 286,
		.cond_nums = 2 }
	},
	/* class_tid: 3, egress */
	[3] = {
	.device_name = BNXT_ULP_DEVICE_ID_THOR2,
	.num_tbls = 1,
	.start_tbl_idx = 75,
	.reject_info = {
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_FALSE,
		.cond_start_idx = 430,
		.cond_nums = 0 }
	},
	/* class_tid: 4, ingress */
	[4] = {
	.device_name = BNXT_ULP_DEVICE_ID_THOR2,
	.num_tbls = 35,
	.start_tbl_idx = 76,
	.reject_info = {
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_FALSE,
		.cond_start_idx = 430,
		.cond_nums = 0 }
	},
	/* class_tid: 5, ingress */
	[5] = {
	.device_name = BNXT_ULP_DEVICE_ID_THOR2,
	.num_tbls = 6,
	.start_tbl_idx = 111,
	.reject_info = {
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_FALSE,
		.cond_start_idx = 433,
		.cond_nums = 0 }
	},
	/* class_tid: 6, egress */
	[6] = {
	.device_name = BNXT_ULP_DEVICE_ID_THOR2,
	.num_tbls = 6,
	.start_tbl_idx = 117,
	.reject_info = {
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_FALSE,
		.cond_start_idx = 434,
		.cond_nums = 0 }
	}
};

struct bnxt_ulp_mapper_tbl_info ulp_thor2_class_tbl_list[] = {
	{ /* class_tid: 1, , table: port_table.rd */
	.description = "port_table.rd",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_PORT_TABLE,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 3,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_READ,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_INDEX,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.key_start_idx = 0,
	.blob_key_bit_size = 10,
	.key_bit_size = 10,
	.key_num_fields = 1,
	.ident_start_idx = 0,
	.ident_nums = 3
	},
	{ /* class_tid: 1, , table: mac_addr_cache.rd */
	.description = "mac_addr_cache.rd",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_MAC_ADDR_CACHE,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 3,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_READ,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_HASH,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.key_start_idx = 1,
	.blob_key_bit_size = 195,
	.key_bit_size = 195,
	.key_num_fields = 10,
	.ident_start_idx = 3,
	.ident_nums = 3
	},
	{ /* class_tid: 1, , table: control.mac_addr_cache_chk */
	.description = "control.mac_addr_cache_chk",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 8,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 3,
		.cond_nums = 1 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_ALLOC_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID
	},
	{ /* class_tid: 1, , table: l2_cntxt_tcam.l2_table_create */
	.description = "l2_cntxt_tcam.l2_table_create",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_TCAM_TABLE,
	.resource_type = CFA_RSUBTYPE_TCAM_L2CTX,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 4,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_TCAM_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_L2_CNTXT_TCAM_INDEX_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.pri_opcode  = BNXT_ULP_PRI_OPC_CONST,
	.pri_operand = ULP_THOR2_SYM_L2_CTXT_PRI_APP,
	.track_type = CFA_TRACK_TYPE_SID,
	.key_start_idx = 11,
	.blob_key_bit_size = 256,
	.key_bit_size = 256,
	.key_num_fields = 24,
	.result_start_idx = 0,
	.result_bit_size = 127,
	.result_num_fields = 17,
	.ident_start_idx = 6,
	.ident_nums = 2
	},
	{ /* class_tid: 1, , table: l2_cntxt_tcam.roce_l2_table_create */
	.description = "l2_cntxt_tcam.roce_l2_table_create",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_TCAM_TABLE,
	.resource_type = CFA_RSUBTYPE_TCAM_L2CTX,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 4,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_TCAM_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_L2_CNTXT_TCAM_INDEX_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.pri_opcode  = BNXT_ULP_PRI_OPC_CONST,
	.pri_operand = ULP_THOR2_SYM_L2_CTXT_PRI_APP,
	.track_type = CFA_TRACK_TYPE_SID,
	.key_start_idx = 35,
	.blob_key_bit_size = 256,
	.key_bit_size = 256,
	.key_num_fields = 24,
	.result_start_idx = 17,
	.result_bit_size = 127,
	.result_num_fields = 17,
	.ident_start_idx = 8,
	.ident_nums = 0
	},
	{ /* class_tid: 1, , table: l2_cntxt_tcam.roce_cnp_l2_table_create */
	.description = "l2_cntxt_tcam.roce_cnp_l2_table_create",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_TCAM_TABLE,
	.resource_type = CFA_RSUBTYPE_TCAM_L2CTX,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 4,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_TCAM_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_L2_CNTXT_TCAM_INDEX_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.pri_opcode  = BNXT_ULP_PRI_OPC_CONST,
	.pri_operand = ULP_THOR2_SYM_L2_CTXT_PRI_APP,
	.track_type = CFA_TRACK_TYPE_SID,
	.key_start_idx = 59,
	.blob_key_bit_size = 256,
	.key_bit_size = 256,
	.key_num_fields = 24,
	.result_start_idx = 34,
	.result_bit_size = 127,
	.result_num_fields = 17,
	.ident_start_idx = 8,
	.ident_nums = 1
	},
	{ /* class_tid: 1, , table: l2_cntxt_tcam.roce_l2_table_create_lpbk */
	.description = "l2_cntxt_tcam.roce_l2_table_create_lpbk",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_TCAM_TABLE,
	.resource_type = CFA_RSUBTYPE_TCAM_L2CTX,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 4,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_TCAM_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_L2_CNTXT_TCAM_INDEX_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.pri_opcode  = BNXT_ULP_PRI_OPC_CONST,
	.pri_operand = ULP_THOR2_SYM_L2_CTXT_PRI_APP,
	.track_type = CFA_TRACK_TYPE_SID,
	.key_start_idx = 83,
	.blob_key_bit_size = 256,
	.key_bit_size = 256,
	.key_num_fields = 24,
	.result_start_idx = 51,
	.result_bit_size = 127,
	.result_num_fields = 17,
	.ident_start_idx = 9,
	.ident_nums = 0
	},
	{ /* class_tid: 1, , table: profile_tcam.catchall */
	.description = "profile_tcam.catchall",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_TCAM_TABLE,
	.resource_type = CFA_RSUBTYPE_TCAM_PROF_TCAM,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 4,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_TCAM_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_PROFILE_TCAM_INDEX_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.pri_opcode  = BNXT_ULP_PRI_OPC_CONST,
	.pri_operand = ULP_THOR2_SYM_PROF_TCAM_PRI_CATCHALL,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.track_type = CFA_TRACK_TYPE_SID,
	.key_start_idx = 107,
	.blob_key_bit_size = 256,
	.key_bit_size = 256,
	.key_num_fields = 66,
	.result_start_idx = 68,
	.result_bit_size = 64,
	.result_num_fields = 10,
	.ident_start_idx = 9,
	.ident_nums = 0
	},
	{ /* class_tid: 1, , table: profile_tcam.cnp_catchall */
	.description = "profile_tcam.cnp_catchall",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_TCAM_TABLE,
	.resource_type = CFA_RSUBTYPE_TCAM_PROF_TCAM,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 4,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_TCAM_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_PROFILE_TCAM_INDEX_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.pri_opcode  = BNXT_ULP_PRI_OPC_CONST,
	.pri_operand = ULP_THOR2_SYM_PROF_TCAM_PRI_CATCHALL,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.track_type = CFA_TRACK_TYPE_SID,
	.key_start_idx = 173,
	.blob_key_bit_size = 256,
	.key_bit_size = 256,
	.key_num_fields = 66,
	.result_start_idx = 78,
	.result_bit_size = 64,
	.result_num_fields = 10,
	.ident_start_idx = 9,
	.ident_nums = 0
	},
	{ /* class_tid: 1, , table: mac_addr_cache.wr */
	.description = "mac_addr_cache.wr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_MAC_ADDR_CACHE,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 4,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_WRITE,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_HASH,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.key_start_idx = 239,
	.blob_key_bit_size = 195,
	.key_bit_size = 195,
	.key_num_fields = 10,
	.result_start_idx = 88,
	.result_bit_size = 94,
	.result_num_fields = 5
	},
	{ /* class_tid: 1, , table: control.roce_check */
	.description = "control.roce_check",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 20,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 4,
		.cond_nums = 1 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP
	},
	{ /* class_tid: 1, , table: proto_header_cache.rd */
	.description = "proto_header_cache.rd",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_PROTO_HEADER,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 5,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_READ,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_HASH,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.key_start_idx = 249,
	.blob_key_bit_size = 76,
	.key_bit_size = 76,
	.key_num_fields = 3,
	.ident_start_idx = 9,
	.ident_nums = 7
	},
	{ /* class_tid: 1, , table: control.proto_header_cache_miss */
	.description = "control.proto_header_cache_miss",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 7,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 5,
		.cond_nums = 1 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_ALLOC_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID
	},
	{ /* class_tid: 1, , table: fkb_select.em_fkb_alloc */
	.description = "fkb_select.em_fkb_alloc",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_INDEX_TABLE,
	.resource_type = CFA_RSUBTYPE_IDX_TBL_EM_FKB,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 6,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_ALLOC_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_EM_KEY_ID_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.track_type = CFA_TRACK_TYPE_SID,
	.result_start_idx = 93,
	.result_bit_size = 256,
	.result_num_fields = 172
	},
	{ /* class_tid: 1, , table: em_key_recipe.alloc_only */
	.description = "em_key_recipe.alloc_only",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_KEY_RECIPE_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_KEY_RECIPE_TABLE_EM,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 6,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_KEY_RECIPE_TBL_OPC_ALLOC_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_EM_RECIPE_ID,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.result_start_idx = 265,
	.result_bit_size = 0,
	.result_num_fields = 0
	},
	{ /* class_tid: 1, , table: fkb_select.wc_fkb */
	.description = "fkb_select.wc_fkb",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_INDEX_TABLE,
	.resource_type = CFA_RSUBTYPE_IDX_TBL_WC_FKB,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 6,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_WC_KEY_ID_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.track_type = CFA_TRACK_TYPE_SID,
	.result_start_idx = 265,
	.result_bit_size = 256,
	.result_num_fields = 172
	},
	{ /* class_tid: 1, , table: wm_key_recipe.wr */
	.description = "wm_key_recipe.wr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_KEY_RECIPE_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_KEY_RECIPE_TABLE_WM,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 36,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_KEY_RECIPE_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_WC_RECIPE_ID,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.key_start_idx = 252,
	.blob_key_bit_size = 0,
	.key_bit_size = 0,
	.key_num_fields = 9,
	.result_start_idx = 437,
	.result_bit_size = 0,
	.result_num_fields = 0
	},
	{ /* class_tid: 1, , table: profile_tcam.wr */
	.description = "profile_tcam.wr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_TCAM_TABLE,
	.resource_type = CFA_RSUBTYPE_TCAM_PROF_TCAM,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 76,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_TCAM_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_PROFILE_TCAM_INDEX_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.pri_opcode  = BNXT_ULP_PRI_OPC_CONST,
	.pri_operand = ULP_THOR2_SYM_PROF_TCAM_PRI_APP,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.track_type = CFA_TRACK_TYPE_SID,
	.key_start_idx = 261,
	.blob_key_bit_size = 256,
	.key_bit_size = 256,
	.key_num_fields = 66,
	.result_start_idx = 437,
	.result_bit_size = 64,
	.result_num_fields = 10,
	.ident_start_idx = 16,
	.ident_nums = 2
	},
	{ /* class_tid: 1, , table: proto_header_cache.wr */
	.description = "proto_header_cache.wr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_PROTO_HEADER,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 84,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_WRITE,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_HASH,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.key_start_idx = 327,
	.blob_key_bit_size = 76,
	.key_bit_size = 76,
	.key_num_fields = 3,
	.result_start_idx = 447,
	.result_bit_size = 106,
	.result_num_fields = 8
	},
	{ /* class_tid: 1, , table: control.terminating_flow */
	.description = "control.terminating_flow",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 84,
		.cond_nums = 0 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.func_info = {
		.func_opc = BNXT_ULP_FUNC_OPC_COND_LIST,
		.func_oper_size = 8,
		.func_src1 = BNXT_ULP_FUNC_SRC_KEY_EXT_LIST,
		.func_opr1 = 13,
		.func_dst_opr = BNXT_ULP_RF_IDX_TERM_FLOW }
	},
	{ /* class_tid: 1, , table: em_flow_conflict_cache.rd */
	.description = "em_flow_conflict_cache.rd",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_EM_FLOW_CONFLICT,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 8,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 91,
		.cond_nums = 2 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_READ,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_HASH,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.key_start_idx = 330,
	.blob_key_bit_size = 78,
	.key_bit_size = 78,
	.key_num_fields = 4,
	.ident_start_idx = 18,
	.ident_nums = 1
	},
	{ /* class_tid: 1, , table: control.em_flow_conflict_cache_miss */
	.description = "control.em_flow_conflict_cache_miss",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 4,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 93,
		.cond_nums = 1 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_ALLOC_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID
	},
	{ /* class_tid: 1, , table: fkb_select.em_fkb */
	.description = "fkb_select.em_fkb",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_INDEX_TABLE,
	.resource_type = CFA_RSUBTYPE_IDX_TBL_EM_FKB,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 94,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_EM_KEY_ID_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.track_type = CFA_TRACK_TYPE_SID,
	.result_start_idx = 455,
	.result_bit_size = 256,
	.result_num_fields = 172
	},
	{ /* class_tid: 1, , table: em_key_recipe.wr */
	.description = "em_key_recipe.wr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_KEY_RECIPE_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_KEY_RECIPE_TABLE_EM,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 126,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_KEY_RECIPE_TBL_OPC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_EM_RECIPE_ID,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.key_start_idx = 334,
	.blob_key_bit_size = 0,
	.key_bit_size = 0,
	.key_num_fields = 9,
	.result_start_idx = 627,
	.result_bit_size = 0,
	.result_num_fields = 0
	},
	{ /* class_tid: 1, , table: em_flow_conflict_cache.wr */
	.description = "em_flow_conflict_cache.wr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_EM_FLOW_CONFLICT,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 2,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 166,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_WRITE,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_HASH,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.key_start_idx = 343,
	.blob_key_bit_size = 78,
	.key_bit_size = 78,
	.key_num_fields = 4,
	.result_start_idx = 627,
	.result_bit_size = 96,
	.result_num_fields = 2
	},
	{ /* class_tid: 1, , table: control.field_sig_validation */
	.description = "control.field_sig_validation",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_RX,
	.true_message = "'Failed:Attempting' 'to' 'add' 'different' 'EM' 'for' 'same' 'headers'",
	.execute_info = {
		.cond_true_goto  = 1023,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 166,
		.cond_nums = 2 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.func_info = {
		.func_opc = BNXT_ULP_FUNC_OPC_EQ,
		.func_src1 = BNXT_ULP_FUNC_SRC_REGFILE,
		.func_opr1 = BNXT_ULP_RF_IDX_FLOW_SIG_ID,
		.func_src2 = BNXT_ULP_FUNC_SRC_COMP_FIELD,
		.func_opr2 = BNXT_ULP_CF_IDX_FLOW_SIG_ID,
		.func_dst_opr = BNXT_ULP_RF_IDX_CC }
	},
	{ /* class_tid: 1, , table: em_normal.wr */
	.description = "em_normal.wr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_EM_TABLE,
	.resource_type = TF_MEM_INTERNAL,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 0,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 168,
		.cond_nums = 1 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_DYN_KEY,
	.key_recipe_operand = BNXT_ULP_RF_IDX_EM_RECIPE_ID,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.critical_resource = BNXT_ULP_CRITICAL_RESOURCE_YES,
	.result_start_idx = 629,
	.result_bit_size = 0,
	.result_num_fields = 17
	},
	{ /* class_tid: 1, , table: em_normal.queue_wr */
	.description = "em_normal.queue_wr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_EM_TABLE,
	.resource_type = TF_MEM_INTERNAL,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 0,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 169,
		.cond_nums = 0 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_DYN_KEY,
	.key_recipe_operand = BNXT_ULP_RF_IDX_EM_RECIPE_ID,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.critical_resource = BNXT_ULP_CRITICAL_RESOURCE_YES,
	.result_start_idx = 646,
	.result_bit_size = 0,
	.result_num_fields = 17
	},
	{ /* class_tid: 1, , table: wm_normal.wr */
	.description = "wm_normal.wr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_TCAM_TABLE,
	.resource_type = CFA_RSUBTYPE_TCAM_WC,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 0,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 169,
		.cond_nums = 1 },
	.tbl_opcode = BNXT_ULP_TCAM_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_WC_TCAM_INDEX_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_DYN_KEY,
	.key_recipe_operand = BNXT_ULP_RF_IDX_WC_RECIPE_ID,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.pri_opcode  = BNXT_ULP_PRI_OPC_APP_PRI,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.critical_resource = BNXT_ULP_CRITICAL_RESOURCE_YES,
	.track_type = CFA_TRACK_TYPE_SID,
	.result_start_idx = 663,
	.result_bit_size = 128,
	.result_num_fields = 15
	},
	{ /* class_tid: 1, , table: wm_normal.queue_wr */
	.description = "wm_normal.queue_wr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_TCAM_TABLE,
	.resource_type = CFA_RSUBTYPE_TCAM_WC,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 0,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 170,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_TCAM_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_WC_TCAM_INDEX_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_DYN_KEY,
	.key_recipe_operand = BNXT_ULP_RF_IDX_WC_RECIPE_ID,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.pri_opcode  = BNXT_ULP_PRI_OPC_APP_PRI,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.critical_resource = BNXT_ULP_CRITICAL_RESOURCE_YES,
	.track_type = CFA_TRACK_TYPE_SID,
	.result_start_idx = 678,
	.result_bit_size = 128,
	.result_num_fields = 15
	},
	{ /* class_tid: 1, , table: proto_header_cache.roce_rd */
	.description = "proto_header_cache.roce_rd",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_PROTO_HEADER,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 170,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_READ,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_HASH,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.key_start_idx = 347,
	.blob_key_bit_size = 76,
	.key_bit_size = 76,
	.key_num_fields = 3,
	.ident_start_idx = 19,
	.ident_nums = 7
	},
	{ /* class_tid: 1, , table: control.roce_proto_header_cache_miss */
	.description = "control.roce_proto_header_cache_miss",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 8,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 170,
		.cond_nums = 1 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_ALLOC_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID
	},
	{ /* class_tid: 1, , table: fkb_select.em_fkb_roce_alloc */
	.description = "fkb_select.em_fkb_roce_alloc",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_INDEX_TABLE,
	.resource_type = CFA_RSUBTYPE_IDX_TBL_EM_FKB,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 171,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_ALLOC_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_EM_KEY_ID_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.track_type = CFA_TRACK_TYPE_SID,
	.result_start_idx = 693,
	.result_bit_size = 256,
	.result_num_fields = 172
	},
	{ /* class_tid: 1, , table: em_key_recipe.roce_alloc_only */
	.description = "em_key_recipe.roce_alloc_only",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_KEY_RECIPE_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_KEY_RECIPE_TABLE_EM,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 171,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_KEY_RECIPE_TBL_OPC_ALLOC_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_EM_RECIPE_ID,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.result_start_idx = 865,
	.result_bit_size = 0,
	.result_num_fields = 0
	},
	{ /* class_tid: 1, , table: fkb_select.roce_wc_fkb */
	.description = "fkb_select.roce_wc_fkb",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_INDEX_TABLE,
	.resource_type = CFA_RSUBTYPE_IDX_TBL_WC_FKB,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 171,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_WC_KEY_ID_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.track_type = CFA_TRACK_TYPE_SID,
	.result_start_idx = 865,
	.result_bit_size = 256,
	.result_num_fields = 172
	},
	{ /* class_tid: 1, , table: wm_key_recipe.roce_wr */
	.description = "wm_key_recipe.roce_wr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_KEY_RECIPE_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_KEY_RECIPE_TABLE_WM,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 181,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_KEY_RECIPE_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_WC_RECIPE_ID,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.key_start_idx = 350,
	.blob_key_bit_size = 0,
	.key_bit_size = 0,
	.key_num_fields = 6,
	.result_start_idx = 1037,
	.result_bit_size = 0,
	.result_num_fields = 0
	},
	{ /* class_tid: 1, , table: profile_tcam.roce_wr */
	.description = "profile_tcam.roce_wr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_TCAM_TABLE,
	.resource_type = CFA_RSUBTYPE_TCAM_PROF_TCAM,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 201,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_TCAM_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_PROFILE_TCAM_INDEX_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.pri_opcode  = BNXT_ULP_PRI_OPC_CONST,
	.pri_operand = ULP_THOR2_SYM_PROF_TCAM_PRI_APP,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.track_type = CFA_TRACK_TYPE_SID,
	.key_start_idx = 356,
	.blob_key_bit_size = 256,
	.key_bit_size = 256,
	.key_num_fields = 66,
	.result_start_idx = 1037,
	.result_bit_size = 64,
	.result_num_fields = 10,
	.ident_start_idx = 26,
	.ident_nums = 2
	},
	{ /* class_tid: 1, , table: profile_tcam.roce_cnp_wr */
	.description = "profile_tcam.roce_cnp_wr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_TCAM_TABLE,
	.resource_type = CFA_RSUBTYPE_TCAM_PROF_TCAM,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 205,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_TCAM_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_PROFILE_TCAM_INDEX_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.pri_opcode  = BNXT_ULP_PRI_OPC_CONST,
	.pri_operand = ULP_THOR2_SYM_PROF_TCAM_PRI_APP,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.track_type = CFA_TRACK_TYPE_SID,
	.key_start_idx = 422,
	.blob_key_bit_size = 256,
	.key_bit_size = 256,
	.key_num_fields = 66,
	.result_start_idx = 1047,
	.result_bit_size = 64,
	.result_num_fields = 10,
	.ident_start_idx = 28,
	.ident_nums = 0
	},
	{ /* class_tid: 1, , table: proto_header_cache.roce_wr */
	.description = "proto_header_cache.roce_wr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_PROTO_HEADER,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 209,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_WRITE,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_HASH,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.key_start_idx = 488,
	.blob_key_bit_size = 76,
	.key_bit_size = 76,
	.key_num_fields = 3,
	.result_start_idx = 1057,
	.result_bit_size = 106,
	.result_num_fields = 8
	},
	{ /* class_tid: 1, , table: control.roce_terminating_flow */
	.description = "control.roce_terminating_flow",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 209,
		.cond_nums = 0 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.func_info = {
		.func_opc = BNXT_ULP_FUNC_OPC_COND_LIST,
		.func_oper_size = 8,
		.func_src1 = BNXT_ULP_FUNC_SRC_KEY_EXT_LIST,
		.func_opr1 = 32,
		.func_dst_opr = BNXT_ULP_RF_IDX_TERM_FLOW }
	},
	{ /* class_tid: 1, , table: em_flow_conflict_cache.roce_rd */
	.description = "em_flow_conflict_cache.roce_rd",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_EM_FLOW_CONFLICT,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 8,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 215,
		.cond_nums = 2 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_READ,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_HASH,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.key_start_idx = 491,
	.blob_key_bit_size = 78,
	.key_bit_size = 78,
	.key_num_fields = 4,
	.ident_start_idx = 28,
	.ident_nums = 1
	},
	{ /* class_tid: 1, , table: control.roce_em_flow_conflict_cache_miss */
	.description = "control.roce_em_flow_conflict_cache_miss",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 4,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 217,
		.cond_nums = 1 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_ALLOC_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID
	},
	{ /* class_tid: 1, , table: fkb_select.em_fkb_roce */
	.description = "fkb_select.em_fkb_roce",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_INDEX_TABLE,
	.resource_type = CFA_RSUBTYPE_IDX_TBL_EM_FKB,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 218,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_EM_KEY_ID_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.track_type = CFA_TRACK_TYPE_SID,
	.result_start_idx = 1065,
	.result_bit_size = 256,
	.result_num_fields = 172
	},
	{ /* class_tid: 1, , table: em_key_recipe.roce */
	.description = "em_key_recipe.roce",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_KEY_RECIPE_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_KEY_RECIPE_TABLE_EM,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 246,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_KEY_RECIPE_TBL_OPC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_EM_RECIPE_ID,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.key_start_idx = 495,
	.blob_key_bit_size = 0,
	.key_bit_size = 0,
	.key_num_fields = 10,
	.result_start_idx = 1237,
	.result_bit_size = 0,
	.result_num_fields = 0
	},
	{ /* class_tid: 1, , table: em_flow_conflict_cache.roce_wr */
	.description = "em_flow_conflict_cache.roce_wr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_EM_FLOW_CONFLICT,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 2,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 282,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_WRITE,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_HASH,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.key_start_idx = 505,
	.blob_key_bit_size = 78,
	.key_bit_size = 78,
	.key_num_fields = 4,
	.result_start_idx = 1237,
	.result_bit_size = 96,
	.result_num_fields = 2
	},
	{ /* class_tid: 1, , table: control.roce_field_sig_validation */
	.description = "control.roce_field_sig_validation",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_RX,
	.true_message = "'Failed:Attempting' 'to' 'add' 'different' 'EM' 'for' 'same' 'headers'",
	.execute_info = {
		.cond_true_goto  = 1023,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 282,
		.cond_nums = 2 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.func_info = {
		.func_opc = BNXT_ULP_FUNC_OPC_EQ,
		.func_src1 = BNXT_ULP_FUNC_SRC_REGFILE,
		.func_opr1 = BNXT_ULP_RF_IDX_FLOW_SIG_ID,
		.func_src2 = BNXT_ULP_FUNC_SRC_COMP_FIELD,
		.func_opr2 = BNXT_ULP_CF_IDX_FLOW_SIG_ID,
		.func_dst_opr = BNXT_ULP_RF_IDX_CC }
	},
	{ /* class_tid: 1, , table: em_normal.roce */
	.description = "em_normal.roce",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_EM_TABLE,
	.resource_type = TF_MEM_INTERNAL,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 0,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 284,
		.cond_nums = 1 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_DYN_KEY,
	.key_recipe_operand = BNXT_ULP_RF_IDX_EM_RECIPE_ID,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.critical_resource = BNXT_ULP_CRITICAL_RESOURCE_YES,
	.result_start_idx = 1239,
	.result_bit_size = 0,
	.result_num_fields = 17
	},
	{ /* class_tid: 1, , table: em_normal.roce_queue */
	.description = "em_normal.roce_queue",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_EM_TABLE,
	.resource_type = TF_MEM_INTERNAL,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 0,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 285,
		.cond_nums = 0 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_DYN_KEY,
	.key_recipe_operand = BNXT_ULP_RF_IDX_EM_RECIPE_ID,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.critical_resource = BNXT_ULP_CRITICAL_RESOURCE_YES,
	.result_start_idx = 1256,
	.result_bit_size = 0,
	.result_num_fields = 17
	},
	{ /* class_tid: 1, , table: wm_normal.roce */
	.description = "wm_normal.roce",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_TCAM_TABLE,
	.resource_type = CFA_RSUBTYPE_TCAM_WC,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 0,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 285,
		.cond_nums = 1 },
	.tbl_opcode = BNXT_ULP_TCAM_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_WC_TCAM_INDEX_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_DYN_KEY,
	.key_recipe_operand = BNXT_ULP_RF_IDX_WC_RECIPE_ID,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.pri_opcode  = BNXT_ULP_PRI_OPC_APP_PRI,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.critical_resource = BNXT_ULP_CRITICAL_RESOURCE_YES,
	.track_type = CFA_TRACK_TYPE_SID,
	.result_start_idx = 1273,
	.result_bit_size = 128,
	.result_num_fields = 15
	},
	{ /* class_tid: 1, , table: wm_normal.roce_queue */
	.description = "wm_normal.roce_queue",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_TCAM_TABLE,
	.resource_type = CFA_RSUBTYPE_TCAM_WC,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 0,
		.cond_false_goto = 0,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 286,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_TCAM_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_WC_TCAM_INDEX_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_DYN_KEY,
	.key_recipe_operand = BNXT_ULP_RF_IDX_WC_RECIPE_ID,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.pri_opcode  = BNXT_ULP_PRI_OPC_APP_PRI,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.critical_resource = BNXT_ULP_CRITICAL_RESOURCE_YES,
	.track_type = CFA_TRACK_TYPE_SID,
	.result_start_idx = 1288,
	.result_bit_size = 128,
	.result_num_fields = 15
	},
	{ /* class_tid: 2, , table: port_table.egr_wr_0 */
	.description = "port_table.egr_wr_0",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_PORT_TABLE,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 288,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_READ,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_INDEX,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.key_start_idx = 509,
	.blob_key_bit_size = 10,
	.key_bit_size = 10,
	.key_num_fields = 1,
	.ident_start_idx = 29,
	.ident_nums = 2
	},
	{ /* class_tid: 2, , table: control.thor2_roce_check */
	.description = "control.thor2_roce_check",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1023,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 288,
		.cond_nums = 1 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP
	},
	{ /* class_tid: 2, , table: l2_cntxt_tcam_cache.egr_rd */
	.description = "l2_cntxt_tcam_cache.egr_rd",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_L2_CNTXT_TCAM,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 289,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_READ,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_INDEX,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.key_start_idx = 510,
	.blob_key_bit_size = 11,
	.key_bit_size = 11,
	.key_num_fields = 1,
	.ident_start_idx = 31,
	.ident_nums = 2
	},
	{ /* class_tid: 2, , table: control.l2_cache_chk */
	.description = "control.l2_cache_chk",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 5,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 289,
		.cond_nums = 1 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_ALLOC_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID
	},
	{ /* class_tid: 2, , table: l2_cntxt_tcam.l2_table_create */
	.description = "l2_cntxt_tcam.l2_table_create",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_TCAM_TABLE,
	.resource_type = CFA_RSUBTYPE_TCAM_L2CTX,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 290,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_TCAM_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_L2_CNTXT_TCAM_INDEX_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.pri_opcode  = BNXT_ULP_PRI_OPC_CONST,
	.pri_operand = ULP_THOR2_SYM_L2_CTXT_PRI_APP,
	.track_type = CFA_TRACK_TYPE_SID,
	.key_start_idx = 511,
	.blob_key_bit_size = 256,
	.key_bit_size = 256,
	.key_num_fields = 24,
	.result_start_idx = 1303,
	.result_bit_size = 127,
	.result_num_fields = 17,
	.ident_start_idx = 33,
	.ident_nums = 2
	},
	{ /* class_tid: 2, , table: l2_cntxt_tcam.roce_table_create */
	.description = "l2_cntxt_tcam.roce_table_create",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_TCAM_TABLE,
	.resource_type = CFA_RSUBTYPE_TCAM_L2CTX,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 290,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_TCAM_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_L2_CNTXT_TCAM_INDEX_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.pri_opcode  = BNXT_ULP_PRI_OPC_CONST,
	.pri_operand = ULP_THOR2_SYM_L2_CTXT_PRI_APP,
	.track_type = CFA_TRACK_TYPE_SID,
	.key_start_idx = 535,
	.blob_key_bit_size = 256,
	.key_bit_size = 256,
	.key_num_fields = 24,
	.result_start_idx = 1320,
	.result_bit_size = 127,
	.result_num_fields = 17,
	.ident_start_idx = 35,
	.ident_nums = 0
	},
	{ /* class_tid: 2, , table: profile_tcam.catchall */
	.description = "profile_tcam.catchall",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_TCAM_TABLE,
	.resource_type = CFA_RSUBTYPE_TCAM_PROF_TCAM,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 290,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_TCAM_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_PROFILE_TCAM_INDEX_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.pri_opcode  = BNXT_ULP_PRI_OPC_CONST,
	.pri_operand = ULP_THOR2_SYM_PROF_TCAM_PRI_CATCHALL,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.track_type = CFA_TRACK_TYPE_SID,
	.key_start_idx = 559,
	.blob_key_bit_size = 256,
	.key_bit_size = 256,
	.key_num_fields = 66,
	.result_start_idx = 1337,
	.result_bit_size = 64,
	.result_num_fields = 10,
	.ident_start_idx = 35,
	.ident_nums = 0
	},
	{ /* class_tid: 2, , table: l2_cntxt_tcam_cache.egr_wr */
	.description = "l2_cntxt_tcam_cache.egr_wr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_L2_CNTXT_TCAM,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 290,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_WRITE,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_INDEX,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.key_start_idx = 625,
	.blob_key_bit_size = 11,
	.key_bit_size = 11,
	.key_num_fields = 1,
	.result_start_idx = 1347,
	.result_bit_size = 94,
	.result_num_fields = 5
	},
	{ /* class_tid: 2, , table: proto_header_cache.roce_rd */
	.description = "proto_header_cache.roce_rd",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_PROTO_HEADER,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 290,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_READ,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_HASH,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.key_start_idx = 626,
	.blob_key_bit_size = 76,
	.key_bit_size = 76,
	.key_num_fields = 3,
	.ident_start_idx = 35,
	.ident_nums = 7
	},
	{ /* class_tid: 2, , table: control.proto_header_cache_miss */
	.description = "control.proto_header_cache_miss",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 7,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 290,
		.cond_nums = 1 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_ALLOC_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID
	},
	{ /* class_tid: 2, , table: fkb_select.em_fkb_roce_alloc */
	.description = "fkb_select.em_fkb_roce_alloc",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_INDEX_TABLE,
	.resource_type = CFA_RSUBTYPE_IDX_TBL_EM_FKB,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 291,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_ALLOC_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_EM_KEY_ID_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.track_type = CFA_TRACK_TYPE_SID,
	.result_start_idx = 1352,
	.result_bit_size = 256,
	.result_num_fields = 172
	},
	{ /* class_tid: 2, , table: em_key_recipe.roce_alloc_only */
	.description = "em_key_recipe.roce_alloc_only",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_KEY_RECIPE_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_KEY_RECIPE_TABLE_EM,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 291,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_KEY_RECIPE_TBL_OPC_ALLOC_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_EM_RECIPE_ID,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.result_start_idx = 1524,
	.result_bit_size = 0,
	.result_num_fields = 0
	},
	{ /* class_tid: 2, , table: fkb_select.wc_fkb_roce */
	.description = "fkb_select.wc_fkb_roce",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_INDEX_TABLE,
	.resource_type = CFA_RSUBTYPE_IDX_TBL_WC_FKB,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 291,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_WC_KEY_ID_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.track_type = CFA_TRACK_TYPE_SID,
	.result_start_idx = 1524,
	.result_bit_size = 256,
	.result_num_fields = 172
	},
	{ /* class_tid: 2, , table: wm_key_recipe.roce */
	.description = "wm_key_recipe.roce",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_KEY_RECIPE_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_KEY_RECIPE_TABLE_WM,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 321,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_KEY_RECIPE_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_WC_RECIPE_ID,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.key_start_idx = 629,
	.blob_key_bit_size = 0,
	.key_bit_size = 0,
	.key_num_fields = 10,
	.result_start_idx = 1696,
	.result_bit_size = 0,
	.result_num_fields = 0
	},
	{ /* class_tid: 2, , table: profile_tcam.roce */
	.description = "profile_tcam.roce",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_TCAM_TABLE,
	.resource_type = CFA_RSUBTYPE_TCAM_PROF_TCAM,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 357,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_TCAM_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_PROFILE_TCAM_INDEX_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.pri_opcode  = BNXT_ULP_PRI_OPC_CONST,
	.pri_operand = ULP_THOR2_SYM_PROF_TCAM_PRI_APP,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.track_type = CFA_TRACK_TYPE_SID,
	.key_start_idx = 639,
	.blob_key_bit_size = 256,
	.key_bit_size = 256,
	.key_num_fields = 66,
	.result_start_idx = 1696,
	.result_bit_size = 64,
	.result_num_fields = 10,
	.ident_start_idx = 42,
	.ident_nums = 2
	},
	{ /* class_tid: 2, , table: proto_header_cache.roce_wr */
	.description = "proto_header_cache.roce_wr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_PROTO_HEADER,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 361,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_WRITE,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_HASH,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.key_start_idx = 705,
	.blob_key_bit_size = 76,
	.key_bit_size = 76,
	.key_num_fields = 3,
	.result_start_idx = 1706,
	.result_bit_size = 106,
	.result_num_fields = 8
	},
	{ /* class_tid: 2, , table: control.roce_terminating_flow */
	.description = "control.roce_terminating_flow",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 361,
		.cond_nums = 0 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.func_info = {
		.func_opc = BNXT_ULP_FUNC_OPC_COND_LIST,
		.func_oper_size = 8,
		.func_src1 = BNXT_ULP_FUNC_SRC_KEY_EXT_LIST,
		.func_opr1 = 47,
		.func_dst_opr = BNXT_ULP_RF_IDX_TERM_FLOW }
	},
	{ /* class_tid: 2, , table: em_flow_conflict_cache.roce_rd */
	.description = "em_flow_conflict_cache.roce_rd",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_EM_FLOW_CONFLICT,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 7,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 365,
		.cond_nums = 2 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_READ,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_HASH,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.key_start_idx = 708,
	.blob_key_bit_size = 78,
	.key_bit_size = 78,
	.key_num_fields = 4,
	.ident_start_idx = 44,
	.ident_nums = 1
	},
	{ /* class_tid: 2, , table: control.roce_em_flow_conflict_cache_miss */
	.description = "control.roce_em_flow_conflict_cache_miss",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 4,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 367,
		.cond_nums = 1 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_ALLOC_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID
	},
	{ /* class_tid: 2, , table: fkb_select.em_fkb_roce */
	.description = "fkb_select.em_fkb_roce",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_INDEX_TABLE,
	.resource_type = CFA_RSUBTYPE_IDX_TBL_EM_FKB,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 368,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_EM_KEY_ID_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.track_type = CFA_TRACK_TYPE_SID,
	.result_start_idx = 1714,
	.result_bit_size = 256,
	.result_num_fields = 172
	},
	{ /* class_tid: 2, , table: em_key_recipe.roce */
	.description = "em_key_recipe.roce",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_KEY_RECIPE_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_KEY_RECIPE_TABLE_EM,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 396,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_KEY_RECIPE_TBL_OPC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_EM_RECIPE_ID,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.key_start_idx = 712,
	.blob_key_bit_size = 0,
	.key_bit_size = 0,
	.key_num_fields = 9,
	.result_start_idx = 1886,
	.result_bit_size = 0,
	.result_num_fields = 0
	},
	{ /* class_tid: 2, , table: em_flow_conflict_cache.roce_wr */
	.description = "em_flow_conflict_cache.roce_wr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_EM_FLOW_CONFLICT,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 2,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 428,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_WRITE,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_HASH,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.key_start_idx = 721,
	.blob_key_bit_size = 78,
	.key_bit_size = 78,
	.key_num_fields = 4,
	.result_start_idx = 1886,
	.result_bit_size = 96,
	.result_num_fields = 2
	},
	{ /* class_tid: 2, , table: control.roce_field_sig_validation */
	.description = "control.roce_field_sig_validation",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_TX,
	.true_message = "'Failed:Attempting' 'to' 'add' 'different' 'EM' 'for' 'same' 'headers'",
	.execute_info = {
		.cond_true_goto  = 1023,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 428,
		.cond_nums = 2 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.func_info = {
		.func_opc = BNXT_ULP_FUNC_OPC_EQ,
		.func_src1 = BNXT_ULP_FUNC_SRC_REGFILE,
		.func_opr1 = BNXT_ULP_RF_IDX_FLOW_SIG_ID,
		.func_src2 = BNXT_ULP_FUNC_SRC_COMP_FIELD,
		.func_opr2 = BNXT_ULP_CF_IDX_FLOW_SIG_ID,
		.func_dst_opr = BNXT_ULP_RF_IDX_CC }
	},
	{ /* class_tid: 2, , table: em_normal.roce */
	.description = "em_normal.roce",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_EM_TABLE,
	.resource_type = TF_MEM_INTERNAL,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 0,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 430,
		.cond_nums = 0 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_DYN_KEY,
	.key_recipe_operand = BNXT_ULP_RF_IDX_EM_RECIPE_ID,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.critical_resource = BNXT_ULP_CRITICAL_RESOURCE_YES,
	.result_start_idx = 1888,
	.result_bit_size = 0,
	.result_num_fields = 17
	},
	{ /* class_tid: 2, , table: wm_normal.roce */
	.description = "wm_normal.roce",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_TCAM_TABLE,
	.resource_type = CFA_RSUBTYPE_TCAM_WC,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 0,
		.cond_false_goto = 0,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 430,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_TCAM_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_WC_TCAM_INDEX_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_DYN_KEY,
	.key_recipe_operand = BNXT_ULP_RF_IDX_WC_RECIPE_ID,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.pri_opcode  = BNXT_ULP_PRI_OPC_APP_PRI,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.critical_resource = BNXT_ULP_CRITICAL_RESOURCE_YES,
	.track_type = CFA_TRACK_TYPE_SID,
	.result_start_idx = 1905,
	.result_bit_size = 128,
	.result_num_fields = 15
	},
	{ /* class_tid: 3, , table: control.do_nothing */
	.description = "control.do_nothing",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 0,
		.cond_false_goto = 0,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 430,
		.cond_nums = 0 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP
	},
	{ /* class_tid: 4, , table: cmm_full_act_record.ing_default_0 */
	.description = "cmm_full_act_record.ing_default_0",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CMM_TABLE,
	.resource_type = CFA_RSUBTYPE_CMM_ACT,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_CMM_TABLE_ACT,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 430,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_CMM_ACT_HNDL,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.result_start_idx = 1920,
	.result_bit_size = 192,
	.result_num_fields = 18
	},
	{ /* class_tid: 4, , table: shared_mirror_record.rd_pf_ing */
	.description = "shared_mirror_record.rd_pf_ing",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_type = TF_TBL_TYPE_MIRROR_CONFIG,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_SHARED_MIRROR,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 430,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_READ,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_INDEX,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.key_start_idx = 725,
	.blob_key_bit_size = 8,
	.key_bit_size = 8,
	.key_num_fields = 1,
	.ident_start_idx = 45,
	.ident_nums = 2
	},
	{ /* class_tid: 4, , table: control.pf_mirror_ing */
	.description = "control.pf_mirror_ing",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 9,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 430,
		.cond_nums = 1 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_ALLOC_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID
	},
	{ /* class_tid: 4, , table: mirror_tbl.alloc_pf_ing */
	.description = "mirror_tbl.alloc_pf_ing",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_INDEX_TABLE,
	.resource_type = CFA_RSUBTYPE_IDX_TBL_MIRROR,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_INDEX_TABLE_NORMAL,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 431,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_ALLOC_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_MIRROR_PTR_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.track_type = CFA_TRACK_TYPE_SID,
	.result_start_idx = 1938,
	.result_bit_size = 128,
	.result_num_fields = 12
	},
	{ /* class_tid: 4, , table: cmm_stat_record.ing_pf_mirror */
	.description = "cmm_stat_record.ing_pf_mirror",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CMM_STAT,
	.resource_type = CFA_RSUBTYPE_CMM_ACT,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_CMM_TABLE_ACT,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 431,
		.cond_nums = 1 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_ALLOC_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_CMM_STAT_HNDL_MIRROR,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.result_start_idx = 1950,
	.result_bit_size = 128,
	.result_num_fields = 2
	},
	{ /* class_tid: 4, , table: control.stat_handle_to_offset_ing_pf_mirror */
	.description = "control.stat_handle_to_offset_ing_pf_mirror",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 432,
		.cond_nums = 0 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.func_info = {
		.func_opc = BNXT_ULP_FUNC_OPC_HANDLE_TO_OFFSET,
		.func_src1 = BNXT_ULP_FUNC_SRC_REGFILE,
		.func_opr1 = BNXT_ULP_RF_IDX_CMM_STAT_HNDL_MIRROR,
		.func_src2 = BNXT_ULP_FUNC_SRC_CONST,
		.func_opr2 = 8,
		.func_dst_opr = BNXT_ULP_RF_IDX_MIRR_FLOW_CNTR_PTR_0 }
	},
	{ /* class_tid: 4, , table: cmm_full_act_record.ing_pf_mirror */
	.description = "cmm_full_act_record.ing_pf_mirror",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CMM_TABLE,
	.resource_type = CFA_RSUBTYPE_CMM_ACT,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_CMM_TABLE_ACT,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 432,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_CMM_ACT_HNDL_MIRROR,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.result_start_idx = 1952,
	.result_bit_size = 192,
	.result_num_fields = 18
	},
	{ /* class_tid: 4, , table: control.act_handle_to_offset_ing_pf_mirror */
	.description = "control.act_handle_to_offset_ing_pf_mirror",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 432,
		.cond_nums = 0 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.func_info = {
		.func_opc = BNXT_ULP_FUNC_OPC_HANDLE_TO_OFFSET,
		.func_src1 = BNXT_ULP_FUNC_SRC_REGFILE,
		.func_opr1 = BNXT_ULP_RF_IDX_CMM_ACT_HNDL_MIRROR,
		.func_src2 = BNXT_ULP_FUNC_SRC_CONST,
		.func_opr2 = 32,
		.func_dst_opr = BNXT_ULP_RF_IDX_MIRR_ACTION_PTR }
	},
	{ /* class_tid: 4, , table: mirror_tbl.wr_pf_ing */
	.description = "mirror_tbl.wr_pf_ing",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_INDEX_TABLE,
	.resource_type = CFA_RSUBTYPE_IDX_TBL_MIRROR,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_INDEX_TABLE_NORMAL,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 432,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_MIRROR_PTR_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.track_type = CFA_TRACK_TYPE_SID,
	.result_start_idx = 1970,
	.result_bit_size = 128,
	.result_num_fields = 12
	},
	{ /* class_tid: 4, , table: shared_mirror_record.wr_pf_ing */
	.description = "shared_mirror_record.wr_pf_ing",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_type = CFA_RSUBTYPE_IDX_TBL_MIRROR,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_SHARED_MIRROR,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 432,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_WRITE,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_INDEX,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.key_start_idx = 726,
	.blob_key_bit_size = 8,
	.key_bit_size = 8,
	.key_num_fields = 1,
	.result_start_idx = 1982,
	.result_bit_size = 101,
	.result_num_fields = 3
	},
	{ /* class_tid: 4, , table: control.save_mirror_id_ing */
	.description = "control.save_mirror_id_ing",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 432,
		.cond_nums = 0 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.func_info = {
		.func_opc = BNXT_ULP_FUNC_OPC_PORT_TABLE,
		.func_oper_size = 16,
		.func_src1 = BNXT_ULP_FUNC_SRC_COMP_FIELD,
		.func_opr1 = BNXT_ULP_CF_IDX_DEV_PORT_ID,
		.func_src2 = BNXT_ULP_FUNC_SRC_REGFILE,
		.func_opr2 = BNXT_ULP_RF_IDX_MIRROR_PTR_0,
		.func_dst_opr = BNXT_ULP_PORT_TABLE_PHY_PORT_MIRROR_ID }
	},
	{ /* class_tid: 4, , table: cmm_full_act_record.ing_default_arec */
	.description = "cmm_full_act_record.ing_default_arec",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CMM_TABLE,
	.resource_type = CFA_RSUBTYPE_CMM_ACT,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_CMM_TABLE_ACT,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 432,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_CMM_ACT_HNDL,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.result_start_idx = 1985,
	.result_bit_size = 192,
	.result_num_fields = 18
	},
	{ /* class_tid: 4, , table: control.act_handle_to_offset_1 */
	.description = "control.act_handle_to_offset_1",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 432,
		.cond_nums = 0 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.func_info = {
		.func_opc = BNXT_ULP_FUNC_OPC_HANDLE_TO_OFFSET,
		.func_src1 = BNXT_ULP_FUNC_SRC_REGFILE,
		.func_opr1 = BNXT_ULP_RF_IDX_CMM_ACT_HNDL,
		.func_src2 = BNXT_ULP_FUNC_SRC_CONST,
		.func_opr2 = 32,
		.func_dst_opr = BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR }
	},
	{ /* class_tid: 4, , table: cmm_full_act_record.ing_default_roce_arec */
	.description = "cmm_full_act_record.ing_default_roce_arec",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CMM_TABLE,
	.resource_type = CFA_RSUBTYPE_CMM_ACT,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_CMM_TABLE_ACT,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 432,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_CMM_ACT_HNDL,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.result_start_idx = 2003,
	.result_bit_size = 192,
	.result_num_fields = 18
	},
	{ /* class_tid: 4, , table: control.act_handle_to_offset_2 */
	.description = "control.act_handle_to_offset_2",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 432,
		.cond_nums = 0 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.func_info = {
		.func_opc = BNXT_ULP_FUNC_OPC_HANDLE_TO_OFFSET,
		.func_src1 = BNXT_ULP_FUNC_SRC_REGFILE,
		.func_opr1 = BNXT_ULP_RF_IDX_CMM_ACT_HNDL,
		.func_src2 = BNXT_ULP_FUNC_SRC_CONST,
		.func_opr2 = 32,
		.func_dst_opr = BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR_ROCE }
	},
	{ /* class_tid: 4, , table: port_table.ing_wr_0 */
	.description = "port_table.ing_wr_0",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_PORT_TABLE,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 432,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_WRITE,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_INDEX,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.key_start_idx = 727,
	.blob_key_bit_size = 10,
	.key_bit_size = 10,
	.key_num_fields = 1,
	.result_start_idx = 2021,
	.result_bit_size = 185,
	.result_num_fields = 7
	},
	{ /* class_tid: 4, , table: cmm_full_act_record.egr_throw_away */
	.description = "cmm_full_act_record.egr_throw_away",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CMM_TABLE,
	.resource_type = CFA_RSUBTYPE_CMM_ACT,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_CMM_TABLE_ACT,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 432,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_CMM_ACT_HNDL,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.result_start_idx = 2028,
	.result_bit_size = 192,
	.result_num_fields = 18
	},
	{ /* class_tid: 4, , table: shared_mirror_record.rd_pf_egr */
	.description = "shared_mirror_record.rd_pf_egr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_type = TF_TBL_TYPE_MIRROR_CONFIG,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_SHARED_MIRROR,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 432,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_READ,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_INDEX,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.key_start_idx = 728,
	.blob_key_bit_size = 8,
	.key_bit_size = 8,
	.key_num_fields = 1,
	.ident_start_idx = 47,
	.ident_nums = 2
	},
	{ /* class_tid: 4, , table: control.pf_mirror_egr */
	.description = "control.pf_mirror_egr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 11,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 432,
		.cond_nums = 1 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_ALLOC_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID
	},
	{ /* class_tid: 4, , table: mirror_tbl.alloc_pf_egr */
	.description = "mirror_tbl.alloc_pf_egr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_INDEX_TABLE,
	.resource_type = CFA_RSUBTYPE_IDX_TBL_MIRROR,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_INDEX_TABLE_NORMAL,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 433,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_ALLOC_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_MIRROR_PTR_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.track_type = CFA_TRACK_TYPE_SID,
	.result_start_idx = 2046,
	.result_bit_size = 128,
	.result_num_fields = 12
	},
	{ /* class_tid: 4, , table: mod_record.meta_pf_mirror */
	.description = "mod_record.meta_pf_mirror",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CMM_TABLE,
	.resource_type = CFA_RSUBTYPE_CMM_ACT,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_CMM_TABLE_ACT,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 433,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_CMM_MOD_HNDL,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.func_info = {
		.func_opc = BNXT_ULP_FUNC_OPC_BIT_OR,
		.func_src1 = BNXT_ULP_FUNC_SRC_COMP_FIELD,
		.func_opr1 = BNXT_ULP_CF_IDX_DRV_FUNC_VNIC,
		.func_src2 = BNXT_ULP_FUNC_SRC_CONST,
		.func_opr2 = ULP_THOR2_SYM_VF_2_VF_META_VAL,
		.func_dst_opr = BNXT_ULP_RF_IDX_RF_0 },
	.result_start_idx = 2058,
	.result_bit_size = 0,
	.result_num_fields = 0,
	.encap_num_fields = 20
	},
	{ /* class_tid: 4, , table: control.mod_handle_to_offset_egr_pf_mirror */
	.description = "control.mod_handle_to_offset_egr_pf_mirror",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 433,
		.cond_nums = 0 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.func_info = {
		.func_opc = BNXT_ULP_FUNC_OPC_HANDLE_TO_OFFSET,
		.func_src1 = BNXT_ULP_FUNC_SRC_REGFILE,
		.func_opr1 = BNXT_ULP_RF_IDX_CMM_MOD_HNDL,
		.func_src2 = BNXT_ULP_FUNC_SRC_CONST,
		.func_opr2 = 8,
		.func_dst_opr = BNXT_ULP_RF_IDX_MIRR_MODIFY_PTR }
	},
	{ /* class_tid: 4, , table: cmm_stat_record.pf_mirror_egr */
	.description = "cmm_stat_record.pf_mirror_egr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CMM_STAT,
	.resource_type = CFA_RSUBTYPE_CMM_ACT,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_CMM_TABLE_ACT,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 433,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_ALLOC_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_CMM_STAT_HNDL_MIRROR,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.result_start_idx = 2078,
	.result_bit_size = 128,
	.result_num_fields = 2
	},
	{ /* class_tid: 4, , table: control.stat_handle_to_offset_egr_pf_mirror */
	.description = "control.stat_handle_to_offset_egr_pf_mirror",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 433,
		.cond_nums = 0 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.func_info = {
		.func_opc = BNXT_ULP_FUNC_OPC_HANDLE_TO_OFFSET,
		.func_src1 = BNXT_ULP_FUNC_SRC_REGFILE,
		.func_opr1 = BNXT_ULP_RF_IDX_CMM_STAT_HNDL_MIRROR,
		.func_src2 = BNXT_ULP_FUNC_SRC_CONST,
		.func_opr2 = 8,
		.func_dst_opr = BNXT_ULP_RF_IDX_MIRR_FLOW_CNTR_PTR_0 }
	},
	{ /* class_tid: 4, , table: cmm_full_act_record.pf_mirror_egr */
	.description = "cmm_full_act_record.pf_mirror_egr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CMM_TABLE,
	.resource_type = CFA_RSUBTYPE_CMM_ACT,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_CMM_TABLE_ACT,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 433,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_CMM_ACT_HNDL_MIRROR,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_RID_REGFILE,
	.fdb_operand = BNXT_ULP_RF_IDX_RID,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.result_start_idx = 2080,
	.result_bit_size = 192,
	.result_num_fields = 18
	},
	{ /* class_tid: 4, , table: control.act_handle_to_offset_egr_pf_mirror */
	.description = "control.act_handle_to_offset_egr_pf_mirror",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 433,
		.cond_nums = 0 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.func_info = {
		.func_opc = BNXT_ULP_FUNC_OPC_HANDLE_TO_OFFSET,
		.func_src1 = BNXT_ULP_FUNC_SRC_REGFILE,
		.func_opr1 = BNXT_ULP_RF_IDX_CMM_ACT_HNDL_MIRROR,
		.func_src2 = BNXT_ULP_FUNC_SRC_CONST,
		.func_opr2 = 32,
		.func_dst_opr = BNXT_ULP_RF_IDX_MAIN_ACTION_PTR }
	},
	{ /* class_tid: 4, , table: mirror_tbl.wr_pf_egr */
	.description = "mirror_tbl.wr_pf_egr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_INDEX_TABLE,
	.resource_type = CFA_RSUBTYPE_IDX_TBL_MIRROR,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_INDEX_TABLE_NORMAL,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 433,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_MIRROR_PTR_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.track_type = CFA_TRACK_TYPE_SID,
	.result_start_idx = 2098,
	.result_bit_size = 128,
	.result_num_fields = 12
	},
	{ /* class_tid: 4, , table: shared_mirror_record.wr_pf_egr */
	.description = "shared_mirror_record.wr_pf_egr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_type = CFA_RSUBTYPE_IDX_TBL_MIRROR,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_SHARED_MIRROR,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 433,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_WRITE,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_INDEX,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.key_start_idx = 729,
	.blob_key_bit_size = 8,
	.key_bit_size = 8,
	.key_num_fields = 1,
	.result_start_idx = 2110,
	.result_bit_size = 101,
	.result_num_fields = 3
	},
	{ /* class_tid: 4, , table: control.save_mirror_id_egr */
	.description = "control.save_mirror_id_egr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 433,
		.cond_nums = 0 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.func_info = {
		.func_opc = BNXT_ULP_FUNC_OPC_PORT_TABLE,
		.func_oper_size = 16,
		.func_src1 = BNXT_ULP_FUNC_SRC_COMP_FIELD,
		.func_opr1 = BNXT_ULP_CF_IDX_DEV_PORT_ID,
		.func_src2 = BNXT_ULP_FUNC_SRC_REGFILE,
		.func_opr2 = BNXT_ULP_RF_IDX_MIRROR_PTR_0,
		.func_dst_opr = BNXT_ULP_PORT_TABLE_PHY_PORT_MIRROR_ID }
	},
	{ /* class_tid: 4, , table: cmm_full_act_record.egr_default_arec */
	.description = "cmm_full_act_record.egr_default_arec",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CMM_TABLE,
	.resource_type = CFA_RSUBTYPE_CMM_ACT,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_CMM_TABLE_ACT,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 433,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_CMM_ACT_HNDL,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.result_start_idx = 2113,
	.result_bit_size = 192,
	.result_num_fields = 18
	},
	{ /* class_tid: 4, , table: control.act_handle_to_offset_egr */
	.description = "control.act_handle_to_offset_egr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 433,
		.cond_nums = 0 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.func_info = {
		.func_opc = BNXT_ULP_FUNC_OPC_HANDLE_TO_OFFSET,
		.func_src1 = BNXT_ULP_FUNC_SRC_REGFILE,
		.func_opr1 = BNXT_ULP_RF_IDX_CMM_ACT_HNDL,
		.func_src2 = BNXT_ULP_FUNC_SRC_CONST,
		.func_opr2 = 32,
		.func_dst_opr = BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR }
	},
	{ /* class_tid: 4, , table: cmm_full_act_record.egr_default_roce_arec */
	.description = "cmm_full_act_record.egr_default_roce_arec",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CMM_TABLE,
	.resource_type = CFA_RSUBTYPE_CMM_ACT,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_CMM_TABLE_ACT,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 433,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_ALLOC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_CMM_ACT_HNDL,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.result_start_idx = 2131,
	.result_bit_size = 192,
	.result_num_fields = 18
	},
	{ /* class_tid: 4, , table: control.act_handle_to_offset_roce_egr */
	.description = "control.act_handle_to_offset_roce_egr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 433,
		.cond_nums = 0 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.func_info = {
		.func_opc = BNXT_ULP_FUNC_OPC_HANDLE_TO_OFFSET,
		.func_src1 = BNXT_ULP_FUNC_SRC_REGFILE,
		.func_opr1 = BNXT_ULP_RF_IDX_CMM_ACT_HNDL,
		.func_src2 = BNXT_ULP_FUNC_SRC_CONST,
		.func_opr2 = 32,
		.func_dst_opr = BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR_ROCE }
	},
	{ /* class_tid: 4, , table: port_table.egr_wr_0 */
	.description = "port_table.egr_wr_0",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_PORT_TABLE,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 433,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_WRITE,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_INDEX,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_PUSH_FID,
	.key_start_idx = 730,
	.blob_key_bit_size = 10,
	.key_bit_size = 10,
	.key_num_fields = 1,
	.result_start_idx = 2149,
	.result_bit_size = 185,
	.result_num_fields = 7
	},
	{ /* class_tid: 4, , table: control.do_nothing */
	.description = "control.do_nothing",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 0,
		.cond_false_goto = 0,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 433,
		.cond_nums = 0 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP
	},
	{ /* class_tid: 5, , table: shared_mirror_record.rd_pf_ing_op */
	.description = "shared_mirror_record.rd_pf_ing_op",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_type = TF_TBL_TYPE_MIRROR_CONFIG,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_SHARED_MIRROR,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 433,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_READ,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_INDEX,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.key_start_idx = 731,
	.blob_key_bit_size = 8,
	.key_bit_size = 8,
	.key_num_fields = 1,
	.ident_start_idx = 49,
	.ident_nums = 2
	},
	{ /* class_tid: 5, , table: control.rd_pf_mirror_op_ing */
	.description = "control.rd_pf_mirror_op_ing",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_RX,
	.true_message = "Reject: no PF mirrors found",
	.execute_info = {
		.cond_true_goto  = 1023,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 433,
		.cond_nums = 1 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP
	},
	{ /* class_tid: 5, , table: cmm_full_act_record.rd_ing_pf_mirror_ar */
	.description = "cmm_full_act_record.rd_ing_pf_mirror_ar",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CMM_TABLE,
	.resource_type = CFA_RSUBTYPE_CMM_ACT,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_CMM_TABLE_ACT,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 434,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_RD_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_CMM_ACT_HNDL_MIRROR,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.ident_start_idx = 51,
	.ident_nums = 17,
	.result_bit_size = 192
	},
	{ /* class_tid: 5, , table: cmm_full_act_record.wr_ing_pf_mirror_ar */
	.description = "cmm_full_act_record.wr_ing_pf_mirror_ar",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CMM_TABLE,
	.resource_type = CFA_RSUBTYPE_CMM_ACT,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_CMM_TABLE_ACT,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 434,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_CMM_ACT_HNDL_MIRROR,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.result_start_idx = 2156,
	.result_bit_size = 192,
	.result_num_fields = 18
	},
	{ /* class_tid: 5, , table: mirror_tbl.update_rd_ing_op */
	.description = "mirror_tbl.update_rd_ing_op",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_INDEX_TABLE,
	.resource_type = CFA_RSUBTYPE_IDX_TBL_MIRROR,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_INDEX_TABLE_NORMAL,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 434,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_RD_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_MIRROR_ID_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.track_type = CFA_TRACK_TYPE_SID,
	.ident_start_idx = 68,
	.ident_nums = 6,
	.result_bit_size = 128
	},
	{ /* class_tid: 5, , table: mirror_tbl.update_wr_ing_op */
	.description = "mirror_tbl.update_wr_ing_op",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_INDEX_TABLE,
	.resource_type = CFA_RSUBTYPE_IDX_TBL_MIRROR,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_INDEX_TABLE_NORMAL,
	.direction = TF_DIR_RX,
	.execute_info = {
		.cond_true_goto  = 0,
		.cond_false_goto = 0,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 434,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_MIRROR_ID_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.track_type = CFA_TRACK_TYPE_SID,
	.result_start_idx = 2174,
	.result_bit_size = 128,
	.result_num_fields = 12
	},
	{ /* class_tid: 6, , table: shared_mirror_record.rd_pf_egr_op */
	.description = "shared_mirror_record.rd_pf_egr_op",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_GENERIC_TABLE,
	.resource_type = TF_TBL_TYPE_MIRROR_CONFIG,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_GENERIC_TABLE_SHARED_MIRROR,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 434,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_GENERIC_TBL_OPC_READ,
	.gen_tbl_lkup_type = BNXT_ULP_GENERIC_TBL_LKUP_TYPE_INDEX,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.key_start_idx = 732,
	.blob_key_bit_size = 8,
	.key_bit_size = 8,
	.key_num_fields = 1,
	.ident_start_idx = 74,
	.ident_nums = 2
	},
	{ /* class_tid: 6, , table: control.rd_pf_mirror_op_egr */
	.description = "control.rd_pf_mirror_op_egr",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CTRL_TABLE,
	.direction = TF_DIR_TX,
	.true_message = "Reject: no PF mirrors found",
	.execute_info = {
		.cond_true_goto  = 1023,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_AND,
		.cond_start_idx = 434,
		.cond_nums = 1 },
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP
	},
	{ /* class_tid: 6, , table: cmm_full_act_record.rd_egr_pf_mirror_ar */
	.description = "cmm_full_act_record.rd_egr_pf_mirror_ar",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CMM_TABLE,
	.resource_type = CFA_RSUBTYPE_CMM_ACT,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_CMM_TABLE_ACT,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 435,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_RD_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_CMM_ACT_HNDL_MIRROR,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.ident_start_idx = 76,
	.ident_nums = 17,
	.result_bit_size = 192
	},
	{ /* class_tid: 6, , table: cmm_full_act_record.wr_egr_pf_mirror_ar */
	.description = "cmm_full_act_record.wr_egr_pf_mirror_ar",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_CMM_TABLE,
	.resource_type = CFA_RSUBTYPE_CMM_ACT,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_CMM_TABLE_ACT,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 435,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_CMM_ACT_HNDL_MIRROR,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.mark_db_opcode = BNXT_ULP_MARK_DB_OPC_NOP,
	.result_start_idx = 2186,
	.result_bit_size = 192,
	.result_num_fields = 18
	},
	{ /* class_tid: 6, , table: mirror_tbl.update_rd_egr_op */
	.description = "mirror_tbl.update_rd_egr_op",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_INDEX_TABLE,
	.resource_type = CFA_RSUBTYPE_IDX_TBL_MIRROR,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_INDEX_TABLE_NORMAL,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 1,
		.cond_false_goto = 1,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 435,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_RD_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_MIRROR_ID_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.track_type = CFA_TRACK_TYPE_SID,
	.ident_start_idx = 93,
	.ident_nums = 6,
	.result_bit_size = 128
	},
	{ /* class_tid: 6, , table: mirror_tbl.update_wr_egr_op */
	.description = "mirror_tbl.update_wr_egr_op",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_INDEX_TABLE,
	.resource_type = CFA_RSUBTYPE_IDX_TBL_MIRROR,
	.resource_sub_type =
		BNXT_ULP_RESOURCE_SUB_TYPE_INDEX_TABLE_NORMAL,
	.direction = TF_DIR_TX,
	.execute_info = {
		.cond_true_goto  = 0,
		.cond_false_goto = 0,
		.cond_list_opcode = BNXT_ULP_COND_LIST_OPC_TRUE,
		.cond_start_idx = 435,
		.cond_nums = 0 },
	.tbl_opcode = BNXT_ULP_INDEX_TBL_OPC_WR_REGFILE,
	.tbl_operand = BNXT_ULP_RF_IDX_MIRROR_ID_0,
	.key_recipe_opcode = BNXT_ULP_KEY_RECIPE_OPC_NOP,
	.fdb_opcode = BNXT_ULP_FDB_OPC_NOP,
	.track_type = CFA_TRACK_TYPE_SID,
	.result_start_idx = 2204,
	.result_bit_size = 128,
	.result_num_fields = 12
	}
};

struct bnxt_ulp_mapper_cond_list_info ulp_thor2_class_cond_oper_list[] = {
};

struct bnxt_ulp_mapper_cond_info ulp_thor2_class_cond_list[] = {
	/* cond_reject: thor2, class_tid: 1 */
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_T_VXLAN
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_SRV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_ICMP
	},
	/* cond_execute: class_tid: 1, control.mac_addr_cache_chk:3*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_RF_IS_SET,
	.cond_operand = BNXT_ULP_RF_IDX_GENERIC_TBL_MISS
	},
	/* cond_execute: class_tid: 1, control.roce_check:4*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	/* cond_execute: class_tid: 1, control.proto_header_cache_miss:5*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_RF_IS_SET,
	.cond_operand = BNXT_ULP_RF_IDX_GENERIC_TBL_MISS
	},
	/* field_cond: class_tid: 1, fkb_select.wc_fkb:6*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.wc_fkb:8*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.wc_fkb:10*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.wc_fkb:12*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.wc_fkb:14*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.wc_fkb:16*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.wc_fkb:18*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.wc_fkb:20*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.wc_fkb:22*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.wc_fkb:24*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.wc_fkb:26*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID
	},
	/* field_cond: class_tid: 1, fkb_select.wc_fkb:28*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID
	},
	/* field_cond: class_tid: 1, fkb_select.wc_fkb:30*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_ICMP
	},
	/* field_cond: class_tid: 1, fkb_select.wc_fkb:33*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_ICMP
	},
	/* field_cond: class_tid: 1, wm_key_recipe.wr:36*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR
	},
	/* field_cond: class_tid: 1, wm_key_recipe.wr:38*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR
	},
	/* field_cond: class_tid: 1, wm_key_recipe.wr:40*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 1, wm_key_recipe.wr:42*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 1, wm_key_recipe.wr:44*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR
	},
	/* field_cond: class_tid: 1, wm_key_recipe.wr:46*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR
	},
	/* field_cond: class_tid: 1, wm_key_recipe.wr:48*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 1, wm_key_recipe.wr:50*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 1, wm_key_recipe.wr:52*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID
	},
	/* field_cond: class_tid: 1, wm_key_recipe.wr:54*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID
	},
	/* field_cond: class_tid: 1, wm_key_recipe.wr:56*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID
	},
	/* field_cond: class_tid: 1, wm_key_recipe.wr:58*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID
	},
	/* field_cond: class_tid: 1, wm_key_recipe.wr:60*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_UDP
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_UDP_SRC_PORT
	},
	/* field_cond: class_tid: 1, wm_key_recipe.wr:62*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_TCP
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_TCP_SRC_PORT
	},
	/* field_cond: class_tid: 1, wm_key_recipe.wr:64*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_UDP
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_UDP_SRC_PORT
	},
	/* field_cond: class_tid: 1, wm_key_recipe.wr:66*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_TCP
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_TCP_SRC_PORT
	},
	/* field_cond: class_tid: 1, wm_key_recipe.wr:68*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_UDP
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_UDP_DST_PORT
	},
	/* field_cond: class_tid: 1, wm_key_recipe.wr:70*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_TCP
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_TCP_DST_PORT
	},
	/* field_cond: class_tid: 1, wm_key_recipe.wr:72*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_UDP
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_UDP_DST_PORT
	},
	/* field_cond: class_tid: 1, wm_key_recipe.wr:74*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_TCP
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_TCP_DST_PORT
	},
	/* field_cond: class_tid: 1, profile_tcam.wr:76*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_I_TCP
	},
	/* field_cond: class_tid: 1, profile_tcam.wr:77*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_I_UDP
	},
	/* field_cond: class_tid: 1, profile_tcam.wr:78*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_I_TCP
	},
	/* field_cond: class_tid: 1, profile_tcam.wr:79*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_I_UDP
	},
	/* field_cond: class_tid: 1, profile_tcam.wr:80*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	/* field_cond: class_tid: 1, profile_tcam.wr:81*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	/* field_cond: class_tid: 1, profile_tcam.wr:82*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	/* field_cond: class_tid: 1, profile_tcam.wr:83*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	/* field_cond: class_tid: 1, control.terminating_flow:84*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_TCP
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_TCP_SRC_PORT
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_TCP_DST_PORT
	},
	/* cond_execute: class_tid: 1, em_flow_conflict_cache.rd:91*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_CF_NOT_SET,
	.cond_operand = BNXT_ULP_CF_IDX_WC_MATCH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_RF_IS_SET,
	.cond_operand = BNXT_ULP_RF_IDX_TERM_FLOW
	},
	/* cond_execute: class_tid: 1, control.em_flow_conflict_cache_miss:93*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_RF_IS_SET,
	.cond_operand = BNXT_ULP_RF_IDX_GENERIC_TBL_MISS
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb:94*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb:96*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb:98*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb:100*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb:102*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb:104*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb:106*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb:108*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb:110*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb:112*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb:114*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb:116*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb:118*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_TCP
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_TCP_SRC_PORT
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb:120*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_UDP
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_UDP_SRC_PORT
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb:122*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_TCP
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_TCP_DST_PORT
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb:124*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_UDP
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_UDP_DST_PORT
	},
	/* field_cond: class_tid: 1, em_key_recipe.wr:126*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR
	},
	/* field_cond: class_tid: 1, em_key_recipe.wr:128*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR
	},
	/* field_cond: class_tid: 1, em_key_recipe.wr:130*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 1, em_key_recipe.wr:132*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 1, em_key_recipe.wr:134*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR
	},
	/* field_cond: class_tid: 1, em_key_recipe.wr:136*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR
	},
	/* field_cond: class_tid: 1, em_key_recipe.wr:138*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 1, em_key_recipe.wr:140*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 1, em_key_recipe.wr:142*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID
	},
	/* field_cond: class_tid: 1, em_key_recipe.wr:144*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID
	},
	/* field_cond: class_tid: 1, em_key_recipe.wr:146*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID
	},
	/* field_cond: class_tid: 1, em_key_recipe.wr:148*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID
	},
	/* field_cond: class_tid: 1, em_key_recipe.wr:150*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_TCP
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_TCP_SRC_PORT
	},
	/* field_cond: class_tid: 1, em_key_recipe.wr:152*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_UDP
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_UDP_SRC_PORT
	},
	/* field_cond: class_tid: 1, em_key_recipe.wr:154*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_TCP
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_TCP_SRC_PORT
	},
	/* field_cond: class_tid: 1, em_key_recipe.wr:156*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_UDP
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_UDP_SRC_PORT
	},
	/* field_cond: class_tid: 1, em_key_recipe.wr:158*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_TCP
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_TCP_DST_PORT
	},
	/* field_cond: class_tid: 1, em_key_recipe.wr:160*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_UDP
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_UDP_DST_PORT
	},
	/* field_cond: class_tid: 1, em_key_recipe.wr:162*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_TCP
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_TCP_DST_PORT
	},
	/* field_cond: class_tid: 1, em_key_recipe.wr:164*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_UDP
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_UDP_DST_PORT
	},
	/* cond_execute: class_tid: 1, control.field_sig_validation:166*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_RF_NOT_SET,
	.cond_operand = BNXT_ULP_RF_IDX_CC
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_RF_IS_SET,
	.cond_operand = BNXT_ULP_RF_IDX_FLOW_SIG_ID
	},
	/* cond_execute: class_tid: 1, em_normal.wr:168*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_ACT_BIT_NOT_SET,
	.cond_operand = BNXT_ULP_ACT_BIT_QUEUE
	},
	/* cond_execute: class_tid: 1, wm_normal.wr:169*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_ACT_BIT_NOT_SET,
	.cond_operand = BNXT_ULP_ACT_BIT_QUEUE
	},
	/* cond_execute: class_tid: 1, control.roce_proto_header_cache_miss:170*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_RF_IS_SET,
	.cond_operand = BNXT_ULP_RF_IDX_GENERIC_TBL_MISS
	},
	/* field_cond: class_tid: 1, fkb_select.roce_wc_fkb:171*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID
	},
	/* field_cond: class_tid: 1, fkb_select.roce_wc_fkb:173*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID
	},
	/* field_cond: class_tid: 1, fkb_select.roce_wc_fkb:175*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE
	},
	/* field_cond: class_tid: 1, fkb_select.roce_wc_fkb:177*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_BTH_FLAGS
	},
	/* field_cond: class_tid: 1, fkb_select.roce_wc_fkb:179*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN
	},
	/* field_cond: class_tid: 1, wm_key_recipe.roce_wr:181*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID
	},
	/* field_cond: class_tid: 1, wm_key_recipe.roce_wr:183*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID
	},
	/* field_cond: class_tid: 1, wm_key_recipe.roce_wr:185*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID
	},
	/* field_cond: class_tid: 1, wm_key_recipe.roce_wr:187*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID
	},
	/* field_cond: class_tid: 1, wm_key_recipe.roce_wr:189*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE
	},
	/* field_cond: class_tid: 1, wm_key_recipe.roce_wr:191*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE
	},
	/* field_cond: class_tid: 1, wm_key_recipe.roce_wr:193*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_BTH_FLAGS
	},
	/* field_cond: class_tid: 1, wm_key_recipe.roce_wr:195*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_BTH_FLAGS
	},
	/* field_cond: class_tid: 1, wm_key_recipe.roce_wr:197*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN
	},
	/* field_cond: class_tid: 1, wm_key_recipe.roce_wr:199*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN
	},
	/* field_cond: class_tid: 1, profile_tcam.roce_wr:201*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	/* field_cond: class_tid: 1, profile_tcam.roce_wr:202*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	/* field_cond: class_tid: 1, profile_tcam.roce_wr:203*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	/* field_cond: class_tid: 1, profile_tcam.roce_wr:204*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	/* field_cond: class_tid: 1, profile_tcam.roce_cnp_wr:205*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	/* field_cond: class_tid: 1, profile_tcam.roce_cnp_wr:206*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	/* field_cond: class_tid: 1, profile_tcam.roce_cnp_wr:207*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	/* field_cond: class_tid: 1, profile_tcam.roce_cnp_wr:208*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	/* field_cond: class_tid: 1, control.roce_terminating_flow:209*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 1, control.roce_terminating_flow:211*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR
	},
	/* field_cond: class_tid: 1, control.roce_terminating_flow:213*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* cond_execute: class_tid: 1, em_flow_conflict_cache.roce_rd:215*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_CF_NOT_SET,
	.cond_operand = BNXT_ULP_CF_IDX_WC_MATCH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_RF_IS_SET,
	.cond_operand = BNXT_ULP_RF_IDX_TERM_FLOW
	},
	/* cond_execute: class_tid: 1, control.roce_em_flow_conflict_cache_miss:217*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_RF_IS_SET,
	.cond_operand = BNXT_ULP_RF_IDX_GENERIC_TBL_MISS
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb_roce:218*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb_roce:220*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb_roce:222*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb_roce:224*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb_roce:226*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb_roce:228*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb_roce:230*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb_roce:232*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb_roce:234*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb_roce:236*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb_roce:238*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb_roce:240*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb_roce:242*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE
	},
	/* field_cond: class_tid: 1, fkb_select.em_fkb_roce:244*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN
	},
	/* field_cond: class_tid: 1, em_key_recipe.roce:246*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR
	},
	/* field_cond: class_tid: 1, em_key_recipe.roce:248*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR
	},
	/* field_cond: class_tid: 1, em_key_recipe.roce:250*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 1, em_key_recipe.roce:252*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 1, em_key_recipe.roce:254*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR
	},
	/* field_cond: class_tid: 1, em_key_recipe.roce:256*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR
	},
	/* field_cond: class_tid: 1, em_key_recipe.roce:258*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 1, em_key_recipe.roce:260*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 1, em_key_recipe.roce:262*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID
	},
	/* field_cond: class_tid: 1, em_key_recipe.roce:264*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID
	},
	/* field_cond: class_tid: 1, em_key_recipe.roce:266*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID
	},
	/* field_cond: class_tid: 1, em_key_recipe.roce:268*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID
	},
	/* field_cond: class_tid: 1, em_key_recipe.roce:270*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE
	},
	/* field_cond: class_tid: 1, em_key_recipe.roce:272*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE
	},
	/* field_cond: class_tid: 1, em_key_recipe.roce:274*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_BTH_FLAGS
	},
	/* field_cond: class_tid: 1, em_key_recipe.roce:276*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_BTH_FLAGS
	},
	/* field_cond: class_tid: 1, em_key_recipe.roce:278*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN
	},
	/* field_cond: class_tid: 1, em_key_recipe.roce:280*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN
	},
	/* cond_execute: class_tid: 1, control.roce_field_sig_validation:282*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_RF_NOT_SET,
	.cond_operand = BNXT_ULP_RF_IDX_CC
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_RF_IS_SET,
	.cond_operand = BNXT_ULP_RF_IDX_FLOW_SIG_ID
	},
	/* cond_execute: class_tid: 1, em_normal.roce:284*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_ACT_BIT_NOT_SET,
	.cond_operand = BNXT_ULP_ACT_BIT_QUEUE
	},
	/* cond_execute: class_tid: 1, wm_normal.roce:285*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_ACT_BIT_NOT_SET,
	.cond_operand = BNXT_ULP_ACT_BIT_QUEUE
	},
	/* cond_reject: thor2, class_tid: 2 */
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_ICMP
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_NOT_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	/* cond_execute: class_tid: 2, control.thor2_roce_check:288*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	/* cond_execute: class_tid: 2, control.l2_cache_chk:289*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_RF_IS_SET,
	.cond_operand = BNXT_ULP_RF_IDX_GENERIC_TBL_MISS
	},
	/* cond_execute: class_tid: 2, control.proto_header_cache_miss:290*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_RF_IS_SET,
	.cond_operand = BNXT_ULP_RF_IDX_GENERIC_TBL_MISS
	},
	/* field_cond: class_tid: 2, fkb_select.wc_fkb_roce:291*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_ETH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_ETH_DMAC
	},
	/* field_cond: class_tid: 2, fkb_select.wc_fkb_roce:293*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 2, fkb_select.wc_fkb_roce:295*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 2, fkb_select.wc_fkb_roce:297*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 2, fkb_select.wc_fkb_roce:299*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 2, fkb_select.wc_fkb_roce:301*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR
	},
	/* field_cond: class_tid: 2, fkb_select.wc_fkb_roce:303*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 2, fkb_select.wc_fkb_roce:305*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 2, fkb_select.wc_fkb_roce:307*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 2, fkb_select.wc_fkb_roce:309*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 2, fkb_select.wc_fkb_roce:311*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR
	},
	/* field_cond: class_tid: 2, fkb_select.wc_fkb_roce:313*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID
	},
	/* field_cond: class_tid: 2, fkb_select.wc_fkb_roce:315*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID
	},
	/* field_cond: class_tid: 2, fkb_select.wc_fkb_roce:317*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE
	},
	/* field_cond: class_tid: 2, fkb_select.wc_fkb_roce:319*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN
	},
	/* field_cond: class_tid: 2, wm_key_recipe.roce:321*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_ETH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_ETH_DMAC
	},
	/* field_cond: class_tid: 2, wm_key_recipe.roce:323*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_ETH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_ETH_DMAC
	},
	/* field_cond: class_tid: 2, wm_key_recipe.roce:325*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR
	},
	/* field_cond: class_tid: 2, wm_key_recipe.roce:327*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR
	},
	/* field_cond: class_tid: 2, wm_key_recipe.roce:329*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 2, wm_key_recipe.roce:331*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 2, wm_key_recipe.roce:333*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR
	},
	/* field_cond: class_tid: 2, wm_key_recipe.roce:335*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR
	},
	/* field_cond: class_tid: 2, wm_key_recipe.roce:337*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 2, wm_key_recipe.roce:339*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 2, wm_key_recipe.roce:341*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID
	},
	/* field_cond: class_tid: 2, wm_key_recipe.roce:343*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID
	},
	/* field_cond: class_tid: 2, wm_key_recipe.roce:345*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID
	},
	/* field_cond: class_tid: 2, wm_key_recipe.roce:347*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID
	},
	/* field_cond: class_tid: 2, wm_key_recipe.roce:349*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE
	},
	/* field_cond: class_tid: 2, wm_key_recipe.roce:351*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE
	},
	/* field_cond: class_tid: 2, wm_key_recipe.roce:353*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN
	},
	/* field_cond: class_tid: 2, wm_key_recipe.roce:355*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_WC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN
	},
	/* field_cond: class_tid: 2, profile_tcam.roce:357*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	/* field_cond: class_tid: 2, profile_tcam.roce:358*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	/* field_cond: class_tid: 2, profile_tcam.roce:359*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	/* field_cond: class_tid: 2, profile_tcam.roce:360*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	/* field_cond: class_tid: 2, control.roce_terminating_flow:361*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 2, control.roce_terminating_flow:363*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR
	},
	/* cond_execute: class_tid: 2, em_flow_conflict_cache.roce_rd:365*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_CF_NOT_SET,
	.cond_operand = BNXT_ULP_CF_IDX_WC_MATCH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_RF_IS_SET,
	.cond_operand = BNXT_ULP_RF_IDX_TERM_FLOW
	},
	/* cond_execute: class_tid: 2, control.roce_em_flow_conflict_cache_miss:367*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_RF_IS_SET,
	.cond_operand = BNXT_ULP_RF_IDX_GENERIC_TBL_MISS
	},
	/* field_cond: class_tid: 2, fkb_select.em_fkb_roce:368*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 2, fkb_select.em_fkb_roce:370*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 2, fkb_select.em_fkb_roce:372*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 2, fkb_select.em_fkb_roce:374*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 2, fkb_select.em_fkb_roce:376*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR
	},
	/* field_cond: class_tid: 2, fkb_select.em_fkb_roce:378*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 2, fkb_select.em_fkb_roce:380*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 2, fkb_select.em_fkb_roce:382*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 2, fkb_select.em_fkb_roce:384*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 2, fkb_select.em_fkb_roce:386*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR
	},
	/* field_cond: class_tid: 2, fkb_select.em_fkb_roce:388*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID
	},
	/* field_cond: class_tid: 2, fkb_select.em_fkb_roce:390*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID
	},
	/* field_cond: class_tid: 2, fkb_select.em_fkb_roce:392*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE
	},
	/* field_cond: class_tid: 2, fkb_select.em_fkb_roce:394*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN
	},
	/* field_cond: class_tid: 2, em_key_recipe.roce:396*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR
	},
	/* field_cond: class_tid: 2, em_key_recipe.roce:398*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR
	},
	/* field_cond: class_tid: 2, em_key_recipe.roce:400*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 2, em_key_recipe.roce:402*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR
	},
	/* field_cond: class_tid: 2, em_key_recipe.roce:404*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR
	},
	/* field_cond: class_tid: 2, em_key_recipe.roce:406*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR
	},
	/* field_cond: class_tid: 2, em_key_recipe.roce:408*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 2, em_key_recipe.roce:410*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR
	},
	/* field_cond: class_tid: 2, em_key_recipe.roce:412*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID
	},
	/* field_cond: class_tid: 2, em_key_recipe.roce:414*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID
	},
	/* field_cond: class_tid: 2, em_key_recipe.roce:416*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV6
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID
	},
	/* field_cond: class_tid: 2, em_key_recipe.roce:418*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_IPV4
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID
	},
	/* field_cond: class_tid: 2, em_key_recipe.roce:420*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE
	},
	/* field_cond: class_tid: 2, em_key_recipe.roce:422*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE
	},
	/* field_cond: class_tid: 2, em_key_recipe.roce:424*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN
	},
	/* field_cond: class_tid: 2, em_key_recipe.roce:426*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_HDR_BIT_IS_SET,
	.cond_operand = BNXT_ULP_HDR_BIT_O_BTH
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_FIELD_BIT_IS_SET,
	.cond_operand = BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN
	},
	/* cond_execute: class_tid: 2, control.roce_field_sig_validation:428*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_RF_NOT_SET,
	.cond_operand = BNXT_ULP_RF_IDX_CC
	},
	{
	.cond_opcode = BNXT_ULP_COND_OPC_RF_IS_SET,
	.cond_operand = BNXT_ULP_RF_IDX_FLOW_SIG_ID
	},
	/* cond_execute: class_tid: 4, control.pf_mirror_ing:430*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_RF_IS_SET,
	.cond_operand = BNXT_ULP_RF_IDX_GENERIC_TBL_MISS
	},
	/* cond_execute: class_tid: 4, cmm_stat_record.ing_pf_mirror:431*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_ACT_BIT_IS_SET,
	.cond_operand = BNXT_ULP_ACT_BIT_COUNT
	},
	/* cond_execute: class_tid: 4, control.pf_mirror_egr:432*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_RF_IS_SET,
	.cond_operand = BNXT_ULP_RF_IDX_GENERIC_TBL_MISS
	},
	/* cond_execute: class_tid: 5, control.rd_pf_mirror_op_ing:433*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_RF_IS_SET,
	.cond_operand = BNXT_ULP_RF_IDX_GENERIC_TBL_MISS
	},
	/* cond_execute: class_tid: 6, control.rd_pf_mirror_op_egr:434*/
	{
	.cond_opcode = BNXT_ULP_COND_OPC_RF_IS_SET,
	.cond_operand = BNXT_ULP_RF_IDX_GENERIC_TBL_MISS
	}
};

struct bnxt_ulp_mapper_key_info ulp_thor2_class_key_info_list[] = {
	/* class_tid: 1, , table: port_table.rd */
	{
	.field_info_mask = {
		.description = "dev.port_id",
		.field_bit_size = 10,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "dev.port_id",
		.field_bit_size = 10,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CF,
		.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff}
		}
	},
	/* class_tid: 1, , table: mac_addr_cache.rd */
	{
	.field_info_mask = {
		.description = "svif",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_SVIF_INDEX >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_SVIF_INDEX & 0xff}
		},
	.field_info_spec = {
		.description = "svif",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_SVIF_INDEX >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_SVIF_INDEX & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		ULP_THOR2_SYM_TUN_HDR_TYPE_NONE}
		},
	.field_info_spec = {
		.description = "tun_hdr",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		ULP_THOR2_SYM_TUN_HDR_TYPE_NONE}
		}
	},
	{
	.field_info_mask = {
		.description = "one_tag",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "one_tag",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "mac_addr",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1_THEN_SRC2_ELSE_SRC3,
		.field_src1 = BNXT_ULP_FIELD_SRC_FIELD_BIT,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_O_ETH_DMAC >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_ETH_DMAC & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_ETH_DMAC >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_ETH_DMAC & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr3 = {
		(BNXT_ULP_RF_IDX_DRV_FUNC_MAC >> 8) & 0xff,
		BNXT_ULP_RF_IDX_DRV_FUNC_MAC & 0xff}
		},
	.field_info_spec = {
		.description = "mac_addr",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1_THEN_SRC2_ELSE_SRC3,
		.field_src1 = BNXT_ULP_FIELD_SRC_FIELD_BIT,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_O_ETH_DMAC >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_ETH_DMAC & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_ETH_DMAC >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_ETH_DMAC & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr3 = {
		(BNXT_ULP_RF_IDX_DRV_FUNC_MAC >> 8) & 0xff,
		BNXT_ULP_RF_IDX_DRV_FUNC_MAC & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "etype",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "etype",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "recycle_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "recycle_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tbl_scope",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tbl_scope",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_filter_id",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_filter_id",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	/* class_tid: 1, , table: l2_cntxt_tcam.l2_table_create */
	{
	.field_info_mask = {
		.description = "etype",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "etype",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ivlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ivlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ivlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ivlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ovlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ovlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ovlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ovlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "addr1",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "addr1",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "addr0",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "addr0",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1_THEN_SRC2_ELSE_SRC3,
		.field_src1 = BNXT_ULP_FIELD_SRC_FIELD_BIT,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_O_ETH_DMAC >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_ETH_DMAC & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_ETH_DMAC >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_ETH_DMAC & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr3 = {
		(BNXT_ULP_RF_IDX_DRV_FUNC_MAC >> 8) & 0xff,
		BNXT_ULP_RF_IDX_DRV_FUNC_MAC & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "tunnel_id",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tunnel_id",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "out_tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "out_tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "llc",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "llc",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "roce",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "roce",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2ip_func",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2ip_func",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "svif",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_SVIF_INDEX >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_SVIF_INDEX & 0xff}
		},
	.field_info_spec = {
		.description = "svif",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_SVIF_INDEX >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_SVIF_INDEX & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "parif",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "parif",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "spif",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "spif",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "recycle_cnt",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "recycle_cnt",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "mpass_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		3}
		},
	.field_info_spec = {
		.description = "mpass_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		},
	.field_info_spec = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		}
	},
	/* class_tid: 1, , table: l2_cntxt_tcam.roce_l2_table_create */
	{
	.field_info_mask = {
		.description = "etype",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "etype",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ivlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ivlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ivlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ivlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ovlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ovlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ovlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ovlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "addr1",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "addr1",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "addr0",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "addr0",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1_THEN_SRC2_ELSE_SRC3,
		.field_src1 = BNXT_ULP_FIELD_SRC_FIELD_BIT,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_O_ETH_DMAC >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_ETH_DMAC & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_ETH_DMAC >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_ETH_DMAC & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr3 = {
		(BNXT_ULP_RF_IDX_DRV_FUNC_MAC >> 8) & 0xff,
		BNXT_ULP_RF_IDX_DRV_FUNC_MAC & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "tunnel_id",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tunnel_id",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "out_tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "out_tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "llc",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "llc",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "roce",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "roce",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		}
	},
	{
	.field_info_mask = {
		.description = "l2ip_func",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2ip_func",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "svif",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_SVIF_INDEX >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_SVIF_INDEX & 0xff}
		},
	.field_info_spec = {
		.description = "svif",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_SVIF_INDEX >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_SVIF_INDEX & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "parif",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "parif",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "spif",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "spif",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "recycle_cnt",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "recycle_cnt",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "mpass_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		3}
		},
	.field_info_spec = {
		.description = "mpass_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		},
	.field_info_spec = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		}
	},
	/* class_tid: 1, , table: l2_cntxt_tcam.roce_cnp_l2_table_create */
	{
	.field_info_mask = {
		.description = "etype",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "etype",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ivlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ivlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ivlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ivlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ovlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ovlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ovlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ovlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "addr1",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "addr1",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "addr0",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "addr0",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1_THEN_SRC2_ELSE_SRC3,
		.field_src1 = BNXT_ULP_FIELD_SRC_FIELD_BIT,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_O_ETH_DMAC >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_ETH_DMAC & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_ETH_DMAC >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_ETH_DMAC & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr3 = {
		(BNXT_ULP_RF_IDX_DRV_FUNC_MAC >> 8) & 0xff,
		BNXT_ULP_RF_IDX_DRV_FUNC_MAC & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "tunnel_id",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		(65535 >> 24) & 0xff,
		(65535 >> 16) & 0xff,
		(65535 >> 8) & 0xff,
		65535 & 0xff}
		},
	.field_info_spec = {
		.description = "tunnel_id",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		(129 >> 24) & 0xff,
		(129 >> 16) & 0xff,
		(129 >> 8) & 0xff,
		129 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "out_tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "out_tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "llc",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "llc",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "roce",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "roce",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		}
	},
	{
	.field_info_mask = {
		.description = "l2ip_func",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2ip_func",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "svif",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_SVIF_INDEX >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_SVIF_INDEX & 0xff}
		},
	.field_info_spec = {
		.description = "svif",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_SVIF_INDEX >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_SVIF_INDEX & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "parif",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "parif",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "spif",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "spif",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "recycle_cnt",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "recycle_cnt",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "mpass_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		3}
		},
	.field_info_spec = {
		.description = "mpass_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		},
	.field_info_spec = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		}
	},
	/* class_tid: 1, , table: l2_cntxt_tcam.roce_l2_table_create_lpbk */
	{
	.field_info_mask = {
		.description = "etype",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "etype",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ivlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ivlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ivlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ivlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ovlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ovlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ovlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ovlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "addr1",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "addr1",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "addr0",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "addr0",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1_THEN_SRC2_ELSE_SRC3,
		.field_src1 = BNXT_ULP_FIELD_SRC_FIELD_BIT,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_O_ETH_DMAC >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_ETH_DMAC & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_ETH_DMAC >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_ETH_DMAC & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr3 = {
		(BNXT_ULP_RF_IDX_DRV_FUNC_MAC >> 8) & 0xff,
		BNXT_ULP_RF_IDX_DRV_FUNC_MAC & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "tunnel_id",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tunnel_id",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "out_tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "out_tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "llc",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "llc",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "roce",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "roce",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		}
	},
	{
	.field_info_mask = {
		.description = "l2ip_func",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2ip_func",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "svif",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "svif",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "parif",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "parif",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "spif",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "spif",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		}
	},
	{
	.field_info_mask = {
		.description = "recycle_cnt",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "recycle_cnt",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "mpass_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		3}
		},
	.field_info_spec = {
		.description = "mpass_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		},
	.field_info_spec = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		}
	},
	/* class_tid: 1, , table: profile_tcam.catchall */
	{
	.field_info_mask = {
		.description = "l4_hdr_dcn_present",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_dcn_present",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_flags",
		.field_bit_size = 9,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_flags",
		.field_bit_size = 9,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_subtype",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_subtype",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_protocol",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l3_protocol",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_err",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_err",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_is_tcp_udp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_is_tcp_udp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "int_ifa_tail",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "int_ifa_tail",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "int_hdr_group",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "int_hdr_group",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "int_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "int_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "hrec_next",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "hrec_next",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l2ip_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2ip_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "agg_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "agg_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "recycle_count",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "recycle_count",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "pkt_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "pkt_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		},
	.field_info_spec = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		}
	},
	{
	.field_info_mask = {
		.description = "padding",
		.field_bit_size = 72,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "padding",
		.field_bit_size = 72,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	/* class_tid: 1, , table: profile_tcam.cnp_catchall */
	{
	.field_info_mask = {
		.description = "l4_hdr_dcn_present",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_dcn_present",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_flags",
		.field_bit_size = 9,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_flags",
		.field_bit_size = 9,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_subtype",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_subtype",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_protocol",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l3_protocol",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_err",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_err",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_is_tcp_udp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_is_tcp_udp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "int_ifa_tail",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "int_ifa_tail",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "int_hdr_group",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "int_hdr_group",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "int_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "int_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "hrec_next",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "hrec_next",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_PROF_FUNC_ID_1 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_PROF_FUNC_ID_1 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l2ip_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2ip_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "agg_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "agg_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "recycle_count",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "recycle_count",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "pkt_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "pkt_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		},
	.field_info_spec = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		}
	},
	{
	.field_info_mask = {
		.description = "padding",
		.field_bit_size = 72,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "padding",
		.field_bit_size = 72,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	/* class_tid: 1, , table: mac_addr_cache.wr */
	{
	.field_info_mask = {
		.description = "svif",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_SVIF_INDEX >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_SVIF_INDEX & 0xff}
		},
	.field_info_spec = {
		.description = "svif",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_SVIF_INDEX >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_SVIF_INDEX & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		ULP_THOR2_SYM_TUN_HDR_TYPE_NONE}
		},
	.field_info_spec = {
		.description = "tun_hdr",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		ULP_THOR2_SYM_TUN_HDR_TYPE_NONE}
		}
	},
	{
	.field_info_mask = {
		.description = "one_tag",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "one_tag",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "mac_addr",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1_THEN_SRC2_ELSE_SRC3,
		.field_src1 = BNXT_ULP_FIELD_SRC_FIELD_BIT,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_O_ETH_DMAC >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_ETH_DMAC & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_ETH_DMAC >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_ETH_DMAC & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr3 = {
		(BNXT_ULP_RF_IDX_DRV_FUNC_MAC >> 8) & 0xff,
		BNXT_ULP_RF_IDX_DRV_FUNC_MAC & 0xff}
		},
	.field_info_spec = {
		.description = "mac_addr",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1_THEN_SRC2_ELSE_SRC3,
		.field_src1 = BNXT_ULP_FIELD_SRC_FIELD_BIT,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_O_ETH_DMAC >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_ETH_DMAC & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_ETH_DMAC >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_ETH_DMAC & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr3 = {
		(BNXT_ULP_RF_IDX_DRV_FUNC_MAC >> 8) & 0xff,
		BNXT_ULP_RF_IDX_DRV_FUNC_MAC & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "etype",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "etype",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "recycle_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "recycle_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tbl_scope",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tbl_scope",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_filter_id",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_filter_id",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	/* class_tid: 1, , table: proto_header_cache.rd */
	{
	.field_info_mask = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CF,
		.field_opr1 = {
		(BNXT_ULP_CF_IDX_PROFILE_BITMAP >> 8) & 0xff,
		BNXT_ULP_CF_IDX_PROFILE_BITMAP & 0xff}
		}
	},
	/* class_tid: 1, , table: wm_key_recipe.wr */
	{
	.field_info_mask = {
		.description = "wc_profile_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "wc_profile_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_WC_PROFILE_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_WC_PROFILE_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l2_cntxt_id",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "l2_cntxt_id",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l3.sip.ipv4",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(36 >> 8) & 0xff,
			36 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l3.sip.ipv4",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(38 >> 8) & 0xff,
			38 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l3.sip.ipv6",
		.field_bit_size = 128,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(40 >> 8) & 0xff,
			40 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l3.sip.ipv6",
		.field_bit_size = 128,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(42 >> 8) & 0xff,
			42 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l3.dip.ipv4",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(44 >> 8) & 0xff,
			44 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l3.dip.ipv4",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(46 >> 8) & 0xff,
			46 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l3.dip.ipv6",
		.field_bit_size = 128,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(48 >> 8) & 0xff,
			48 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l3.dip.ipv6",
		.field_bit_size = 128,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(50 >> 8) & 0xff,
			50 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(52 >> 8) & 0xff,
			52 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(3 >> 8) & 0xff,
		3 & 0xff}
		},
	.field_info_spec = {
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(56 >> 8) & 0xff,
			56 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(4 >> 8) & 0xff,
		4 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l4.src",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(60 >> 8) & 0xff,
			60 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_UDP_SRC_PORT >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_UDP_SRC_PORT & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(5 >> 8) & 0xff,
		5 & 0xff}
		},
	.field_info_spec = {
		.description = "l4.src",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(64 >> 8) & 0xff,
			64 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_UDP_SRC_PORT >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_UDP_SRC_PORT & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(6 >> 8) & 0xff,
		6 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l4.dst",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(68 >> 8) & 0xff,
			68 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_UDP_DST_PORT >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_UDP_DST_PORT & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(7 >> 8) & 0xff,
		7 & 0xff}
		},
	.field_info_spec = {
		.description = "l4.dst",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(72 >> 8) & 0xff,
			72 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_UDP_DST_PORT >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_UDP_DST_PORT & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(8 >> 8) & 0xff,
		8 & 0xff}
		}
	},
	/* class_tid: 1, , table: profile_tcam.wr */
	{
	.field_info_mask = {
		.description = "l4_hdr_dcn_present",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_dcn_present",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_flags",
		.field_bit_size = 9,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_flags",
		.field_bit_size = 9,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_subtype",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(76 >> 8) & 0xff,
			76 & 0xff,
			(1 >> 8) & 0xff,
			1 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_ONES,
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(9 >> 8) & 0xff,
		9 & 0xff}
		},
	.field_info_spec = {
		.description = "l4_hdr_subtype",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(78 >> 8) & 0xff,
			78 & 0xff,
			(1 >> 8) & 0xff,
			1 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_ZERO,
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(10 >> 8) & 0xff,
		10 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		ULP_THOR2_SYM_L4_HDR_VALID_YES}
		}
	},
	{
	.field_info_mask = {
		.description = "l3_protocol",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l3_protocol",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(80 >> 8) & 0xff,
			80 & 0xff,
			(1 >> 8) & 0xff,
			1 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_ONES,
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(11 >> 8) & 0xff,
		11 & 0xff}
		},
	.field_info_spec = {
		.description = "l3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(82 >> 8) & 0xff,
			82 & 0xff,
			(1 >> 8) & 0xff,
			1 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr2 = {
		ULP_THOR2_SYM_L3_HDR_TYPE_IPV6},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(12 >> 8) & 0xff,
		12 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		ULP_THOR2_SYM_L3_HDR_VALID_YES}
		}
	},
	{
	.field_info_mask = {
		.description = "l2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l2_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		ULP_THOR2_SYM_L2_HDR_VALID_YES}
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_err",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_err",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_is_tcp_udp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_is_tcp_udp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "int_ifa_tail",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "int_ifa_tail",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "int_hdr_group",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "int_hdr_group",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "int_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "int_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "hrec_next",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "hrec_next",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l2ip_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2ip_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "agg_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "agg_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "recycle_count",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "recycle_count",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "pkt_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "pkt_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		},
	.field_info_spec = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		}
	},
	{
	.field_info_mask = {
		.description = "padding",
		.field_bit_size = 72,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "padding",
		.field_bit_size = 72,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	/* class_tid: 1, , table: proto_header_cache.wr */
	{
	.field_info_mask = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CF,
		.field_opr1 = {
		(BNXT_ULP_CF_IDX_PROFILE_BITMAP >> 8) & 0xff,
		BNXT_ULP_CF_IDX_PROFILE_BITMAP & 0xff}
		}
	},
	/* class_tid: 1, , table: em_flow_conflict_cache.rd */
	{
	.field_info_mask = {
		.description = "recycle_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "recycle_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CF,
		.field_opr1 = {
		(BNXT_ULP_CF_IDX_PROFILE_BITMAP >> 8) & 0xff,
		BNXT_ULP_CF_IDX_PROFILE_BITMAP & 0xff}
		}
	},
	/* class_tid: 1, , table: em_key_recipe.wr */
	{
	.field_info_mask = {
		.description = "em_profile_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "em_profile_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_EM_PROFILE_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_EM_PROFILE_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l2_cntxt_id",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "l2_cntxt_id",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l3.sip.ipv4",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(126 >> 8) & 0xff,
			126 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l3.sip.ipv4",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(128 >> 8) & 0xff,
			128 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l3.sip.ipv6",
		.field_bit_size = 128,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(130 >> 8) & 0xff,
			130 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l3.sip.ipv6",
		.field_bit_size = 128,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(132 >> 8) & 0xff,
			132 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l3.dip.ipv4",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(134 >> 8) & 0xff,
			134 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l3.dip.ipv4",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(136 >> 8) & 0xff,
			136 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l3.dip.ipv6",
		.field_bit_size = 128,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(138 >> 8) & 0xff,
			138 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l3.dip.ipv6",
		.field_bit_size = 128,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(140 >> 8) & 0xff,
			140 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(142 >> 8) & 0xff,
			142 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(19 >> 8) & 0xff,
		19 & 0xff}
		},
	.field_info_spec = {
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(146 >> 8) & 0xff,
			146 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(20 >> 8) & 0xff,
		20 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l4.src",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(150 >> 8) & 0xff,
			150 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_TCP_SRC_PORT >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_TCP_SRC_PORT & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(21 >> 8) & 0xff,
		21 & 0xff}
		},
	.field_info_spec = {
		.description = "l4.src",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(154 >> 8) & 0xff,
			154 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_TCP_SRC_PORT >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_TCP_SRC_PORT & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(22 >> 8) & 0xff,
		22 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l4.dst",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(158 >> 8) & 0xff,
			158 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_TCP_DST_PORT >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_TCP_DST_PORT & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(23 >> 8) & 0xff,
		23 & 0xff}
		},
	.field_info_spec = {
		.description = "l4.dst",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(162 >> 8) & 0xff,
			162 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_TCP_DST_PORT >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_TCP_DST_PORT & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(24 >> 8) & 0xff,
		24 & 0xff}
		}
	},
	/* class_tid: 1, , table: em_flow_conflict_cache.wr */
	{
	.field_info_mask = {
		.description = "recycle_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "recycle_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CF,
		.field_opr1 = {
		(BNXT_ULP_CF_IDX_PROFILE_BITMAP >> 8) & 0xff,
		BNXT_ULP_CF_IDX_PROFILE_BITMAP & 0xff}
		}
	},
	/* class_tid: 1, , table: proto_header_cache.roce_rd */
	{
	.field_info_mask = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CF,
		.field_opr1 = {
		(BNXT_ULP_CF_IDX_PROFILE_BITMAP >> 8) & 0xff,
		BNXT_ULP_CF_IDX_PROFILE_BITMAP & 0xff}
		}
	},
	/* class_tid: 1, , table: wm_key_recipe.roce_wr */
	{
	.field_info_mask = {
		.description = "wc_profile_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "wc_profile_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_WC_PROFILE_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_WC_PROFILE_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l2_cntxt_id",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "l2_cntxt_id",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(181 >> 8) & 0xff,
			181 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(26 >> 8) & 0xff,
		26 & 0xff}
		},
	.field_info_spec = {
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(185 >> 8) & 0xff,
			185 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(27 >> 8) & 0xff,
		27 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l4.dst",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(189 >> 8) & 0xff,
			189 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_CONST
		},
	.field_info_spec = {
		.description = "l4.dst",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(191 >> 8) & 0xff,
			191 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_CONST
		}
	},
	{
	.field_info_mask = {
		.description = "l4.flags",
		.field_bit_size = 9,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(193 >> 8) & 0xff,
			193 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_BTH_BTH_FLAGS >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_BTH_BTH_FLAGS & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l4.flags",
		.field_bit_size = 9,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(195 >> 8) & 0xff,
			195 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_BTH_BTH_FLAGS >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_BTH_BTH_FLAGS & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l4.ack",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(197 >> 8) & 0xff,
			197 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l4.ack",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(199 >> 8) & 0xff,
			199 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	/* class_tid: 1, , table: profile_tcam.roce_wr */
	{
	.field_info_mask = {
		.description = "l4_hdr_dcn_present",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_dcn_present",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_flags",
		.field_bit_size = 9,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_flags",
		.field_bit_size = 9,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_subtype",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l4_hdr_subtype",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		ULP_THOR2_SYM_L4_HDR_SUBTYPE_ROCE}
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		ULP_THOR2_SYM_L4_HDR_VALID_YES}
		}
	},
	{
	.field_info_mask = {
		.description = "l3_protocol",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l3_protocol",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(201 >> 8) & 0xff,
			201 & 0xff,
			(1 >> 8) & 0xff,
			1 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_ONES,
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(28 >> 8) & 0xff,
		28 & 0xff}
		},
	.field_info_spec = {
		.description = "l3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(203 >> 8) & 0xff,
			203 & 0xff,
			(1 >> 8) & 0xff,
			1 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr2 = {
		ULP_THOR2_SYM_L3_HDR_TYPE_IPV6},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(29 >> 8) & 0xff,
		29 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		ULP_THOR2_SYM_L3_HDR_VALID_YES}
		}
	},
	{
	.field_info_mask = {
		.description = "l2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l2_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		ULP_THOR2_SYM_L2_HDR_VALID_YES}
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_err",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_err",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_is_tcp_udp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_is_tcp_udp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "int_ifa_tail",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "int_ifa_tail",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "int_hdr_group",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "int_hdr_group",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "int_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "int_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "hrec_next",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "hrec_next",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l2ip_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2ip_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "agg_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "agg_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "recycle_count",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "recycle_count",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "pkt_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "pkt_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		},
	.field_info_spec = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		}
	},
	{
	.field_info_mask = {
		.description = "padding",
		.field_bit_size = 72,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "padding",
		.field_bit_size = 72,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	/* class_tid: 1, , table: profile_tcam.roce_cnp_wr */
	{
	.field_info_mask = {
		.description = "l4_hdr_dcn_present",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_dcn_present",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_flags",
		.field_bit_size = 9,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_flags",
		.field_bit_size = 9,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_subtype",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l4_hdr_subtype",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		ULP_THOR2_SYM_L4_HDR_SUBTYPE_ROCE}
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		ULP_THOR2_SYM_L4_HDR_VALID_YES}
		}
	},
	{
	.field_info_mask = {
		.description = "l3_protocol",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l3_protocol",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(205 >> 8) & 0xff,
			205 & 0xff,
			(1 >> 8) & 0xff,
			1 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_ONES,
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(30 >> 8) & 0xff,
		30 & 0xff}
		},
	.field_info_spec = {
		.description = "l3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(207 >> 8) & 0xff,
			207 & 0xff,
			(1 >> 8) & 0xff,
			1 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr2 = {
		ULP_THOR2_SYM_L3_HDR_TYPE_IPV6},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(31 >> 8) & 0xff,
		31 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		ULP_THOR2_SYM_L3_HDR_VALID_YES}
		}
	},
	{
	.field_info_mask = {
		.description = "l2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l2_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		ULP_THOR2_SYM_L2_HDR_VALID_YES}
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_err",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_err",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_is_tcp_udp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_is_tcp_udp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "int_ifa_tail",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "int_ifa_tail",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "int_hdr_group",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "int_hdr_group",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "int_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "int_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "hrec_next",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "hrec_next",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_PROF_FUNC_ID_1 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_PROF_FUNC_ID_1 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l2ip_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2ip_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "agg_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "agg_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "recycle_count",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "recycle_count",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "pkt_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "pkt_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		},
	.field_info_spec = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		}
	},
	{
	.field_info_mask = {
		.description = "padding",
		.field_bit_size = 72,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "padding",
		.field_bit_size = 72,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	/* class_tid: 1, , table: proto_header_cache.roce_wr */
	{
	.field_info_mask = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CF,
		.field_opr1 = {
		(BNXT_ULP_CF_IDX_PROFILE_BITMAP >> 8) & 0xff,
		BNXT_ULP_CF_IDX_PROFILE_BITMAP & 0xff}
		}
	},
	/* class_tid: 1, , table: em_flow_conflict_cache.roce_rd */
	{
	.field_info_mask = {
		.description = "recycle_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "recycle_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CF,
		.field_opr1 = {
		(BNXT_ULP_CF_IDX_PROFILE_BITMAP >> 8) & 0xff,
		BNXT_ULP_CF_IDX_PROFILE_BITMAP & 0xff}
		}
	},
	/* class_tid: 1, , table: em_key_recipe.roce */
	{
	.field_info_mask = {
		.description = "em_profile_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "em_profile_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_EM_PROFILE_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_EM_PROFILE_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l2_cntxt_id",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "l2_cntxt_id",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l3.sip.ipv4",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(246 >> 8) & 0xff,
			246 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l3.sip.ipv4",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(248 >> 8) & 0xff,
			248 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l3.sip.ipv6",
		.field_bit_size = 128,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(250 >> 8) & 0xff,
			250 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l3.sip.ipv6",
		.field_bit_size = 128,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(252 >> 8) & 0xff,
			252 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l3.dip.ipv4",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(254 >> 8) & 0xff,
			254 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l3.dip.ipv4",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(256 >> 8) & 0xff,
			256 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l3.dip.ipv6",
		.field_bit_size = 128,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(258 >> 8) & 0xff,
			258 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l3.dip.ipv6",
		.field_bit_size = 128,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(260 >> 8) & 0xff,
			260 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(262 >> 8) & 0xff,
			262 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(38 >> 8) & 0xff,
		38 & 0xff}
		},
	.field_info_spec = {
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(266 >> 8) & 0xff,
			266 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(39 >> 8) & 0xff,
		39 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l4.dst",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(270 >> 8) & 0xff,
			270 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l4.dst",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(272 >> 8) & 0xff,
			272 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l4.flags",
		.field_bit_size = 9,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(274 >> 8) & 0xff,
			274 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_BTH_BTH_FLAGS >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_BTH_BTH_FLAGS & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l4.flags",
		.field_bit_size = 9,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(276 >> 8) & 0xff,
			276 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_BTH_BTH_FLAGS >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_BTH_BTH_FLAGS & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l4.ack",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(278 >> 8) & 0xff,
			278 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l4.ack",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(280 >> 8) & 0xff,
			280 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	/* class_tid: 1, , table: em_flow_conflict_cache.roce_wr */
	{
	.field_info_mask = {
		.description = "recycle_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "recycle_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CF,
		.field_opr1 = {
		(BNXT_ULP_CF_IDX_PROFILE_BITMAP >> 8) & 0xff,
		BNXT_ULP_CF_IDX_PROFILE_BITMAP & 0xff}
		}
	},
	/* class_tid: 2, , table: port_table.egr_wr_0 */
	{
	.field_info_mask = {
		.description = "dev.port_id",
		.field_bit_size = 10,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "dev.port_id",
		.field_bit_size = 10,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CF,
		.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff}
		}
	},
	/* class_tid: 2, , table: l2_cntxt_tcam_cache.egr_rd */
	{
	.field_info_mask = {
		.description = "svif",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_SVIF_INDEX >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_SVIF_INDEX & 0xff}
		},
	.field_info_spec = {
		.description = "svif",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_SVIF_INDEX >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_SVIF_INDEX & 0xff}
		}
	},
	/* class_tid: 2, , table: l2_cntxt_tcam.l2_table_create */
	{
	.field_info_mask = {
		.description = "etype",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "etype",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ivlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ivlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ivlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ivlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ovlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ovlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ovlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ovlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "addr1",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "addr1",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "addr0",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "addr0",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tunnel_id",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tunnel_id",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "out_tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "out_tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "llc",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "llc",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "roce",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "roce",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2ip_func",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2ip_func",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "svif",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_SVIF_INDEX >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_SVIF_INDEX & 0xff}
		},
	.field_info_spec = {
		.description = "svif",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_SVIF_INDEX >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_SVIF_INDEX & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "parif",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "parif",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "spif",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "spif",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "recycle_cnt",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "recycle_cnt",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "mpass_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "mpass_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		},
	.field_info_spec = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		}
	},
	/* class_tid: 2, , table: l2_cntxt_tcam.roce_table_create */
	{
	.field_info_mask = {
		.description = "etype",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "etype",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ivlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ivlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ivlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ivlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ovlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ovlan_tpid_sel",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_ovlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_ovlan_vid",
		.field_bit_size = 12,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "addr1",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "addr1",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "addr0",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "addr0",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tunnel_id",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tunnel_id",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "out_tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "out_tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "llc",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "llc",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "roce",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "roce",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		}
	},
	{
	.field_info_mask = {
		.description = "l2ip_func",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2ip_func",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "svif",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_SVIF_INDEX >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_SVIF_INDEX & 0xff}
		},
	.field_info_spec = {
		.description = "svif",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_SVIF_INDEX >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_SVIF_INDEX & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "parif",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "parif",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "spif",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "spif",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "recycle_cnt",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "recycle_cnt",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "mpass_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "mpass_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		},
	.field_info_spec = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		}
	},
	/* class_tid: 2, , table: profile_tcam.catchall */
	{
	.field_info_mask = {
		.description = "l4_hdr_dcn_present",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_dcn_present",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_flags",
		.field_bit_size = 9,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_flags",
		.field_bit_size = 9,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_subtype",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_subtype",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_protocol",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l3_protocol",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_err",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_err",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_is_tcp_udp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_is_tcp_udp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "int_ifa_tail",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "int_ifa_tail",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "int_hdr_group",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "int_hdr_group",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "int_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "int_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "hrec_next",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "hrec_next",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l2ip_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2ip_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "agg_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "agg_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "recycle_count",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "recycle_count",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "pkt_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "pkt_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		},
	.field_info_spec = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		}
	},
	{
	.field_info_mask = {
		.description = "padding",
		.field_bit_size = 72,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "padding",
		.field_bit_size = 72,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	/* class_tid: 2, , table: l2_cntxt_tcam_cache.egr_wr */
	{
	.field_info_mask = {
		.description = "svif",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_SVIF_INDEX >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_SVIF_INDEX & 0xff}
		},
	.field_info_spec = {
		.description = "svif",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr1 = {
		(BNXT_ULP_GLB_HF_ID_SVIF_INDEX >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_SVIF_INDEX & 0xff}
		}
	},
	/* class_tid: 2, , table: proto_header_cache.roce_rd */
	{
	.field_info_mask = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CF,
		.field_opr1 = {
		(BNXT_ULP_CF_IDX_PROFILE_BITMAP >> 8) & 0xff,
		BNXT_ULP_CF_IDX_PROFILE_BITMAP & 0xff}
		}
	},
	/* class_tid: 2, , table: wm_key_recipe.roce */
	{
	.field_info_mask = {
		.description = "wc_profile_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "wc_profile_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_WC_PROFILE_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_WC_PROFILE_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l2_cntxt_id",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "l2_cntxt_id",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l2_dmac",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(321 >> 8) & 0xff,
			321 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_ETH_DMAC >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_ETH_DMAC & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l2_dmac",
		.field_bit_size = 48,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(323 >> 8) & 0xff,
			323 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_ETH_DMAC >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_ETH_DMAC & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l3.sip.ipv4",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(325 >> 8) & 0xff,
			325 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l3.sip.ipv4",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(327 >> 8) & 0xff,
			327 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l3.sip.ipv6",
		.field_bit_size = 128,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(329 >> 8) & 0xff,
			329 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l3.sip.ipv6",
		.field_bit_size = 128,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(331 >> 8) & 0xff,
			331 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l3.dip.ipv4",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(333 >> 8) & 0xff,
			333 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l3.dip.ipv4",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(335 >> 8) & 0xff,
			335 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l3.dip.ipv6",
		.field_bit_size = 128,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(337 >> 8) & 0xff,
			337 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l3.dip.ipv6",
		.field_bit_size = 128,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(339 >> 8) & 0xff,
			339 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(341 >> 8) & 0xff,
			341 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(43 >> 8) & 0xff,
		43 & 0xff}
		},
	.field_info_spec = {
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(345 >> 8) & 0xff,
			345 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(44 >> 8) & 0xff,
		44 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l4.dst",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(349 >> 8) & 0xff,
			349 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l4.dst",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(351 >> 8) & 0xff,
			351 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l4.ack",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(353 >> 8) & 0xff,
			353 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l4.ack",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(355 >> 8) & 0xff,
			355 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	/* class_tid: 2, , table: profile_tcam.roce */
	{
	.field_info_mask = {
		.description = "l4_hdr_dcn_present",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_dcn_present",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_flags",
		.field_bit_size = 9,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_flags",
		.field_bit_size = 9,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_subtype",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l4_hdr_subtype",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		ULP_THOR2_SYM_L4_HDR_SUBTYPE_ROCE}
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		ULP_THOR2_SYM_L4_HDR_VALID_YES}
		}
	},
	{
	.field_info_mask = {
		.description = "l3_protocol",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l3_protocol",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(357 >> 8) & 0xff,
			357 & 0xff,
			(1 >> 8) & 0xff,
			1 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_ONES,
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(45 >> 8) & 0xff,
		45 & 0xff}
		},
	.field_info_spec = {
		.description = "l3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(359 >> 8) & 0xff,
			359 & 0xff,
			(1 >> 8) & 0xff,
			1 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr2 = {
		ULP_THOR2_SYM_L3_HDR_TYPE_IPV6},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(46 >> 8) & 0xff,
		46 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		ULP_THOR2_SYM_L3_HDR_VALID_YES}
		}
	},
	{
	.field_info_mask = {
		.description = "l2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l2_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "l2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "l2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		ULP_THOR2_SYM_L2_HDR_VALID_YES}
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_err",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tun_hdr_err",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tun_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "tun_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_is_udp_tcp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "tl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "tl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "tl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "tl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "tl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_flags",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_type",
		.field_bit_size = 5,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "ot_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "ot_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_is_tcp_udp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_is_tcp_udp",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl4_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_isIP",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl3_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_two_vtags",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_vtag_present",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_uc_mc_bc",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "otl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "otl2_hdr_valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "int_ifa_tail",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "int_ifa_tail",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "int_hdr_group",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "int_hdr_group",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "int_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "int_hdr_type",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "hrec_next",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "hrec_next",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l2ip_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "l2ip_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "agg_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "agg_error",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "metadata",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "recycle_count",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "recycle_count",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "pkt_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "pkt_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "loopback",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "spare",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		},
	.field_info_spec = {
		.description = "valid",
		.field_bit_size = 1,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr1 = {
		1}
		}
	},
	{
	.field_info_mask = {
		.description = "padding",
		.field_bit_size = 72,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "padding",
		.field_bit_size = 72,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	/* class_tid: 2, , table: proto_header_cache.roce_wr */
	{
	.field_info_mask = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CF,
		.field_opr1 = {
		(BNXT_ULP_CF_IDX_PROFILE_BITMAP >> 8) & 0xff,
		BNXT_ULP_CF_IDX_PROFILE_BITMAP & 0xff}
		}
	},
	/* class_tid: 2, , table: em_flow_conflict_cache.roce_rd */
	{
	.field_info_mask = {
		.description = "recycle_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "recycle_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CF,
		.field_opr1 = {
		(BNXT_ULP_CF_IDX_PROFILE_BITMAP >> 8) & 0xff,
		BNXT_ULP_CF_IDX_PROFILE_BITMAP & 0xff}
		}
	},
	/* class_tid: 2, , table: em_key_recipe.roce */
	{
	.field_info_mask = {
		.description = "em_profile_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "em_profile_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_EM_PROFILE_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_EM_PROFILE_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l2_cntxt_id",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "l2_cntxt_id",
		.field_bit_size = 11,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l3.sip.ipv4",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(396 >> 8) & 0xff,
			396 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l3.sip.ipv4",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(398 >> 8) & 0xff,
			398 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_SRC_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l3.sip.ipv6",
		.field_bit_size = 128,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(400 >> 8) & 0xff,
			400 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l3.sip.ipv6",
		.field_bit_size = 128,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(402 >> 8) & 0xff,
			402 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_SRC_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l3.dip.ipv4",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(404 >> 8) & 0xff,
			404 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l3.dip.ipv4",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(406 >> 8) & 0xff,
			406 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_DST_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l3.dip.ipv6",
		.field_bit_size = 128,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(408 >> 8) & 0xff,
			408 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l3.dip.ipv6",
		.field_bit_size = 128,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(410 >> 8) & 0xff,
			410 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_DST_ADDR & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(412 >> 8) & 0xff,
			412 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(52 >> 8) & 0xff,
		52 & 0xff}
		},
	.field_info_spec = {
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(416 >> 8) & 0xff,
			416 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV6_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
		.field_opr3 = {
		(53 >> 8) & 0xff,
		53 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "l4.dst",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(420 >> 8) & 0xff,
			420 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l4.dst",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(422 >> 8) & 0xff,
			422 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_BTH_OP_CODE & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	{
	.field_info_mask = {
		.description = "l4.ack",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(424 >> 8) & 0xff,
			424 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	.field_info_spec = {
		.description = "l4.ack",
		.field_bit_size = 32,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(426 >> 8) & 0xff,
			426 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_BTH_DST_QPN & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
	},
	/* class_tid: 2, , table: em_flow_conflict_cache.roce_wr */
	{
	.field_info_mask = {
		.description = "recycle_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "recycle_cnt",
		.field_bit_size = 2,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		},
	.field_info_spec = {
		.description = "group_metadata",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
		}
	},
	{
	.field_info_mask = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "prof_func_id",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_RF,
		.field_opr1 = {
		(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
		BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
		}
	},
	{
	.field_info_mask = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "hdr_bitmap",
		.field_bit_size = 64,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CF,
		.field_opr1 = {
		(BNXT_ULP_CF_IDX_PROFILE_BITMAP >> 8) & 0xff,
		BNXT_ULP_CF_IDX_PROFILE_BITMAP & 0xff}
		}
	},
	/* class_tid: 4, , table: shared_mirror_record.rd_pf_ing */
	{
	.field_info_mask = {
		.description = "shared_index",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "shared_index",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CF,
		.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff}
		}
	},
	/* class_tid: 4, , table: shared_mirror_record.wr_pf_ing */
	{
	.field_info_mask = {
		.description = "shared_index",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "shared_index",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CF,
		.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff}
		}
	},
	/* class_tid: 4, , table: port_table.ing_wr_0 */
	{
	.field_info_mask = {
		.description = "dev.port_id",
		.field_bit_size = 10,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "dev.port_id",
		.field_bit_size = 10,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CF,
		.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff}
		}
	},
	/* class_tid: 4, , table: shared_mirror_record.rd_pf_egr */
	{
	.field_info_mask = {
		.description = "shared_index",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "shared_index",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CF,
		.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff}
		}
	},
	/* class_tid: 4, , table: shared_mirror_record.wr_pf_egr */
	{
	.field_info_mask = {
		.description = "shared_index",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "shared_index",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CF,
		.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff}
		}
	},
	/* class_tid: 4, , table: port_table.egr_wr_0 */
	{
	.field_info_mask = {
		.description = "dev.port_id",
		.field_bit_size = 10,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff,
			0xff}
		},
	.field_info_spec = {
		.description = "dev.port_id",
		.field_bit_size = 10,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CF,
		.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff}
		}
	},
	/* class_tid: 5, , table: shared_mirror_record.rd_pf_ing_op */
	{
	.field_info_mask = {
		.description = "shared_index",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "shared_index",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CF,
		.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff}
		}
	},
	/* class_tid: 6, , table: shared_mirror_record.rd_pf_egr_op */
	{
	.field_info_mask = {
		.description = "shared_index",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_ONES,
		.field_opr1 = {
			0xff}
		},
	.field_info_spec = {
		.description = "shared_index",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
		.field_src1 = BNXT_ULP_FIELD_SRC_CF,
		.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff}
		}
	}
};

struct bnxt_ulp_mapper_field_info ulp_thor2_class_key_ext_list[] = {
	{
		.description = "l3_sip0.en",
		.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(14 >> 8) & 0xff,
		14 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3_dip0.en",
		.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(24 >> 8) & 0xff,
		24 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3_prot.en",
		.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(28 >> 8) & 0xff,
		28 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(54 >> 8) & 0xff,
			54 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	{
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(58 >> 8) & 0xff,
			58 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	{
		.description = "l4.src",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(62 >> 8) & 0xff,
			62 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_TCP_SRC_PORT >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_TCP_SRC_PORT & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_CONST
		},
	{
		.description = "l4.src",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(66 >> 8) & 0xff,
			66 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_TCP_SRC_PORT >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_TCP_SRC_PORT & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_CONST
		},
	{
		.description = "l4.dst",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(70 >> 8) & 0xff,
			70 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_TCP_DST_PORT >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_TCP_DST_PORT & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_CONST
		},
	{
		.description = "l4.dst",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(74 >> 8) & 0xff,
			74 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_TCP_DST_PORT >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_TCP_DST_PORT & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_CONST
		},
	{
		.description = "l4_hdr_subtype",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(77 >> 8) & 0xff,
			77 & 0xff,
			(1 >> 8) & 0xff,
			1 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_ONES,
		.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l4_hdr_subtype",
		.field_bit_size = 3,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(79 >> 8) & 0xff,
			79 & 0xff,
			(1 >> 8) & 0xff,
			1 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
		.field_opr2 = {
		ULP_THOR2_SYM_L4_HDR_TYPE_UDP},
		.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(81 >> 8) & 0xff,
			81 & 0xff,
			(1 >> 8) & 0xff,
			1 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_ONES,
		.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(83 >> 8) & 0xff,
			83 & 0xff,
			(1 >> 8) & 0xff,
			1 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_ZERO,
		.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	/* class_tid: 1, , table: control.terminating_flow */
	{
	.description = "",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(84 >> 8) & 0xff,
		84 & 0xff,
		(7 >> 8) & 0xff,
		7 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1}
	},
	{
		.description = "l3_sip0.en",
		.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(102 >> 8) & 0xff,
		102 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3_dip0.en",
		.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(112 >> 8) & 0xff,
		112 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3_prot.en",
		.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(116 >> 8) & 0xff,
		116 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l4_src.en",
		.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(120 >> 8) & 0xff,
		120 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l4_dst.en",
		.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(124 >> 8) & 0xff,
		124 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(144 >> 8) & 0xff,
			144 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	{
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(148 >> 8) & 0xff,
			148 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	{
		.description = "l4.src",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(152 >> 8) & 0xff,
			152 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_UDP_SRC_PORT >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_UDP_SRC_PORT & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	{
		.description = "l4.src",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(156 >> 8) & 0xff,
			156 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_UDP_SRC_PORT >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_UDP_SRC_PORT & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	{
		.description = "l4.dst",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(160 >> 8) & 0xff,
			160 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_UDP_DST_PORT >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_UDP_DST_PORT & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	{
		.description = "l4.dst",
		.field_bit_size = 16,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(164 >> 8) & 0xff,
			164 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_UDP_DST_PORT >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_UDP_DST_PORT & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	{
		.description = "l3_prot.en",
		.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(173 >> 8) & 0xff,
		173 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(183 >> 8) & 0xff,
			183 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	{
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(187 >> 8) & 0xff,
			187 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	{
		.description = "l3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(202 >> 8) & 0xff,
			202 & 0xff,
			(1 >> 8) & 0xff,
			1 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_ONES,
		.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(204 >> 8) & 0xff,
			204 & 0xff,
			(1 >> 8) & 0xff,
			1 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_ZERO,
		.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(206 >> 8) & 0xff,
			206 & 0xff,
			(1 >> 8) & 0xff,
			1 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_ONES,
		.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(208 >> 8) & 0xff,
			208 & 0xff,
			(1 >> 8) & 0xff,
			1 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_ZERO,
		.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	/* class_tid: 1, , table: control.roce_terminating_flow */
	{
	.description = "",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(209 >> 8) & 0xff,
		209 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
	.field_opr3 = {
	(33 >> 8) & 0xff,
	33 & 0xff}
	},
	{
		.description = "",
		.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(211 >> 8) & 0xff,
		211 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
	.field_opr3 = {
	(34 >> 8) & 0xff,
	34 & 0xff}
		},
	{
		.description = "",
		.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(213 >> 8) & 0xff,
		213 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3_sip0.en",
		.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(226 >> 8) & 0xff,
		226 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3_dip0.en",
		.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(236 >> 8) & 0xff,
		236 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3_prot.en",
		.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(240 >> 8) & 0xff,
		240 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(264 >> 8) & 0xff,
			264 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	{
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(268 >> 8) & 0xff,
			268 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	{
		.description = "l3_sip0.en",
		.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(301 >> 8) & 0xff,
		301 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3_dip0.en",
		.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(311 >> 8) & 0xff,
		311 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3_prot.en",
		.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(315 >> 8) & 0xff,
		315 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(343 >> 8) & 0xff,
			343 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	{
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(347 >> 8) & 0xff,
			347 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	{
		.description = "l3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(358 >> 8) & 0xff,
			358 & 0xff,
			(1 >> 8) & 0xff,
			1 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_ONES,
		.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3_hdr_type",
		.field_bit_size = 4,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(360 >> 8) & 0xff,
			360 & 0xff,
			(1 >> 8) & 0xff,
			1 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_ZERO,
		.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	/* class_tid: 2, , table: control.roce_terminating_flow */
	{
	.description = "",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(361 >> 8) & 0xff,
		361 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
	.field_opr3 = {
	(48 >> 8) & 0xff,
	48 & 0xff}
	},
	{
		.description = "",
		.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(363 >> 8) & 0xff,
		363 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3_sip0.en",
		.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(376 >> 8) & 0xff,
		376 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3_dip0.en",
		.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(386 >> 8) & 0xff,
		386 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3_prot.en",
		.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(390 >> 8) & 0xff,
		390 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
		},
	{
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(414 >> 8) & 0xff,
			414 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		},
	{
		.description = "l3.prot",
		.field_bit_size = 8,
		.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
		.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
		.field_opr1 = {
			(418 >> 8) & 0xff,
			418 & 0xff,
			(2 >> 8) & 0xff,
			2 & 0xff},
		.field_src2 = BNXT_ULP_FIELD_SRC_HF,
		.field_opr2 = {
		(BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID >> 8) & 0xff,
		BNXT_ULP_GLB_HF_ID_O_IPV4_PROTO_ID & 0xff},
		.field_src3 = BNXT_ULP_FIELD_SRC_SKIP
		}
};

struct bnxt_ulp_mapper_field_info ulp_thor2_class_result_field_list[] = {
	/* class_tid: 1, , table: l2_cntxt_tcam.l2_table_create */
	{
	.description = "l2ip_dest_data",
	.field_bit_size = 17,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_dest_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_rfs_data",
	.field_bit_size = 9,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_rfs_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_act_rec_ptr",
	.field_bit_size = 26,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR >> 8) & 0xff,
	BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR & 0xff}
	},
	{
	.description = "l2ip_act_scope",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_PORT_TABLE,
	.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff,
		(BNXT_ULP_PORT_TABLE_TABLE_SCOPE >> 8) & 0xff,
		BNXT_ULP_PORT_TABLE_TABLE_SCOPE & 0xff}
	},
	{
	.description = "l2ip_act_hint",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_act_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "l2ip_meta",
	.field_bit_size = 35,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_meta_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "ctxt_opcode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	ULP_THOR2_SYM_CTXT_OPCODE_NORMAL_FLOW}
	},
	{
	.description = "prof_func_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
	},
	{
	.description = "prsv_prof_func_id",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_cntxt_id",
	.field_bit_size = 11,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 & 0xff}
	},
	{
	.description = "prsv_l2ip_cntxt_id",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "parif",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "prsv_parif",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 1, , table: l2_cntxt_tcam.roce_l2_table_create */
	{
	.description = "l2ip_dest_data",
	.field_bit_size = 17,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_dest_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_rfs_data",
	.field_bit_size = 9,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_rfs_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_act_rec_ptr",
	.field_bit_size = 26,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR_ROCE >> 8) & 0xff,
	BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR_ROCE & 0xff}
	},
	{
	.description = "l2ip_act_scope",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_PORT_TABLE,
	.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff,
		(BNXT_ULP_PORT_TABLE_TABLE_SCOPE >> 8) & 0xff,
		BNXT_ULP_PORT_TABLE_TABLE_SCOPE & 0xff}
	},
	{
	.description = "l2ip_act_hint",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_act_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "l2ip_meta",
	.field_bit_size = 35,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_meta_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "ctxt_opcode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	ULP_THOR2_SYM_CTXT_OPCODE_NORMAL_FLOW}
	},
	{
	.description = "prof_func_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
	},
	{
	.description = "prsv_prof_func_id",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_cntxt_id",
	.field_bit_size = 11,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 & 0xff}
	},
	{
	.description = "prsv_l2ip_cntxt_id",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "parif",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "prsv_parif",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 1, , table: l2_cntxt_tcam.roce_cnp_l2_table_create */
	{
	.description = "l2ip_dest_data",
	.field_bit_size = 17,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_dest_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_rfs_data",
	.field_bit_size = 9,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_rfs_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_act_rec_ptr",
	.field_bit_size = 26,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR_ROCE >> 8) & 0xff,
	BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR_ROCE & 0xff}
	},
	{
	.description = "l2ip_act_scope",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_PORT_TABLE,
	.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff,
		(BNXT_ULP_PORT_TABLE_TABLE_SCOPE >> 8) & 0xff,
		BNXT_ULP_PORT_TABLE_TABLE_SCOPE & 0xff}
	},
	{
	.description = "l2ip_act_hint",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_act_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "l2ip_meta",
	.field_bit_size = 35,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_meta_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "ctxt_opcode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	ULP_THOR2_SYM_CTXT_OPCODE_NORMAL_FLOW}
	},
	{
	.description = "prof_func_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PROF_FUNC_ID_1 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PROF_FUNC_ID_1 & 0xff}
	},
	{
	.description = "prsv_prof_func_id",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_cntxt_id",
	.field_bit_size = 11,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 & 0xff}
	},
	{
	.description = "prsv_l2ip_cntxt_id",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "parif",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "prsv_parif",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 1, , table: l2_cntxt_tcam.roce_l2_table_create_lpbk */
	{
	.description = "l2ip_dest_data",
	.field_bit_size = 17,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_dest_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_rfs_data",
	.field_bit_size = 9,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_rfs_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_act_rec_ptr",
	.field_bit_size = 26,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR_ROCE >> 8) & 0xff,
	BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR_ROCE & 0xff}
	},
	{
	.description = "l2ip_act_scope",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_PORT_TABLE,
	.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff,
		(BNXT_ULP_PORT_TABLE_TABLE_SCOPE >> 8) & 0xff,
		BNXT_ULP_PORT_TABLE_TABLE_SCOPE & 0xff}
	},
	{
	.description = "l2ip_act_hint",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_act_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "l2ip_meta",
	.field_bit_size = 35,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_meta_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "ctxt_opcode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	ULP_THOR2_SYM_CTXT_OPCODE_BYPASS_LKUP}
	},
	{
	.description = "prof_func_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
	},
	{
	.description = "prsv_prof_func_id",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_cntxt_id",
	.field_bit_size = 11,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 & 0xff}
	},
	{
	.description = "prsv_l2ip_cntxt_id",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "parif",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "prsv_parif",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 1, , table: profile_tcam.catchall */
	{
	.description = "wc_scope",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "wc_key_id",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "wc_profile_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "wc_search_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "em_scope",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "em_key_id",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "em_profile_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "em_search_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "pl_byp_lkup_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "padding",
	.field_bit_size = 21,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 1, , table: profile_tcam.cnp_catchall */
	{
	.description = "wc_scope",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "wc_key_id",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "wc_profile_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "wc_search_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "em_scope",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "em_key_id",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "em_profile_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "em_search_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "pl_byp_lkup_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "padding",
	.field_bit_size = 21,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 1, , table: mac_addr_cache.wr */
	{
	.description = "rid",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_RID >> 8) & 0xff,
	BNXT_ULP_RF_IDX_RID & 0xff}
	},
	{
	.description = "l2_cntxt_tcam_index",
	.field_bit_size = 11,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PROF_FUNC_ID_1 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PROF_FUNC_ID_1 & 0xff}
	},
	{
	.description = "l2_cntxt_id",
	.field_bit_size = 11,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 & 0xff}
	},
	{
	.description = "src_property_ptr",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "prof_func_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
	},
	/* class_tid: 1, , table: fkb_select.em_fkb_alloc */
	{
	.description = "l2_cntxt_id.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_func.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "parif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "spif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "svif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "lcos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meta_hi.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meta_lo.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "rcyc_cnt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "loopback.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otuntype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otflags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otids.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otctxts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otctxt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otqos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "oterr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tuntype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tflags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tids.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tctxts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tctxt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tqos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "terr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_ack.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_win.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_tsval.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_txecr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "padding",
	.field_bit_size = 85,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 1, , table: fkb_select.wc_fkb */
	{
	.description = "l2_cntxt_id.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "l2ip_func.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "parif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "spif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "svif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "lcos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meta_hi.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meta_lo.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "rcyc_cnt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "loopback.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otuntype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otflags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otids.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otctxts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otctxt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otqos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "oterr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tuntype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tflags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tids.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tctxts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tctxt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tqos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "terr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(6 >> 8) & 0xff,
		6 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(8 >> 8) & 0xff,
		8 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(10 >> 8) & 0xff,
		10 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(12 >> 8) & 0xff,
		12 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_NEXT
	},
	{
	.description = "l3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(16 >> 8) & 0xff,
		16 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(18 >> 8) & 0xff,
		18 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(20 >> 8) & 0xff,
		20 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(22 >> 8) & 0xff,
		22 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
	.field_opr3 = {
	(1 >> 8) & 0xff,
	1 & 0xff}
	},
	{
	.description = "l3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(26 >> 8) & 0xff,
		26 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
	.field_opr3 = {
	(2 >> 8) & 0xff,
	2 & 0xff}
	},
	{
	.description = "l3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_OR,
	.field_opr1 = {
		(30 >> 8) & 0xff,
		30 & 0xff,
		(3 >> 8) & 0xff,
		3 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_OR,
	.field_opr1 = {
		(33 >> 8) & 0xff,
		33 & 0xff,
		(3 >> 8) & 0xff,
		3 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_ack.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_win.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_tsval.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_txecr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "padding",
	.field_bit_size = 85,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 1, , table: profile_tcam.wr */
	{
	.description = "wc_scope",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_PORT_TABLE,
	.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff,
		(BNXT_ULP_PORT_TABLE_TABLE_SCOPE >> 8) & 0xff,
		BNXT_ULP_PORT_TABLE_TABLE_SCOPE & 0xff}
	},
	{
	.description = "wc_key_id",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_WC_KEY_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_WC_KEY_ID_0 & 0xff}
	},
	{
	.description = "wc_profile_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_WC_PROFILE_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_WC_PROFILE_ID_0 & 0xff}
	},
	{
	.description = "wc_search_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "em_scope",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_PORT_TABLE,
	.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff,
		(BNXT_ULP_PORT_TABLE_TABLE_SCOPE >> 8) & 0xff,
		BNXT_ULP_PORT_TABLE_TABLE_SCOPE & 0xff}
	},
	{
	.description = "em_key_id",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_EM_KEY_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_EM_KEY_ID_0 & 0xff}
	},
	{
	.description = "em_profile_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_EM_PROFILE_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_EM_PROFILE_ID_0 & 0xff}
	},
	{
	.description = "em_search_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "pl_byp_lkup_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "padding",
	.field_bit_size = 21,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 1, , table: proto_header_cache.wr */
	{
	.description = "rid",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_RID >> 8) & 0xff,
	BNXT_ULP_RF_IDX_RID & 0xff}
	},
	{
	.description = "profile_tcam_index",
	.field_bit_size = 10,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PROFILE_TCAM_INDEX_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PROFILE_TCAM_INDEX_0 & 0xff}
	},
	{
	.description = "em_profile_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_EM_PROFILE_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_EM_PROFILE_ID_0 & 0xff}
	},
	{
	.description = "em_key_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_EM_KEY_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_EM_KEY_ID_0 & 0xff}
	},
	{
	.description = "wc_profile_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_WC_PROFILE_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_WC_PROFILE_ID_0 & 0xff}
	},
	{
	.description = "wc_key_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_WC_KEY_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_WC_KEY_ID_0 & 0xff}
	},
	{
	.description = "em_recipe_id",
	.field_bit_size = 16,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_EM_RECIPE_ID >> 8) & 0xff,
	BNXT_ULP_RF_IDX_EM_RECIPE_ID & 0xff}
	},
	{
	.description = "wc_recipe_id",
	.field_bit_size = 16,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_WC_RECIPE_ID >> 8) & 0xff,
	BNXT_ULP_RF_IDX_WC_RECIPE_ID & 0xff}
	},
	/* class_tid: 1, , table: fkb_select.em_fkb */
	{
	.description = "l2_cntxt_id.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "l2ip_func.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "parif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "spif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "svif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "lcos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meta_hi.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meta_lo.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "rcyc_cnt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "loopback.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otuntype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otflags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otids.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otctxts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otctxt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otqos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "oterr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tuntype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tflags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tids.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tctxts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tctxt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tqos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "terr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(94 >> 8) & 0xff,
		94 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(96 >> 8) & 0xff,
		96 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(98 >> 8) & 0xff,
		98 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(100 >> 8) & 0xff,
		100 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
	.field_opr3 = {
	(14 >> 8) & 0xff,
	14 & 0xff}
	},
	{
	.description = "l3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(104 >> 8) & 0xff,
		104 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(106 >> 8) & 0xff,
		106 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(108 >> 8) & 0xff,
		108 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(110 >> 8) & 0xff,
		110 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
	.field_opr3 = {
	(15 >> 8) & 0xff,
	15 & 0xff}
	},
	{
	.description = "l3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(114 >> 8) & 0xff,
		114 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
	.field_opr3 = {
	(16 >> 8) & 0xff,
	16 & 0xff}
	},
	{
	.description = "l3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(118 >> 8) & 0xff,
		118 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
	.field_opr3 = {
	(17 >> 8) & 0xff,
	17 & 0xff}
	},
	{
	.description = "l4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(122 >> 8) & 0xff,
		122 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
	.field_opr3 = {
	(18 >> 8) & 0xff,
	18 & 0xff}
	},
	{
	.description = "l4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_ack.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_win.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_tsval.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_txecr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "padding",
	.field_bit_size = 85,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 1, , table: em_flow_conflict_cache.wr */
	{
	.description = "rid",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_RID >> 8) & 0xff,
	BNXT_ULP_RF_IDX_RID & 0xff}
	},
	{
	.description = "flow_sig_id",
	.field_bit_size = 64,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CF,
	.field_opr1 = {
	(BNXT_ULP_CF_IDX_FLOW_SIG_ID >> 8) & 0xff,
	BNXT_ULP_CF_IDX_FLOW_SIG_ID & 0xff}
	},
	/* class_tid: 1, , table: em_normal.wr */
	{
	.description = "valid",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "rec_size",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "epoch0",
	.field_bit_size = 12,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CF,
	.field_opr1 = {
	(BNXT_ULP_CF_IDX_FUNCTION_ID >> 8) & 0xff,
	BNXT_ULP_CF_IDX_FUNCTION_ID & 0xff}
	},
	{
	.description = "epoch1",
	.field_bit_size = 6,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "opcode",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "strength",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	3}
	},
	{
	.description = "act_hint",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "act_rec_ptr",
	.field_bit_size = 26,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MAIN_ACTION_PTR >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MAIN_ACTION_PTR & 0xff}
	},
	{
	.description = "ring_table_idx",
	.field_bit_size = 9,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "act_rec_size",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "paths_m1",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_op",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "pad2",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "range_profile",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "range_index",
	.field_bit_size = 16,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 1, , table: em_normal.queue_wr */
	{
	.description = "valid",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "rec_size",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "epoch0",
	.field_bit_size = 12,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CF,
	.field_opr1 = {
	(BNXT_ULP_CF_IDX_FUNCTION_ID >> 8) & 0xff,
	BNXT_ULP_CF_IDX_FUNCTION_ID & 0xff}
	},
	{
	.description = "epoch1",
	.field_bit_size = 6,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "opcode",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "strength",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	3}
	},
	{
	.description = "act_hint",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "act_rec_ptr",
	.field_bit_size = 26,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MAIN_ACTION_PTR >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MAIN_ACTION_PTR & 0xff}
	},
	{
	.description = "ring_table_idx",
	.field_bit_size = 9,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "act_rec_size",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "paths_m1",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_op",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "pad2",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "range_profile",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "range_index",
	.field_bit_size = 16,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 1, , table: wm_normal.wr */
	{
	.description = "fc_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_op",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "paths_m1",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "act_rec_size",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "ring_table_idx",
	.field_bit_size = 9,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "act_rec_ptr",
	.field_bit_size = 26,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MAIN_ACTION_PTR >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MAIN_ACTION_PTR & 0xff}
	},
	{
	.description = "act_hint",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "strength",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	2}
	},
	{
	.description = "opcode",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "epoch1",
	.field_bit_size = 6,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "epoch0",
	.field_bit_size = 12,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CF,
	.field_opr1 = {
	(BNXT_ULP_CF_IDX_FUNCTION_ID >> 8) & 0xff,
	BNXT_ULP_CF_IDX_FUNCTION_ID & 0xff}
	},
	{
	.description = "rec_size",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "valid",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "pad1",
	.field_bit_size = 24,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 1, , table: wm_normal.queue_wr */
	{
	.description = "fc_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_op",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "paths_m1",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "act_rec_size",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "ring_table_idx",
	.field_bit_size = 9,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ACT_PROP,
	.field_opr1 = {
	(BNXT_ULP_ACT_PROP_IDX_QUEUE_INDEX >> 8) & 0xff,
	BNXT_ULP_ACT_PROP_IDX_QUEUE_INDEX & 0xff}
	},
	{
	.description = "act_rec_ptr",
	.field_bit_size = 26,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MAIN_ACTION_PTR >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MAIN_ACTION_PTR & 0xff}
	},
	{
	.description = "act_hint",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "strength",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	2}
	},
	{
	.description = "opcode",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "epoch1",
	.field_bit_size = 6,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "epoch0",
	.field_bit_size = 12,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CF,
	.field_opr1 = {
	(BNXT_ULP_CF_IDX_FUNCTION_ID >> 8) & 0xff,
	BNXT_ULP_CF_IDX_FUNCTION_ID & 0xff}
	},
	{
	.description = "rec_size",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "valid",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "pad1",
	.field_bit_size = 24,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 1, , table: fkb_select.em_fkb_roce_alloc */
	{
	.description = "l2_cntxt_id.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_func.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "parif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "spif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "svif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "lcos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meta_hi.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meta_lo.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "rcyc_cnt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "loopback.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otuntype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otflags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otids.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otctxts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otctxt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otqos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "oterr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tuntype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tflags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tids.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tctxts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tctxt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tqos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "terr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_ack.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_win.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_tsval.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_txecr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "padding",
	.field_bit_size = 85,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 1, , table: fkb_select.roce_wc_fkb */
	{
	.description = "l2_cntxt_id.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "l2ip_func.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "parif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "spif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "svif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "lcos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meta_hi.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meta_lo.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "rcyc_cnt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "loopback.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otuntype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otflags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otids.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otctxts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otctxt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otqos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "oterr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tuntype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tflags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tids.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tctxts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tctxt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tqos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "terr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(171 >> 8) & 0xff,
		171 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
	.field_opr3 = {
	(25 >> 8) & 0xff,
	25 & 0xff}
	},
	{
	.description = "l3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(175 >> 8) & 0xff,
		175 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(177 >> 8) & 0xff,
		177 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_ack.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(179 >> 8) & 0xff,
		179 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_win.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_tsval.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_txecr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "padding",
	.field_bit_size = 85,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 1, , table: profile_tcam.roce_wr */
	{
	.description = "wc_scope",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_PORT_TABLE,
	.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff,
		(BNXT_ULP_PORT_TABLE_TABLE_SCOPE >> 8) & 0xff,
		BNXT_ULP_PORT_TABLE_TABLE_SCOPE & 0xff}
	},
	{
	.description = "wc_key_id",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_WC_KEY_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_WC_KEY_ID_0 & 0xff}
	},
	{
	.description = "wc_profile_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_WC_PROFILE_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_WC_PROFILE_ID_0 & 0xff}
	},
	{
	.description = "wc_search_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "em_scope",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_PORT_TABLE,
	.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff,
		(BNXT_ULP_PORT_TABLE_TABLE_SCOPE >> 8) & 0xff,
		BNXT_ULP_PORT_TABLE_TABLE_SCOPE & 0xff}
	},
	{
	.description = "em_key_id",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_EM_KEY_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_EM_KEY_ID_0 & 0xff}
	},
	{
	.description = "em_profile_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_EM_PROFILE_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_EM_PROFILE_ID_0 & 0xff}
	},
	{
	.description = "em_search_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "pl_byp_lkup_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "padding",
	.field_bit_size = 21,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 1, , table: profile_tcam.roce_cnp_wr */
	{
	.description = "wc_scope",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_PORT_TABLE,
	.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff,
		(BNXT_ULP_PORT_TABLE_TABLE_SCOPE >> 8) & 0xff,
		BNXT_ULP_PORT_TABLE_TABLE_SCOPE & 0xff}
	},
	{
	.description = "wc_key_id",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_WC_KEY_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_WC_KEY_ID_0 & 0xff}
	},
	{
	.description = "wc_profile_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_WC_PROFILE_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_WC_PROFILE_ID_0 & 0xff}
	},
	{
	.description = "wc_search_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "em_scope",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_PORT_TABLE,
	.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff,
		(BNXT_ULP_PORT_TABLE_TABLE_SCOPE >> 8) & 0xff,
		BNXT_ULP_PORT_TABLE_TABLE_SCOPE & 0xff}
	},
	{
	.description = "em_key_id",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_EM_KEY_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_EM_KEY_ID_0 & 0xff}
	},
	{
	.description = "em_profile_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_EM_PROFILE_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_EM_PROFILE_ID_0 & 0xff}
	},
	{
	.description = "em_search_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "pl_byp_lkup_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "padding",
	.field_bit_size = 21,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 1, , table: proto_header_cache.roce_wr */
	{
	.description = "rid",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_RID >> 8) & 0xff,
	BNXT_ULP_RF_IDX_RID & 0xff}
	},
	{
	.description = "profile_tcam_index",
	.field_bit_size = 10,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PROFILE_TCAM_INDEX_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PROFILE_TCAM_INDEX_0 & 0xff}
	},
	{
	.description = "em_profile_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_EM_PROFILE_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_EM_PROFILE_ID_0 & 0xff}
	},
	{
	.description = "em_key_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_EM_KEY_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_EM_KEY_ID_0 & 0xff}
	},
	{
	.description = "wc_profile_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_WC_PROFILE_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_WC_PROFILE_ID_0 & 0xff}
	},
	{
	.description = "wc_key_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_WC_KEY_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_WC_KEY_ID_0 & 0xff}
	},
	{
	.description = "em_recipe_id",
	.field_bit_size = 16,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_EM_RECIPE_ID >> 8) & 0xff,
	BNXT_ULP_RF_IDX_EM_RECIPE_ID & 0xff}
	},
	{
	.description = "wc_recipe_id",
	.field_bit_size = 16,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_WC_RECIPE_ID >> 8) & 0xff,
	BNXT_ULP_RF_IDX_WC_RECIPE_ID & 0xff}
	},
	/* class_tid: 1, , table: fkb_select.em_fkb_roce */
	{
	.description = "l2_cntxt_id.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "l2ip_func.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "parif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "spif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "svif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "lcos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meta_hi.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meta_lo.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "rcyc_cnt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "loopback.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otuntype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otflags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otids.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otctxts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otctxt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otqos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "oterr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tuntype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tflags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tids.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tctxts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tctxt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tqos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "terr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(218 >> 8) & 0xff,
		218 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(220 >> 8) & 0xff,
		220 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(222 >> 8) & 0xff,
		222 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(224 >> 8) & 0xff,
		224 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
	.field_opr3 = {
	(35 >> 8) & 0xff,
	35 & 0xff}
	},
	{
	.description = "l3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(228 >> 8) & 0xff,
		228 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(230 >> 8) & 0xff,
		230 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(232 >> 8) & 0xff,
		232 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(234 >> 8) & 0xff,
		234 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
	.field_opr3 = {
	(36 >> 8) & 0xff,
	36 & 0xff}
	},
	{
	.description = "l3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(238 >> 8) & 0xff,
		238 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
	.field_opr3 = {
	(37 >> 8) & 0xff,
	37 & 0xff}
	},
	{
	.description = "l3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(242 >> 8) & 0xff,
		242 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_ack.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(244 >> 8) & 0xff,
		244 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_win.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_tsval.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_txecr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "padding",
	.field_bit_size = 85,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 1, , table: em_flow_conflict_cache.roce_wr */
	{
	.description = "rid",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_RID >> 8) & 0xff,
	BNXT_ULP_RF_IDX_RID & 0xff}
	},
	{
	.description = "flow_sig_id",
	.field_bit_size = 64,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CF,
	.field_opr1 = {
	(BNXT_ULP_CF_IDX_FLOW_SIG_ID >> 8) & 0xff,
	BNXT_ULP_CF_IDX_FLOW_SIG_ID & 0xff}
	},
	/* class_tid: 1, , table: em_normal.roce */
	{
	.description = "valid",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "rec_size",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "epoch0",
	.field_bit_size = 12,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CF,
	.field_opr1 = {
	(BNXT_ULP_CF_IDX_FUNCTION_ID >> 8) & 0xff,
	BNXT_ULP_CF_IDX_FUNCTION_ID & 0xff}
	},
	{
	.description = "epoch1",
	.field_bit_size = 6,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "opcode",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "strength",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	3}
	},
	{
	.description = "act_hint",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "act_rec_ptr",
	.field_bit_size = 26,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MAIN_ACTION_PTR >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MAIN_ACTION_PTR & 0xff}
	},
	{
	.description = "ring_table_idx",
	.field_bit_size = 9,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "act_rec_size",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "paths_m1",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_op",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "pad2",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "range_profile",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "range_index",
	.field_bit_size = 16,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 1, , table: em_normal.roce_queue */
	{
	.description = "valid",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "rec_size",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "epoch0",
	.field_bit_size = 12,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CF,
	.field_opr1 = {
	(BNXT_ULP_CF_IDX_FUNCTION_ID >> 8) & 0xff,
	BNXT_ULP_CF_IDX_FUNCTION_ID & 0xff}
	},
	{
	.description = "epoch1",
	.field_bit_size = 6,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "opcode",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	ULP_THOR2_SYM_EM_WM_OPCODE_OP_NORMAL_RFS}
	},
	{
	.description = "strength",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	3}
	},
	{
	.description = "act_hint",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "act_rec_ptr",
	.field_bit_size = 26,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MAIN_ACTION_PTR >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MAIN_ACTION_PTR & 0xff}
	},
	{
	.description = "ring_table_idx",
	.field_bit_size = 9,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ACT_PROP,
	.field_opr1 = {
	(BNXT_ULP_ACT_PROP_IDX_QUEUE_INDEX >> 8) & 0xff,
	BNXT_ULP_ACT_PROP_IDX_QUEUE_INDEX & 0xff}
	},
	{
	.description = "act_rec_size",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "paths_m1",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_op",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "pad2",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "range_profile",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "range_index",
	.field_bit_size = 16,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 1, , table: wm_normal.roce */
	{
	.description = "fc_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_op",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "paths_m1",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "act_rec_size",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "ring_table_idx",
	.field_bit_size = 9,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "act_rec_ptr",
	.field_bit_size = 26,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MAIN_ACTION_PTR >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MAIN_ACTION_PTR & 0xff}
	},
	{
	.description = "act_hint",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "strength",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	2}
	},
	{
	.description = "opcode",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "epoch1",
	.field_bit_size = 6,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "epoch0",
	.field_bit_size = 12,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CF,
	.field_opr1 = {
	(BNXT_ULP_CF_IDX_FUNCTION_ID >> 8) & 0xff,
	BNXT_ULP_CF_IDX_FUNCTION_ID & 0xff}
	},
	{
	.description = "rec_size",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "valid",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "pad1",
	.field_bit_size = 24,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 1, , table: wm_normal.roce_queue */
	{
	.description = "fc_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_op",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "paths_m1",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "act_rec_size",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "ring_table_idx",
	.field_bit_size = 9,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ACT_PROP,
	.field_opr1 = {
	(BNXT_ULP_ACT_PROP_IDX_QUEUE_INDEX >> 8) & 0xff,
	BNXT_ULP_ACT_PROP_IDX_QUEUE_INDEX & 0xff}
	},
	{
	.description = "act_rec_ptr",
	.field_bit_size = 26,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MAIN_ACTION_PTR >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MAIN_ACTION_PTR & 0xff}
	},
	{
	.description = "act_hint",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "strength",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	2}
	},
	{
	.description = "opcode",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	ULP_THOR2_SYM_EM_WM_OPCODE_OP_NORMAL_RFS}
	},
	{
	.description = "epoch1",
	.field_bit_size = 6,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "epoch0",
	.field_bit_size = 12,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CF,
	.field_opr1 = {
	(BNXT_ULP_CF_IDX_FUNCTION_ID >> 8) & 0xff,
	BNXT_ULP_CF_IDX_FUNCTION_ID & 0xff}
	},
	{
	.description = "rec_size",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "valid",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "pad1",
	.field_bit_size = 24,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 2, , table: l2_cntxt_tcam.l2_table_create */
	{
	.description = "l2ip_dest_data",
	.field_bit_size = 17,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_dest_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_rfs_data",
	.field_bit_size = 9,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_rfs_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_act_rec_ptr",
	.field_bit_size = 26,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR >> 8) & 0xff,
	BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR & 0xff}
	},
	{
	.description = "l2ip_act_scope",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_PORT_TABLE,
	.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff,
		(BNXT_ULP_PORT_TABLE_TABLE_SCOPE >> 8) & 0xff,
		BNXT_ULP_PORT_TABLE_TABLE_SCOPE & 0xff}
	},
	{
	.description = "l2ip_act_hint",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_act_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "l2ip_meta",
	.field_bit_size = 35,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_meta_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "ctxt_opcode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	ULP_THOR2_SYM_CTXT_OPCODE_NORMAL_FLOW}
	},
	{
	.description = "prof_func_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
	},
	{
	.description = "prsv_prof_func_id",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_cntxt_id",
	.field_bit_size = 11,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 & 0xff}
	},
	{
	.description = "prsv_l2ip_cntxt_id",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "parif",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "prsv_parif",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 2, , table: l2_cntxt_tcam.roce_table_create */
	{
	.description = "l2ip_dest_data",
	.field_bit_size = 17,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_dest_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_rfs_data",
	.field_bit_size = 9,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_rfs_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_act_rec_ptr",
	.field_bit_size = 26,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR_ROCE >> 8) & 0xff,
	BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR_ROCE & 0xff}
	},
	{
	.description = "l2ip_act_scope",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_PORT_TABLE,
	.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff,
		(BNXT_ULP_PORT_TABLE_TABLE_SCOPE >> 8) & 0xff,
		BNXT_ULP_PORT_TABLE_TABLE_SCOPE & 0xff}
	},
	{
	.description = "l2ip_act_hint",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_act_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "l2ip_meta",
	.field_bit_size = 35,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_meta_enb",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "ctxt_opcode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	ULP_THOR2_SYM_CTXT_OPCODE_NORMAL_FLOW}
	},
	{
	.description = "prof_func_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
	},
	{
	.description = "prsv_prof_func_id",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_cntxt_id",
	.field_bit_size = 11,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 & 0xff}
	},
	{
	.description = "prsv_l2ip_cntxt_id",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "parif",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "prsv_parif",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 2, , table: profile_tcam.catchall */
	{
	.description = "wc_scope",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "wc_key_id",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "wc_profile_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "wc_search_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "em_scope",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "em_key_id",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "em_profile_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "em_search_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "pl_byp_lkup_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "padding",
	.field_bit_size = 21,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 2, , table: l2_cntxt_tcam_cache.egr_wr */
	{
	.description = "rid",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_RID >> 8) & 0xff,
	BNXT_ULP_RF_IDX_RID & 0xff}
	},
	{
	.description = "l2_cntxt_tcam_index",
	.field_bit_size = 11,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_cntxt_id",
	.field_bit_size = 11,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_L2_CNTXT_ID_0 & 0xff}
	},
	{
	.description = "src_property_ptr",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "prof_func_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PROF_FUNC_ID_0 & 0xff}
	},
	/* class_tid: 2, , table: fkb_select.em_fkb_roce_alloc */
	{
	.description = "l2_cntxt_id.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2ip_func.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "parif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "spif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "svif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "lcos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meta_hi.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meta_lo.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "rcyc_cnt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "loopback.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otuntype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otflags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otids.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otctxts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otctxt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otqos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "oterr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tuntype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tflags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tids.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tctxts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tctxt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tqos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "terr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_ack.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_win.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_tsval.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_txecr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "padding",
	.field_bit_size = 85,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 2, , table: fkb_select.wc_fkb_roce */
	{
	.description = "l2_cntxt_id.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "l2ip_func.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "parif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "spif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "svif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "lcos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meta_hi.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meta_lo.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "rcyc_cnt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "loopback.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otuntype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otflags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otids.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otctxts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otctxt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otqos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "oterr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tuntype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tflags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tids.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tctxts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tctxt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tqos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "terr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(291 >> 8) & 0xff,
		291 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(293 >> 8) & 0xff,
		293 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(295 >> 8) & 0xff,
		295 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(297 >> 8) & 0xff,
		297 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(299 >> 8) & 0xff,
		299 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
	.field_opr3 = {
	(40 >> 8) & 0xff,
	40 & 0xff}
	},
	{
	.description = "l3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(303 >> 8) & 0xff,
		303 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(305 >> 8) & 0xff,
		305 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(307 >> 8) & 0xff,
		307 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(309 >> 8) & 0xff,
		309 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
	.field_opr3 = {
	(41 >> 8) & 0xff,
	41 & 0xff}
	},
	{
	.description = "l3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(313 >> 8) & 0xff,
		313 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
	.field_opr3 = {
	(42 >> 8) & 0xff,
	42 & 0xff}
	},
	{
	.description = "l3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(317 >> 8) & 0xff,
		317 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_ack.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(319 >> 8) & 0xff,
		319 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_win.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_tsval.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_txecr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "padding",
	.field_bit_size = 85,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 2, , table: profile_tcam.roce */
	{
	.description = "wc_scope",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_PORT_TABLE,
	.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff,
		(BNXT_ULP_PORT_TABLE_TABLE_SCOPE >> 8) & 0xff,
		BNXT_ULP_PORT_TABLE_TABLE_SCOPE & 0xff}
	},
	{
	.description = "wc_key_id",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_WC_KEY_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_WC_KEY_ID_0 & 0xff}
	},
	{
	.description = "wc_profile_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_WC_PROFILE_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_WC_PROFILE_ID_0 & 0xff}
	},
	{
	.description = "wc_search_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "em_scope",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_PORT_TABLE,
	.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff,
		(BNXT_ULP_PORT_TABLE_TABLE_SCOPE >> 8) & 0xff,
		BNXT_ULP_PORT_TABLE_TABLE_SCOPE & 0xff}
	},
	{
	.description = "em_key_id",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_EM_KEY_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_EM_KEY_ID_0 & 0xff}
	},
	{
	.description = "em_profile_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_EM_PROFILE_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_EM_PROFILE_ID_0 & 0xff}
	},
	{
	.description = "em_search_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "pl_byp_lkup_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "padding",
	.field_bit_size = 21,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 2, , table: proto_header_cache.roce_wr */
	{
	.description = "rid",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_RID >> 8) & 0xff,
	BNXT_ULP_RF_IDX_RID & 0xff}
	},
	{
	.description = "profile_tcam_index",
	.field_bit_size = 10,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PROFILE_TCAM_INDEX_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PROFILE_TCAM_INDEX_0 & 0xff}
	},
	{
	.description = "em_profile_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_EM_PROFILE_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_EM_PROFILE_ID_0 & 0xff}
	},
	{
	.description = "em_key_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_EM_KEY_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_EM_KEY_ID_0 & 0xff}
	},
	{
	.description = "wc_profile_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_WC_PROFILE_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_WC_PROFILE_ID_0 & 0xff}
	},
	{
	.description = "wc_key_id",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_WC_KEY_ID_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_WC_KEY_ID_0 & 0xff}
	},
	{
	.description = "em_recipe_id",
	.field_bit_size = 16,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_EM_RECIPE_ID >> 8) & 0xff,
	BNXT_ULP_RF_IDX_EM_RECIPE_ID & 0xff}
	},
	{
	.description = "wc_recipe_id",
	.field_bit_size = 16,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_WC_RECIPE_ID >> 8) & 0xff,
	BNXT_ULP_RF_IDX_WC_RECIPE_ID & 0xff}
	},
	/* class_tid: 2, , table: fkb_select.em_fkb_roce */
	{
	.description = "l2_cntxt_id.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "l2ip_func.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "parif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "spif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "svif.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "lcos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meta_hi.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meta_lo.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "rcyc_cnt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "loopback.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otl4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otuntype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otflags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otids.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otctxts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otctxt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "otqos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "oterr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tl4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tuntype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tflags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tids.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tctxts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tctxt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tqos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "terr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_l2type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_dmac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_smac.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_dt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_sa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_nvt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ovt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivd.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivv.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_ivt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_etype.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_l3type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(368 >> 8) & 0xff,
		368 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(370 >> 8) & 0xff,
		370 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(372 >> 8) & 0xff,
		372 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(374 >> 8) & 0xff,
		374 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
	.field_opr3 = {
	(49 >> 8) & 0xff,
	49 & 0xff}
	},
	{
	.description = "l3_dip3.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(378 >> 8) & 0xff,
		378 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip2.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(380 >> 8) & 0xff,
		380 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip1.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(382 >> 8) & 0xff,
		382 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip0.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(384 >> 8) & 0xff,
		384 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
	.field_opr3 = {
	(50 >> 8) & 0xff,
	50 & 0xff}
	},
	{
	.description = "l3_ttl.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_prot.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(388 >> 8) & 0xff,
		388 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_NEXT,
	.field_opr3 = {
	(51 >> 8) & 0xff,
	51 & 0xff}
	},
	{
	.description = "l3_fid.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_qos.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_nonext.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_esp.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_auth.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_dest.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_rthdr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_hop.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_ieh_1frag.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_df.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_l3err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_l4type.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_src.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_dst.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(392 >> 8) & 0xff,
		392 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_flags.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_seq.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_ack.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_TERNARY_LIST,
	.field_src1 = BNXT_ULP_FIELD_SRC_LIST_AND,
	.field_opr1 = {
		(394 >> 8) & 0xff,
		394 & 0xff,
		(2 >> 8) & 0xff,
		2 & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr2 = {
	1},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_win.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_pa.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_opt.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_tcpts.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_tsval.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_txecr.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_err.en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "padding",
	.field_bit_size = 85,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 2, , table: em_flow_conflict_cache.roce_wr */
	{
	.description = "rid",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_RID >> 8) & 0xff,
	BNXT_ULP_RF_IDX_RID & 0xff}
	},
	{
	.description = "flow_sig_id",
	.field_bit_size = 64,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CF,
	.field_opr1 = {
	(BNXT_ULP_CF_IDX_FLOW_SIG_ID >> 8) & 0xff,
	BNXT_ULP_CF_IDX_FLOW_SIG_ID & 0xff}
	},
	/* class_tid: 2, , table: em_normal.roce */
	{
	.description = "valid",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "rec_size",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "epoch0",
	.field_bit_size = 12,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CF,
	.field_opr1 = {
	(BNXT_ULP_CF_IDX_FUNCTION_ID >> 8) & 0xff,
	BNXT_ULP_CF_IDX_FUNCTION_ID & 0xff}
	},
	{
	.description = "epoch1",
	.field_bit_size = 6,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "opcode",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "strength",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	2}
	},
	{
	.description = "act_hint",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "act_rec_ptr",
	.field_bit_size = 26,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MAIN_ACTION_PTR >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MAIN_ACTION_PTR & 0xff}
	},
	{
	.description = "ring_table_idx",
	.field_bit_size = 9,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "act_rec_size",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "paths_m1",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_op",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "pad2",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "range_profile",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "range_index",
	.field_bit_size = 16,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 2, , table: wm_normal.roce */
	{
	.description = "fc_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "fc_op",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "paths_m1",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "act_rec_size",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "ring_table_idx",
	.field_bit_size = 9,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "act_rec_ptr",
	.field_bit_size = 26,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MAIN_ACTION_PTR >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MAIN_ACTION_PTR & 0xff}
	},
	{
	.description = "act_hint",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "strength",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1_THEN_SRC2_ELSE_SRC3,
	.field_src1 = BNXT_ULP_FIELD_SRC_CF,
	.field_opr1 = {
	(BNXT_ULP_CF_IDX_LOOKUP_STRENGTH >> 8) & 0xff,
	BNXT_ULP_CF_IDX_LOOKUP_STRENGTH & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_CF,
	.field_opr2 = {
	(BNXT_ULP_CF_IDX_LOOKUP_STRENGTH >> 8) & 0xff,
	BNXT_ULP_CF_IDX_LOOKUP_STRENGTH & 0xff},
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "opcode",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "epoch1",
	.field_bit_size = 6,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "epoch0",
	.field_bit_size = 12,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CF,
	.field_opr1 = {
	(BNXT_ULP_CF_IDX_FUNCTION_ID >> 8) & 0xff,
	BNXT_ULP_CF_IDX_FUNCTION_ID & 0xff}
	},
	{
	.description = "rec_size",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "valid",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "pad1",
	.field_bit_size = 24,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 4, , table: cmm_full_act_record.ing_default_0 */
	{
	.description = "type",
	.field_bit_size = 3,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "drop",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "vlan_del_rpt",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "vnic_or_vport",
	.field_bit_size = 11,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "dest_op",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "decap_func",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "mirror",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meter_ptr",
	.field_bit_size = 10,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat0_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat0_ing_egr",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat0_ctr_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ing_egr",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ctr_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "mod_rec_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "encap_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "src_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "rsvd0",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 4, , table: mirror_tbl.alloc_pf_ing */
	{
	.description = "reserved1",
	.field_bit_size = 21,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "arp_relative",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "action_hint",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "sample_mode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "trunc_mode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "ignore_drop",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "copy_mode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "mirr_cond",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "act_rec_ptr",
	.field_bit_size = 26,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "reserved2",
	.field_bit_size = 6,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "samp_cfg",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "padding1",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 4, , table: cmm_stat_record.ing_pf_mirror */
	{
	.description = "packet_count",
	.field_bit_size = 64,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "byte_count",
	.field_bit_size = 64,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 4, , table: cmm_full_act_record.ing_pf_mirror */
	{
	.description = "type",
	.field_bit_size = 3,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "drop",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "vlan_del_rpt",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "vnic_or_vport",
	.field_bit_size = 11,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CF,
	.field_opr1 = {
	(BNXT_ULP_CF_IDX_DRV_FUNC_VNIC >> 8) & 0xff,
	BNXT_ULP_CF_IDX_DRV_FUNC_VNIC & 0xff}
	},
	{
	.description = "dest_op",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "decap_func",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "mirror",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meter_ptr",
	.field_bit_size = 10,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat0_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MIRR_FLOW_CNTR_PTR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MIRR_FLOW_CNTR_PTR_0 & 0xff}
	},
	{
	.description = "stat0_ing_egr",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat0_ctr_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ing_egr",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ctr_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "mod_rec_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "encap_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "src_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "rsvd0",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 4, , table: mirror_tbl.wr_pf_ing */
	{
	.description = "reserved1",
	.field_bit_size = 21,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "arp_relative",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "action_hint",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "sample_mode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "trunc_mode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "ignore_drop",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "copy_mode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	ULP_THOR2_SYM_MIRR_COPY_MODE_INGRESS}
	},
	{
	.description = "mirr_cond",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "act_rec_ptr",
	.field_bit_size = 26,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MIRR_ACTION_PTR >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MIRR_ACTION_PTR & 0xff}
	},
	{
	.description = "reserved2",
	.field_bit_size = 6,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "samp_cfg",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "padding1",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 4, , table: shared_mirror_record.wr_pf_ing */
	{
	.description = "rid",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_RID >> 8) & 0xff,
	BNXT_ULP_RF_IDX_RID & 0xff}
	},
	{
	.description = "mirror_id",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MIRROR_PTR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MIRROR_PTR_0 & 0xff}
	},
	{
	.description = "mirr_act_ptr",
	.field_bit_size = 64,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_CMM_ACT_HNDL_MIRROR >> 8) & 0xff,
	BNXT_ULP_RF_IDX_CMM_ACT_HNDL_MIRROR & 0xff}
	},
	/* class_tid: 4, , table: cmm_full_act_record.ing_default_arec */
	{
	.description = "type",
	.field_bit_size = 3,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "drop",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "vlan_del_rpt",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "vnic_or_vport",
	.field_bit_size = 11,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CF,
	.field_opr1 = {
	(BNXT_ULP_CF_IDX_DRV_FUNC_VNIC >> 8) & 0xff,
	BNXT_ULP_CF_IDX_DRV_FUNC_VNIC & 0xff}
	},
	{
	.description = "dest_op",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "decap_func",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "mirror",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meter_ptr",
	.field_bit_size = 10,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat0_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat0_ing_egr",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat0_ctr_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ing_egr",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ctr_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "mod_rec_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "encap_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "src_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "rsvd0",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 4, , table: cmm_full_act_record.ing_default_roce_arec */
	{
	.description = "type",
	.field_bit_size = 3,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "drop",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "vlan_del_rpt",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "vnic_or_vport",
	.field_bit_size = 11,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_PORT_TABLE,
	.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff,
		(BNXT_ULP_PORT_TABLE_DRV_FUNC_ROCE_VNIC >> 8) & 0xff,
		BNXT_ULP_PORT_TABLE_DRV_FUNC_ROCE_VNIC & 0xff}
	},
	{
	.description = "dest_op",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "decap_func",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "mirror",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_PORT_TABLE,
	.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff,
		(BNXT_ULP_PORT_TABLE_PHY_PORT_MIRROR_ID >> 8) & 0xff,
		BNXT_ULP_PORT_TABLE_PHY_PORT_MIRROR_ID & 0xff}
	},
	{
	.description = "meter_ptr",
	.field_bit_size = 10,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat0_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat0_ing_egr",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat0_ctr_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ing_egr",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ctr_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "mod_rec_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "encap_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "src_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "rsvd0",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 4, , table: port_table.ing_wr_0 */
	{
	.description = "rid",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "drv_func.mac",
	.field_bit_size = 48,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_PORT_TABLE,
	.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff,
		(BNXT_ULP_PORT_TABLE_DRV_FUNC_MAC >> 8) & 0xff,
		BNXT_ULP_PORT_TABLE_DRV_FUNC_MAC & 0xff}
	},
	{
	.description = "drv_func.parent.mac",
	.field_bit_size = 48,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "phy_port",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "port_is_pf",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "default_arec_ptr",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR >> 8) & 0xff,
	BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR & 0xff}
	},
	{
	.description = "default_arec_ptr.roce",
	.field_bit_size = 16,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR_ROCE >> 8) & 0xff,
	BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR_ROCE & 0xff}
	},
	/* class_tid: 4, , table: cmm_full_act_record.egr_throw_away */
	{
	.description = "type",
	.field_bit_size = 3,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "drop",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "vlan_del_rpt",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "vnic_or_vport",
	.field_bit_size = 11,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "dest_op",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "decap_func",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "mirror",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meter_ptr",
	.field_bit_size = 10,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat0_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat0_ing_egr",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat0_ctr_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ing_egr",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ctr_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "mod_rec_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "encap_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "src_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "rsvd0",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 4, , table: mirror_tbl.alloc_pf_egr */
	{
	.description = "reserved1",
	.field_bit_size = 21,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "arp_relative",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "action_hint",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "sample_mode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "trunc_mode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "ignore_drop",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "copy_mode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "mirr_cond",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "act_rec_ptr",
	.field_bit_size = 26,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "reserved2",
	.field_bit_size = 6,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "samp_cfg",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "padding1",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 4, , table: mod_record.meta_pf_mirror */
	{
	.description = "metadata_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "rem_ovlan",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "rem_ivlan",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "rep_add_ivlan",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "rep_add_ovlan",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "ttl_update",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "tun_md_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "reserved_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_dmac_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l2_smac_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip_ipv6_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip_ipv6_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_sip_ipv4_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l3_dip_ipv4_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_sport_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "l4_dport_en",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "metadata_rsvd",
	.field_bit_size = 10,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "metadata_op",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "metadata_prof",
	.field_bit_size = 4,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_GLB_RF,
	.field_opr1 = {
	(BNXT_ULP_GLB_RF_IDX_GLB_METADATA_TX_ACT_0 >> 8) & 0xff,
	BNXT_ULP_GLB_RF_IDX_GLB_METADATA_TX_ACT_0 & 0xff}
	},
	{
	.description = "metadata_data",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_RF_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_RF_0 & 0xff}
	},
	/* class_tid: 4, , table: cmm_stat_record.pf_mirror_egr */
	{
	.description = "packet_count",
	.field_bit_size = 64,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "byte_count",
	.field_bit_size = 64,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 4, , table: cmm_full_act_record.pf_mirror_egr */
	{
	.description = "type",
	.field_bit_size = 3,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "drop",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ACT_BIT,
	.field_opr1 = {
	((uint64_t)BNXT_ULP_ACT_BIT_DROP >> 56) & 0xff,
	((uint64_t)BNXT_ULP_ACT_BIT_DROP >> 48) & 0xff,
	((uint64_t)BNXT_ULP_ACT_BIT_DROP >> 40) & 0xff,
	((uint64_t)BNXT_ULP_ACT_BIT_DROP >> 32) & 0xff,
	((uint64_t)BNXT_ULP_ACT_BIT_DROP >> 24) & 0xff,
	((uint64_t)BNXT_ULP_ACT_BIT_DROP >> 16) & 0xff,
	((uint64_t)BNXT_ULP_ACT_BIT_DROP >> 8) & 0xff,
	(uint64_t)BNXT_ULP_ACT_BIT_DROP & 0xff}
	},
	{
	.description = "vlan_del_rpt",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "vnic_or_vport",
	.field_bit_size = 11,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	(ULP_THOR2_SYM_LOOPBACK_PORT >> 8) & 0xff,
	ULP_THOR2_SYM_LOOPBACK_PORT & 0xff}
	},
	{
	.description = "dest_op",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "decap_func",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "mirror",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meter_ptr",
	.field_bit_size = 10,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat0_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MIRR_FLOW_CNTR_PTR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MIRR_FLOW_CNTR_PTR_0 & 0xff}
	},
	{
	.description = "stat0_ing_egr",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "stat0_ctr_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ing_egr",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ctr_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "mod_rec_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MIRR_MODIFY_PTR >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MIRR_MODIFY_PTR & 0xff}
	},
	{
	.description = "encap_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "src_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "rsvd0",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 4, , table: mirror_tbl.wr_pf_egr */
	{
	.description = "reserved1",
	.field_bit_size = 21,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "arp_relative",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "action_hint",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "sample_mode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "trunc_mode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "ignore_drop",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "copy_mode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	ULP_THOR2_SYM_MIRR_COPY_MODE_INGRESS}
	},
	{
	.description = "mirr_cond",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "act_rec_ptr",
	.field_bit_size = 26,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MAIN_ACTION_PTR >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MAIN_ACTION_PTR & 0xff}
	},
	{
	.description = "reserved2",
	.field_bit_size = 6,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "samp_cfg",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "padding1",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 4, , table: shared_mirror_record.wr_pf_egr */
	{
	.description = "rid",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_RID >> 8) & 0xff,
	BNXT_ULP_RF_IDX_RID & 0xff}
	},
	{
	.description = "mirror_id",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MIRROR_PTR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MIRROR_PTR_0 & 0xff}
	},
	{
	.description = "mirr_act_ptr",
	.field_bit_size = 64,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_CMM_ACT_HNDL_MIRROR >> 8) & 0xff,
	BNXT_ULP_RF_IDX_CMM_ACT_HNDL_MIRROR & 0xff}
	},
	/* class_tid: 4, , table: cmm_full_act_record.egr_default_arec */
	{
	.description = "type",
	.field_bit_size = 3,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "drop",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "vlan_del_rpt",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "vnic_or_vport",
	.field_bit_size = 11,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CF,
	.field_opr1 = {
	(BNXT_ULP_CF_IDX_PHY_PORT_VPORT >> 8) & 0xff,
	BNXT_ULP_CF_IDX_PHY_PORT_VPORT & 0xff}
	},
	{
	.description = "dest_op",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "decap_func",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "mirror",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "meter_ptr",
	.field_bit_size = 10,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat0_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat0_ing_egr",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat0_ctr_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ing_egr",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ctr_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "mod_rec_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "encap_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "src_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "rsvd0",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 4, , table: cmm_full_act_record.egr_default_roce_arec */
	{
	.description = "type",
	.field_bit_size = 3,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CONST,
	.field_opr1 = {
	1}
	},
	{
	.description = "drop",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "vlan_del_rpt",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "vnic_or_vport",
	.field_bit_size = 11,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_CF,
	.field_opr1 = {
	(BNXT_ULP_CF_IDX_PHY_PORT_LAG_VPORT >> 8) & 0xff,
	BNXT_ULP_CF_IDX_PHY_PORT_LAG_VPORT & 0xff}
	},
	{
	.description = "dest_op",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "decap_func",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "mirror",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_PORT_TABLE,
	.field_opr1 = {
		(BNXT_ULP_CF_IDX_DEV_PORT_ID >> 8) & 0xff,
		BNXT_ULP_CF_IDX_DEV_PORT_ID & 0xff,
		(BNXT_ULP_PORT_TABLE_PHY_PORT_MIRROR_ID >> 8) & 0xff,
		BNXT_ULP_PORT_TABLE_PHY_PORT_MIRROR_ID & 0xff}
	},
	{
	.description = "meter_ptr",
	.field_bit_size = 10,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat0_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat0_ing_egr",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat0_ctr_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ing_egr",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "stat1_ctr_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "mod_rec_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "encap_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "src_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "rsvd0",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 4, , table: port_table.egr_wr_0 */
	{
	.description = "rid",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "drv_func.mac",
	.field_bit_size = 48,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "drv_func.parent.mac",
	.field_bit_size = 48,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "phy_port",
	.field_bit_size = 8,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "port_is_pf",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "default_arec_ptr",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR >> 8) & 0xff,
	BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR & 0xff}
	},
	{
	.description = "default_arec_ptr.roce",
	.field_bit_size = 16,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR_ROCE >> 8) & 0xff,
	BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR_ROCE & 0xff}
	},
	/* class_tid: 5, , table: cmm_full_act_record.wr_ing_pf_mirror_ar */
	{
	.description = "type",
	.field_bit_size = 3,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_TYPE_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_TYPE_0 & 0xff}
	},
	{
	.description = "drop",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1_THEN_SRC2_ELSE_SRC3,
	.field_src1 = BNXT_ULP_FIELD_SRC_CF,
	.field_opr1 = {
	(BNXT_ULP_CF_IDX_MIRROR_OP >> 8) & 0xff,
	BNXT_ULP_CF_IDX_MIRROR_OP & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_ZERO,
	.field_src3 = BNXT_ULP_FIELD_SRC_ONES
	},
	{
	.description = "vlan_del_rpt",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_VLAN_DEL_RPT_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_VLAN_DEL_RPT_0 & 0xff}
	},
	{
	.description = "vnic_or_vport",
	.field_bit_size = 11,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_VNIC_OR_VPORT_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_VNIC_OR_VPORT_0 & 0xff}
	},
	{
	.description = "dest_op",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_DEST_OP_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_DEST_OP_0 & 0xff}
	},
	{
	.description = "decap_func",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_DECAP_FUNC_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_DECAP_FUNC_0 & 0xff}
	},
	{
	.description = "mirror",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_MIRROR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_MIRROR_0 & 0xff}
	},
	{
	.description = "meter_ptr",
	.field_bit_size = 10,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_METER_PTR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_METER_PTR_0 & 0xff}
	},
	{
	.description = "stat0_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_STAT0_PTR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_STAT0_PTR_0 & 0xff}
	},
	{
	.description = "stat0_ing_egr",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_STAT0_ING_EGR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_STAT0_ING_EGR_0 & 0xff}
	},
	{
	.description = "stat0_ctr_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_STAT0_CTR_TYPE_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_STAT0_CTR_TYPE_0 & 0xff}
	},
	{
	.description = "stat1_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_STAT1_PTR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_STAT1_PTR_0 & 0xff}
	},
	{
	.description = "stat1_ing_egr",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_STAT1_ING_EGR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_STAT1_ING_EGR_0 & 0xff}
	},
	{
	.description = "stat1_ctr_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_STAT1_CTR_TYPE_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_STAT1_CTR_TYPE_0 & 0xff}
	},
	{
	.description = "mod_rec_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_MOD_REC_PTR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_MOD_REC_PTR_0 & 0xff}
	},
	{
	.description = "encap_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_ENCAP_PTR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_ENCAP_PTR_0 & 0xff}
	},
	{
	.description = "src_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_SRC_PTR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_SRC_PTR_0 & 0xff}
	},
	{
	.description = "rsvd0",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 5, , table: mirror_tbl.update_wr_ing_op */
	{
	.description = "reserved1",
	.field_bit_size = 21,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "arp_relative",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MIRR_ARP_RELATIVE_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MIRR_ARP_RELATIVE_0 & 0xff}
	},
	{
	.description = "action_hint",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "sample_mode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "trunc_mode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "ignore_drop",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MIRR_IGNORE_DROP_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MIRR_IGNORE_DROP_0 & 0xff}
	},
	{
	.description = "copy_mode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MIRR_COPY_MODE_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MIRR_COPY_MODE_0 & 0xff}
	},
	{
	.description = "mirr_cond",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MIRR_COND_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MIRR_COND_0 & 0xff}
	},
	{
	.description = "act_rec_ptr",
	.field_bit_size = 26,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MIRR_ACTION_PTR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MIRR_ACTION_PTR_0 & 0xff}
	},
	{
	.description = "reserved2",
	.field_bit_size = 6,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "samp_cfg",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1_THEN_SRC2_ELSE_SRC3,
	.field_src1 = BNXT_ULP_FIELD_SRC_CF,
	.field_opr1 = {
	(BNXT_ULP_CF_IDX_MIRROR_OP >> 8) & 0xff,
	BNXT_ULP_CF_IDX_MIRROR_OP & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_ONES,
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "padding1",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 6, , table: cmm_full_act_record.wr_egr_pf_mirror_ar */
	{
	.description = "type",
	.field_bit_size = 3,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_TYPE_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_TYPE_0 & 0xff}
	},
	{
	.description = "drop",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1_THEN_SRC2_ELSE_SRC3,
	.field_src1 = BNXT_ULP_FIELD_SRC_CF,
	.field_opr1 = {
	(BNXT_ULP_CF_IDX_MIRROR_OP >> 8) & 0xff,
	BNXT_ULP_CF_IDX_MIRROR_OP & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_ZERO,
	.field_src3 = BNXT_ULP_FIELD_SRC_ONES
	},
	{
	.description = "vlan_del_rpt",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_VLAN_DEL_RPT_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_VLAN_DEL_RPT_0 & 0xff}
	},
	{
	.description = "vnic_or_vport",
	.field_bit_size = 11,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_VNIC_OR_VPORT_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_VNIC_OR_VPORT_0 & 0xff}
	},
	{
	.description = "dest_op",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_DEST_OP_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_DEST_OP_0 & 0xff}
	},
	{
	.description = "decap_func",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_DECAP_FUNC_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_DECAP_FUNC_0 & 0xff}
	},
	{
	.description = "mirror",
	.field_bit_size = 5,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_MIRROR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_MIRROR_0 & 0xff}
	},
	{
	.description = "meter_ptr",
	.field_bit_size = 10,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_METER_PTR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_METER_PTR_0 & 0xff}
	},
	{
	.description = "stat0_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_STAT0_PTR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_STAT0_PTR_0 & 0xff}
	},
	{
	.description = "stat0_ing_egr",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_STAT0_ING_EGR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_STAT0_ING_EGR_0 & 0xff}
	},
	{
	.description = "stat0_ctr_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_STAT0_CTR_TYPE_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_STAT0_CTR_TYPE_0 & 0xff}
	},
	{
	.description = "stat1_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_STAT1_PTR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_STAT1_PTR_0 & 0xff}
	},
	{
	.description = "stat1_ing_egr",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_STAT1_ING_EGR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_STAT1_ING_EGR_0 & 0xff}
	},
	{
	.description = "stat1_ctr_type",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_STAT1_CTR_TYPE_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_STAT1_CTR_TYPE_0 & 0xff}
	},
	{
	.description = "mod_rec_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_MOD_REC_PTR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_MOD_REC_PTR_0 & 0xff}
	},
	{
	.description = "encap_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_ENCAP_PTR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_ENCAP_PTR_0 & 0xff}
	},
	{
	.description = "src_ptr",
	.field_bit_size = 28,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_PF_MIRR_SRC_PTR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_PF_MIRR_SRC_PTR_0 & 0xff}
	},
	{
	.description = "rsvd0",
	.field_bit_size = 7,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	/* class_tid: 6, , table: mirror_tbl.update_wr_egr_op */
	{
	.description = "reserved1",
	.field_bit_size = 21,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "arp_relative",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MIRR_ARP_RELATIVE_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MIRR_ARP_RELATIVE_0 & 0xff}
	},
	{
	.description = "action_hint",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "sample_mode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "trunc_mode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "ignore_drop",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MIRR_IGNORE_DROP_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MIRR_IGNORE_DROP_0 & 0xff}
	},
	{
	.description = "copy_mode",
	.field_bit_size = 2,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MIRR_COPY_MODE_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MIRR_COPY_MODE_0 & 0xff}
	},
	{
	.description = "mirr_cond",
	.field_bit_size = 1,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MIRR_COND_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MIRR_COND_0 & 0xff}
	},
	{
	.description = "act_rec_ptr",
	.field_bit_size = 26,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_RF,
	.field_opr1 = {
	(BNXT_ULP_RF_IDX_MIRR_ACTION_PTR_0 >> 8) & 0xff,
	BNXT_ULP_RF_IDX_MIRR_ACTION_PTR_0 & 0xff}
	},
	{
	.description = "reserved2",
	.field_bit_size = 6,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "samp_cfg",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1_THEN_SRC2_ELSE_SRC3,
	.field_src1 = BNXT_ULP_FIELD_SRC_CF,
	.field_opr1 = {
	(BNXT_ULP_CF_IDX_MIRROR_OP >> 8) & 0xff,
	BNXT_ULP_CF_IDX_MIRROR_OP & 0xff},
	.field_src2 = BNXT_ULP_FIELD_SRC_ONES,
	.field_src3 = BNXT_ULP_FIELD_SRC_ZERO
	},
	{
	.description = "padding1",
	.field_bit_size = 32,
	.field_opc = BNXT_ULP_FIELD_OPC_SRC1,
	.field_src1 = BNXT_ULP_FIELD_SRC_ZERO
	}
};

struct bnxt_ulp_mapper_ident_info ulp_thor2_class_ident_list[] = {
	/* class_tid: 1, , table: port_table.rd */
	{
	.description = "default_arec_ptr",
	.regfile_idx = BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR,
	.ident_bit_size = 32,
	.ident_bit_pos = 137
	},
	{
	.description = "default_arec_ptr.roce",
	.regfile_idx = BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR_ROCE,
	.ident_bit_size = 16,
	.ident_bit_pos = 169
	},
	{
	.description = "drv_func.mac",
	.regfile_idx = BNXT_ULP_RF_IDX_DRV_FUNC_MAC,
	.ident_bit_size = 48,
	.ident_bit_pos = 32
	},
	/* class_tid: 1, , table: mac_addr_cache.rd */
	{
	.description = "l2_cntxt_id",
	.regfile_idx = BNXT_ULP_RF_IDX_L2_CNTXT_ID_0,
	.ident_bit_size = 11,
	.ident_bit_pos = 43
	},
	{
	.description = "l2_cntxt_tcam_index",
	.regfile_idx = BNXT_ULP_RF_IDX_PROF_FUNC_ID_1,
	.ident_bit_size = 11,
	.ident_bit_pos = 32
	},
	{
	.description = "prof_func_id",
	.regfile_idx = BNXT_ULP_RF_IDX_PROF_FUNC_ID_0,
	.ident_bit_size = 8,
	.ident_bit_pos = 86
	},
	/* class_tid: 1, , table: l2_cntxt_tcam.l2_table_create */
	{
	.description = "l2_cntxt_id",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_IDENTIFIER,
	.ident_type = CFA_RSUBTYPE_IDENT_L2CTX,
	.regfile_idx = BNXT_ULP_RF_IDX_L2_CNTXT_ID_0,
	.ident_bit_size = 11,
	.ident_bit_pos = 109
	},
	{
	.description = "prof_func_id",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_IDENTIFIER,
	.ident_type = CFA_RSUBTYPE_IDENT_PROF_FUNC,
	.regfile_idx = BNXT_ULP_RF_IDX_PROF_FUNC_ID_0,
	.ident_bit_size = 8,
	.ident_bit_pos = 100
	},
	/* class_tid: 1, , table: l2_cntxt_tcam.roce_cnp_l2_table_create */
	{
	.description = "prof_func_id",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_IDENTIFIER,
	.ident_type = CFA_RSUBTYPE_IDENT_PROF_FUNC,
	.regfile_idx = BNXT_ULP_RF_IDX_PROF_FUNC_ID_1,
	.ident_bit_size = 8,
	.ident_bit_pos = 100
	},
	/* class_tid: 1, , table: proto_header_cache.rd */
	{
	.description = "em_key_id",
	.regfile_idx = BNXT_ULP_RF_IDX_EM_KEY_ID_0,
	.ident_bit_size = 8,
	.ident_bit_pos = 50
	},
	{
	.description = "em_profile_id",
	.regfile_idx = BNXT_ULP_RF_IDX_EM_PROFILE_ID_0,
	.ident_bit_size = 8,
	.ident_bit_pos = 42
	},
	{
	.description = "em_recipe_id",
	.regfile_idx = BNXT_ULP_RF_IDX_EM_RECIPE_ID,
	.ident_bit_size = 16,
	.ident_bit_pos = 74
	},
	{
	.description = "profile_tcam_index",
	.regfile_idx = BNXT_ULP_RF_IDX_PROFILE_TCAM_INDEX_0,
	.ident_bit_size = 10,
	.ident_bit_pos = 32
	},
	{
	.description = "wc_key_id",
	.regfile_idx = BNXT_ULP_RF_IDX_WC_KEY_ID_0,
	.ident_bit_size = 8,
	.ident_bit_pos = 66
	},
	{
	.description = "wc_profile_id",
	.regfile_idx = BNXT_ULP_RF_IDX_WC_PROFILE_ID_0,
	.ident_bit_size = 8,
	.ident_bit_pos = 58
	},
	{
	.description = "wc_recipe_id",
	.regfile_idx = BNXT_ULP_RF_IDX_WC_RECIPE_ID,
	.ident_bit_size = 16,
	.ident_bit_pos = 90
	},
	/* class_tid: 1, , table: profile_tcam.wr */
	{
	.description = "em_profile_id",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_IDENTIFIER,
	.ident_type = CFA_RSUBTYPE_IDENT_EM_PROF,
	.regfile_idx = BNXT_ULP_RF_IDX_EM_PROFILE_ID_0,
	.ident_bit_size = 8,
	.ident_bit_pos = 33
	},
	{
	.description = "wc_profile_id",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_IDENTIFIER,
	.ident_type = CFA_RSUBTYPE_IDENT_WC_PROF,
	.regfile_idx = BNXT_ULP_RF_IDX_WC_PROFILE_ID_0,
	.ident_bit_size = 8,
	.ident_bit_pos = 12
	},
	/* class_tid: 1, , table: em_flow_conflict_cache.rd */
	{
	.description = "flow_sig_id",
	.regfile_idx = BNXT_ULP_RF_IDX_FLOW_SIG_ID,
	.ident_bit_size = 64,
	.ident_bit_pos = 32
	},
	/* class_tid: 1, , table: proto_header_cache.roce_rd */
	{
	.description = "em_key_id",
	.regfile_idx = BNXT_ULP_RF_IDX_EM_KEY_ID_0,
	.ident_bit_size = 8,
	.ident_bit_pos = 50
	},
	{
	.description = "em_profile_id",
	.regfile_idx = BNXT_ULP_RF_IDX_EM_PROFILE_ID_0,
	.ident_bit_size = 8,
	.ident_bit_pos = 42
	},
	{
	.description = "em_recipe_id",
	.regfile_idx = BNXT_ULP_RF_IDX_EM_RECIPE_ID,
	.ident_bit_size = 16,
	.ident_bit_pos = 74
	},
	{
	.description = "profile_tcam_index",
	.regfile_idx = BNXT_ULP_RF_IDX_PROFILE_TCAM_INDEX_0,
	.ident_bit_size = 10,
	.ident_bit_pos = 32
	},
	{
	.description = "wc_key_id",
	.regfile_idx = BNXT_ULP_RF_IDX_WC_KEY_ID_0,
	.ident_bit_size = 8,
	.ident_bit_pos = 66
	},
	{
	.description = "wc_profile_id",
	.regfile_idx = BNXT_ULP_RF_IDX_WC_PROFILE_ID_0,
	.ident_bit_size = 8,
	.ident_bit_pos = 58
	},
	{
	.description = "wc_recipe_id",
	.regfile_idx = BNXT_ULP_RF_IDX_WC_RECIPE_ID,
	.ident_bit_size = 16,
	.ident_bit_pos = 90
	},
	/* class_tid: 1, , table: profile_tcam.roce_wr */
	{
	.description = "em_profile_id",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_IDENTIFIER,
	.ident_type = CFA_RSUBTYPE_IDENT_EM_PROF,
	.regfile_idx = BNXT_ULP_RF_IDX_EM_PROFILE_ID_0,
	.ident_bit_size = 8,
	.ident_bit_pos = 33
	},
	{
	.description = "wc_profile_id",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_IDENTIFIER,
	.ident_type = CFA_RSUBTYPE_IDENT_WC_PROF,
	.regfile_idx = BNXT_ULP_RF_IDX_WC_PROFILE_ID_0,
	.ident_bit_size = 8,
	.ident_bit_pos = 12
	},
	/* class_tid: 1, , table: em_flow_conflict_cache.roce_rd */
	{
	.description = "flow_sig_id",
	.regfile_idx = BNXT_ULP_RF_IDX_FLOW_SIG_ID,
	.ident_bit_size = 64,
	.ident_bit_pos = 32
	},
	/* class_tid: 2, , table: port_table.egr_wr_0 */
	{
	.description = "default_arec_ptr",
	.regfile_idx = BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR,
	.ident_bit_size = 32,
	.ident_bit_pos = 137
	},
	{
	.description = "default_arec_ptr.roce",
	.regfile_idx = BNXT_ULP_RF_IDX_DEFAULT_AREC_PTR_ROCE,
	.ident_bit_size = 16,
	.ident_bit_pos = 169
	},
	/* class_tid: 2, , table: l2_cntxt_tcam_cache.egr_rd */
	{
	.description = "l2_cntxt_id",
	.regfile_idx = BNXT_ULP_RF_IDX_L2_CNTXT_ID_0,
	.ident_bit_size = 11,
	.ident_bit_pos = 43
	},
	{
	.description = "prof_func_id",
	.regfile_idx = BNXT_ULP_RF_IDX_PROF_FUNC_ID_0,
	.ident_bit_size = 8,
	.ident_bit_pos = 86
	},
	/* class_tid: 2, , table: l2_cntxt_tcam.l2_table_create */
	{
	.description = "l2_cntxt_id",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_IDENTIFIER,
	.ident_type = CFA_RSUBTYPE_IDENT_L2CTX,
	.regfile_idx = BNXT_ULP_RF_IDX_L2_CNTXT_ID_0,
	.ident_bit_size = 11,
	.ident_bit_pos = 109
	},
	{
	.description = "prof_func_id",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_IDENTIFIER,
	.ident_type = CFA_RSUBTYPE_IDENT_PROF_FUNC,
	.regfile_idx = BNXT_ULP_RF_IDX_PROF_FUNC_ID_0,
	.ident_bit_size = 8,
	.ident_bit_pos = 100
	},
	/* class_tid: 2, , table: proto_header_cache.roce_rd */
	{
	.description = "em_key_id",
	.regfile_idx = BNXT_ULP_RF_IDX_EM_KEY_ID_0,
	.ident_bit_size = 8,
	.ident_bit_pos = 50
	},
	{
	.description = "em_profile_id",
	.regfile_idx = BNXT_ULP_RF_IDX_EM_PROFILE_ID_0,
	.ident_bit_size = 8,
	.ident_bit_pos = 42
	},
	{
	.description = "em_recipe_id",
	.regfile_idx = BNXT_ULP_RF_IDX_EM_RECIPE_ID,
	.ident_bit_size = 16,
	.ident_bit_pos = 74
	},
	{
	.description = "profile_tcam_index",
	.regfile_idx = BNXT_ULP_RF_IDX_PROFILE_TCAM_INDEX_0,
	.ident_bit_size = 10,
	.ident_bit_pos = 32
	},
	{
	.description = "wc_key_id",
	.regfile_idx = BNXT_ULP_RF_IDX_WC_KEY_ID_0,
	.ident_bit_size = 8,
	.ident_bit_pos = 66
	},
	{
	.description = "wc_profile_id",
	.regfile_idx = BNXT_ULP_RF_IDX_WC_PROFILE_ID_0,
	.ident_bit_size = 8,
	.ident_bit_pos = 58
	},
	{
	.description = "wc_recipe_id",
	.regfile_idx = BNXT_ULP_RF_IDX_WC_RECIPE_ID,
	.ident_bit_size = 16,
	.ident_bit_pos = 90
	},
	/* class_tid: 2, , table: profile_tcam.roce */
	{
	.description = "em_profile_id",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_IDENTIFIER,
	.ident_type = CFA_RSUBTYPE_IDENT_EM_PROF,
	.regfile_idx = BNXT_ULP_RF_IDX_EM_PROFILE_ID_0,
	.ident_bit_size = 8,
	.ident_bit_pos = 33
	},
	{
	.description = "wc_profile_id",
	.resource_func = BNXT_ULP_RESOURCE_FUNC_IDENTIFIER,
	.ident_type = CFA_RSUBTYPE_IDENT_WC_PROF,
	.regfile_idx = BNXT_ULP_RF_IDX_WC_PROFILE_ID_0,
	.ident_bit_size = 8,
	.ident_bit_pos = 12
	},
	/* class_tid: 2, , table: em_flow_conflict_cache.roce_rd */
	{
	.description = "flow_sig_id",
	.regfile_idx = BNXT_ULP_RF_IDX_FLOW_SIG_ID,
	.ident_bit_size = 64,
	.ident_bit_pos = 32
	},
	/* class_tid: 4, , table: shared_mirror_record.rd_pf_ing */
	{
	.description = "mirr_act_ptr",
	.regfile_idx = BNXT_ULP_RF_IDX_CMM_ACT_HNDL_MIRROR,
	.ident_bit_size = 64,
	.ident_bit_pos = 37
	},
	{
	.description = "mirror_id",
	.regfile_idx = BNXT_ULP_RF_IDX_MIRROR_PTR_0,
	.ident_bit_size = 5,
	.ident_bit_pos = 32
	},
	/* class_tid: 4, , table: shared_mirror_record.rd_pf_egr */
	{
	.description = "mirr_act_ptr",
	.regfile_idx = BNXT_ULP_RF_IDX_CMM_ACT_HNDL_MIRROR,
	.ident_bit_size = 64,
	.ident_bit_pos = 37
	},
	{
	.description = "mirror_id",
	.regfile_idx = BNXT_ULP_RF_IDX_MIRROR_PTR_0,
	.ident_bit_size = 5,
	.ident_bit_pos = 32
	},
	/* class_tid: 5, , table: shared_mirror_record.rd_pf_ing_op */
	{
	.description = "mirr_act_ptr",
	.regfile_idx = BNXT_ULP_RF_IDX_CMM_ACT_HNDL_MIRROR,
	.ident_bit_size = 64,
	.ident_bit_pos = 37
	},
	{
	.description = "mirror_id",
	.regfile_idx = BNXT_ULP_RF_IDX_MIRROR_ID_0,
	.ident_bit_size = 5,
	.ident_bit_pos = 32
	},
	/* class_tid: 5, , table: cmm_full_act_record.rd_ing_pf_mirror_ar */
	{
	.description = "decap_func",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_DECAP_FUNC_0,
	.ident_bit_size = 5,
	.ident_bit_pos = 19
	},
	{
	.description = "dest_op",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_DEST_OP_0,
	.ident_bit_size = 2,
	.ident_bit_pos = 17
	},
	{
	.description = "drop",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_DROP_0,
	.ident_bit_size = 1,
	.ident_bit_pos = 3
	},
	{
	.description = "encap_ptr",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_ENCAP_PTR_0,
	.ident_bit_size = 28,
	.ident_bit_pos = 129
	},
	{
	.description = "meter_ptr",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_METER_PTR_0,
	.ident_bit_size = 10,
	.ident_bit_pos = 29
	},
	{
	.description = "mirror",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_MIRROR_0,
	.ident_bit_size = 5,
	.ident_bit_pos = 24
	},
	{
	.description = "mod_rec_ptr",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_MOD_REC_PTR_0,
	.ident_bit_size = 28,
	.ident_bit_pos = 101
	},
	{
	.description = "src_ptr",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_SRC_PTR_0,
	.ident_bit_size = 28,
	.ident_bit_pos = 157
	},
	{
	.description = "stat0_ctr_type",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_STAT0_CTR_TYPE_0,
	.ident_bit_size = 2,
	.ident_bit_pos = 68
	},
	{
	.description = "stat0_ing_egr",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_STAT0_ING_EGR_0,
	.ident_bit_size = 1,
	.ident_bit_pos = 67
	},
	{
	.description = "stat0_ptr",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_STAT0_PTR_0,
	.ident_bit_size = 28,
	.ident_bit_pos = 39
	},
	{
	.description = "stat1_ctr_type",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_STAT1_CTR_TYPE_0,
	.ident_bit_size = 2,
	.ident_bit_pos = 99
	},
	{
	.description = "stat1_ing_egr",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_STAT1_ING_EGR_0,
	.ident_bit_size = 1,
	.ident_bit_pos = 98
	},
	{
	.description = "stat1_ptr",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_STAT1_PTR_0,
	.ident_bit_size = 28,
	.ident_bit_pos = 70
	},
	{
	.description = "type",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_TYPE_0,
	.ident_bit_size = 3,
	.ident_bit_pos = 0
	},
	{
	.description = "vlan_del_rpt",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_VLAN_DEL_RPT_0,
	.ident_bit_size = 2,
	.ident_bit_pos = 4
	},
	{
	.description = "vnic_or_vport",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_VNIC_OR_VPORT_0,
	.ident_bit_size = 11,
	.ident_bit_pos = 6
	},
	/* class_tid: 5, , table: mirror_tbl.update_rd_ing_op */
	{
	.description = "act_rec_ptr",
	.regfile_idx = BNXT_ULP_RF_IDX_MIRR_ACTION_PTR_0,
	.ident_bit_size = 26,
	.ident_bit_pos = 32
	},
	{
	.description = "arp_relative",
	.regfile_idx = BNXT_ULP_RF_IDX_MIRR_ARP_RELATIVE_0,
	.ident_bit_size = 1,
	.ident_bit_pos = 21
	},
	{
	.description = "copy_mode",
	.regfile_idx = BNXT_ULP_RF_IDX_MIRR_COPY_MODE_0,
	.ident_bit_size = 2,
	.ident_bit_pos = 29
	},
	{
	.description = "ignore_drop",
	.regfile_idx = BNXT_ULP_RF_IDX_MIRR_IGNORE_DROP_0,
	.ident_bit_size = 1,
	.ident_bit_pos = 28
	},
	{
	.description = "mirr_cond",
	.regfile_idx = BNXT_ULP_RF_IDX_MIRR_COND_0,
	.ident_bit_size = 1,
	.ident_bit_pos = 31
	},
	{
	.description = "samp_cfg",
	.regfile_idx = BNXT_ULP_RF_IDX_MIRR_SAMP_CFG_0,
	.ident_bit_size = 32,
	.ident_bit_pos = 64
	},
	/* class_tid: 6, , table: shared_mirror_record.rd_pf_egr_op */
	{
	.description = "mirr_act_ptr",
	.regfile_idx = BNXT_ULP_RF_IDX_CMM_ACT_HNDL_MIRROR,
	.ident_bit_size = 64,
	.ident_bit_pos = 37
	},
	{
	.description = "mirror_id",
	.regfile_idx = BNXT_ULP_RF_IDX_MIRROR_ID_0,
	.ident_bit_size = 5,
	.ident_bit_pos = 32
	},
	/* class_tid: 6, , table: cmm_full_act_record.rd_egr_pf_mirror_ar */
	{
	.description = "decap_func",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_DECAP_FUNC_0,
	.ident_bit_size = 5,
	.ident_bit_pos = 19
	},
	{
	.description = "dest_op",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_DEST_OP_0,
	.ident_bit_size = 2,
	.ident_bit_pos = 17
	},
	{
	.description = "drop",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_DROP_0,
	.ident_bit_size = 1,
	.ident_bit_pos = 3
	},
	{
	.description = "encap_ptr",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_ENCAP_PTR_0,
	.ident_bit_size = 28,
	.ident_bit_pos = 129
	},
	{
	.description = "meter_ptr",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_METER_PTR_0,
	.ident_bit_size = 10,
	.ident_bit_pos = 29
	},
	{
	.description = "mirror",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_MIRROR_0,
	.ident_bit_size = 5,
	.ident_bit_pos = 24
	},
	{
	.description = "mod_rec_ptr",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_MOD_REC_PTR_0,
	.ident_bit_size = 28,
	.ident_bit_pos = 101
	},
	{
	.description = "src_ptr",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_SRC_PTR_0,
	.ident_bit_size = 28,
	.ident_bit_pos = 157
	},
	{
	.description = "stat0_ctr_type",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_STAT0_CTR_TYPE_0,
	.ident_bit_size = 2,
	.ident_bit_pos = 68
	},
	{
	.description = "stat0_ing_egr",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_STAT0_ING_EGR_0,
	.ident_bit_size = 1,
	.ident_bit_pos = 67
	},
	{
	.description = "stat0_ptr",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_STAT0_PTR_0,
	.ident_bit_size = 28,
	.ident_bit_pos = 39
	},
	{
	.description = "stat1_ctr_type",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_STAT1_CTR_TYPE_0,
	.ident_bit_size = 2,
	.ident_bit_pos = 99
	},
	{
	.description = "stat1_ing_egr",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_STAT1_ING_EGR_0,
	.ident_bit_size = 1,
	.ident_bit_pos = 98
	},
	{
	.description = "stat1_ptr",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_STAT1_PTR_0,
	.ident_bit_size = 28,
	.ident_bit_pos = 70
	},
	{
	.description = "type",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_TYPE_0,
	.ident_bit_size = 3,
	.ident_bit_pos = 0
	},
	{
	.description = "vlan_del_rpt",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_VLAN_DEL_RPT_0,
	.ident_bit_size = 2,
	.ident_bit_pos = 4
	},
	{
	.description = "vnic_or_vport",
	.regfile_idx = BNXT_ULP_RF_IDX_PF_MIRR_VNIC_OR_VPORT_0,
	.ident_bit_size = 11,
	.ident_bit_pos = 6
	},
	/* class_tid: 6, , table: mirror_tbl.update_rd_egr_op */
	{
	.description = "act_rec_ptr",
	.regfile_idx = BNXT_ULP_RF_IDX_MIRR_ACTION_PTR_0,
	.ident_bit_size = 26,
	.ident_bit_pos = 32
	},
	{
	.description = "arp_relative",
	.regfile_idx = BNXT_ULP_RF_IDX_MIRR_ARP_RELATIVE_0,
	.ident_bit_size = 1,
	.ident_bit_pos = 21
	},
	{
	.description = "copy_mode",
	.regfile_idx = BNXT_ULP_RF_IDX_MIRR_COPY_MODE_0,
	.ident_bit_size = 2,
	.ident_bit_pos = 29
	},
	{
	.description = "ignore_drop",
	.regfile_idx = BNXT_ULP_RF_IDX_MIRR_IGNORE_DROP_0,
	.ident_bit_size = 1,
	.ident_bit_pos = 28
	},
	{
	.description = "mirr_cond",
	.regfile_idx = BNXT_ULP_RF_IDX_MIRR_COND_0,
	.ident_bit_size = 1,
	.ident_bit_pos = 31
	},
	{
	.description = "samp_cfg",
	.regfile_idx = BNXT_ULP_RF_IDX_MIRR_SAMP_CFG_0,
	.ident_bit_size = 32,
	.ident_bit_pos = 64
	}
};
