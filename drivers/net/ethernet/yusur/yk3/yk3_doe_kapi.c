// SPDX-License-Identifier: GPL-2.0
#include "yk3_doe_kapi.h"
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/log2.h>
#include <linux/module.h>

#include "yk3_doe_process.h"
#include "yk3_uapi.h"
/**************************************************************************************************/
static int
doe_create_arraytbl(struct yk3_doe_priv *doe_priv, struct yk3_tbl_cfg *cfg)
{
	struct yk3_doe_cmd cmd = DOE_CMD_CONTAINER(cmd);
	bool is_mall_array = (cfg->tbl_type == DOE_TABLE_SMALL_ARRAY);
	int ret;

	cmd.opcode = YK3_DOE_SW_CREATE_ARRAY;
	cmd.tbl_id = cfg->tbl_id;

	cmd.tbl_param.tbl_type = DOE_TABLE_NORMAL_ARRAY;
	cmd.tbl_param.xie = is_mall_array ? YK3_DOE_LAIE : YK3_DOE_AIE;
	cmd.tbl_param.depth = cfg->depth;
	cmd.tbl_param.dov_len = cfg->array.value_len;

	cmd.tbl_param.ddr_channel = doe_array_channel(doe_priv, is_mall_array);
	cmd.tbl_param.endian = cfg->endian;
	cmd.tbl_param.is_small_array = is_mall_array;

	ret = doe_kernel_call(doe_priv, &cmd, YK3_DOE_SYNC_CALL);
	doe_debug(DOE_DEBUG_RET, "%s ret: %d", __func__, ret);

	return ret;
}

static int
doe_create_countertbl(struct yk3_doe_priv *doe_priv, struct yk3_tbl_cfg *cfg)
{
	struct yk3_doe_cmd cmd = DOE_CMD_CONTAINER(cmd);
	int ret;

	cmd.opcode = YK3_DOE_SW_CREATE_ARRAY;
	cmd.tbl_id = cfg->tbl_id;

	cmd.tbl_param.tbl_type = DOE_TABLE_COUNTER;
	cmd.tbl_param.xie = YK3_DOE_CIE;
	cmd.tbl_param.depth = cfg->depth;
	cmd.tbl_param.dov_len = sizeof(struct yk3_doe_counter);

	cmd.tbl_param.ddr_channel = doe_ddr_channel(doe_priv);
	cmd.tbl_param.endian = cfg->endian;

	ret = doe_kernel_call(doe_priv, &cmd, YK3_DOE_SYNC_CALL);
	doe_debug(DOE_DEBUG_RET, "%s ret: %d", __func__, ret);

	return ret;
}

static int
doe_create_metertbl(struct yk3_doe_priv *doe_priv, struct yk3_tbl_cfg *cfg)
{
	struct yk3_doe_cmd cmd = DOE_CMD_CONTAINER(cmd);
	int ret;

	cmd.opcode = YK3_DOE_SW_CREATE_ARRAY;
	cmd.tbl_id = cfg->tbl_id;

	cmd.tbl_param.tbl_type = DOE_TABLE_METER;
	cmd.tbl_param.xie = YK3_DOE_MIE;
	cmd.tbl_param.depth = cfg->depth;
	cmd.tbl_param.dov_len = sizeof(struct yk3_meter_config);

	cmd.tbl_param.ddr_channel = doe_ddr_channel(doe_priv);
	cmd.tbl_param.endian = cfg->endian;

	ret = doe_kernel_call(doe_priv, &cmd, YK3_DOE_SYNC_CALL);
	doe_debug(DOE_DEBUG_RET, "%s ret: %d", __func__, ret);

	return ret;
}

static int
doe_create_hashtbl(struct yk3_doe_priv *doe_priv, struct yk3_tbl_cfg *cfg)
{
	struct yk3_doe_cmd cmd = DOE_CMD_CONTAINER(cmd);
	struct yk3_doe_hie_cache cache = cfg->hash.cache;
	bool is_small_hash = (cfg->tbl_type == DOE_TABLE_SMALL_HASH);
	int ret, tbl_id = cfg->tbl_id;
	u32 depth = cfg->depth;
	u32 sdepth = ALIGN((cfg->hash.sdepth ? : depth), YK3_DOE_SDEPTH_ALIGN);

	// k3max hash's cache
	cmd.tbl_param.cache = cache;
	if (is_doe_k3max(doe_priv) && doe_priv->is_cache_isolation) {
		if (cache.cache_base & YK3_DOE_CACHE_BASE_MASK) {
			// cache_base must align 32
			return doe_error(-E_DOE_INVALID,
					 "hash tbl[%u] cache_base[%u] not %d align",
					 tbl_id, cache.cache_base,
					 YK3_DOE_CACHELINE_COUNT_PER_UNIT);
		} else if (cache.cache_unit_num > YK3_DOE_CACHELINE_UNIT_LIMIT) {
			// cache_unit_num must <= 32
			return doe_error(-E_DOE_INVALID,
					 "hash tbl[%u] cache_unit_num[%u > %u]",
					 tbl_id, cache.cache_unit_num,
					 YK3_DOE_CACHELINE_UNIT_LIMIT);
		} else if (cache.cache_unit_num == 0) {
			if (cache.cache_base) {
				// cache_unit_num must > 0 @ cache_base > 0
				doe_err("hash tbl[%u] cache_base[%u] with cache_unit_num[0]",
					tbl_id, cache.cache_base);
				return -E_DOE_INVALID;
			}

			// base=0, unit_num=0
			//	hardware as : base=0, unit_num=32
			cache.cache_unit_num = YK3_DOE_CACHELINE_UNIT_LIMIT;
		}

		if (!doe_priv->hmc.enable) {
			// non-ddr
			//	ignore depth and sdepth
			//	real depth is: cache_unit_num * 32
			depth = cache.cache_unit_num * YK3_DOE_CACHELINE_COUNT_PER_UNIT;
			sdepth = depth;
		}
	}

	cmd.opcode = YK3_DOE_SW_CREATE_HASH;
	cmd.tbl_id = tbl_id;

	cmd.tbl_param.tbl_type = cfg->tbl_type;
	cmd.tbl_param.is_hash = 1;
	cmd.tbl_param.is_small_hash = is_small_hash;
	cmd.tbl_param.xie = is_small_hash ? YK3_DOE_LHIE : YK3_DOE_HIE;
	cmd.tbl_param.depth = depth;
	cmd.tbl_param.key_len = is_small_hash ? YK3_DOE_LHIE_KLEN : cfg->hash.key_len;
	cmd.tbl_param.dov_len = is_small_hash ? YK3_DOE_LHIE_VLEN : cfg->hash.value_len;

	cmd.tbl_param.ddr_channel = doe_ddr_channel(doe_priv);
	cmd.tbl_param.endian = cfg->endian;
	cmd.tbl_param.chain_limit = cfg->hash.chain_limit;
	cmd.tbl_param.sdepth = sdepth;
	cmd.tbl_param.index_cache = YK3_DOE_INDEX_CACHE(sdepth);

	ret = doe_kernel_call(doe_priv, &cmd, YK3_DOE_SYNC_CALL);
	doe_debug(DOE_DEBUG_RET, "%s ret: %d", __func__, ret);

	return ret;
}

static int
doe_delete_arraytbl(struct yk3_doe_priv *doe_priv, int tbl_id)
{
	struct yk3_doe_cmd cmd = DOE_CMD_CONTAINER(cmd);
	int ret;

	cmd.opcode	= YK3_DOE_SW_DELETE_ARRAY;
	cmd.tbl_id	= tbl_id;

	ret = doe_kernel_call(doe_priv, &cmd, YK3_DOE_SYNC_CALL);
	doe_debug(DOE_DEBUG_RET, "%s ret: %d", __func__, ret);

	return ret;
}

static int
doe_delete_hashtbl(struct yk3_doe_priv *doe_priv, int tbl_id)
{
	struct yk3_doe_cmd cmd = DOE_CMD_CONTAINER(cmd);
	int ret;

	cmd.opcode	= YK3_DOE_SW_DELETE_HASH;
	cmd.tbl_id	= tbl_id;

	ret = doe_kernel_call(doe_priv, &cmd, YK3_DOE_SYNC_CALL);
	doe_debug(DOE_DEBUG_RET, "%s ret: %d", __func__, ret);

	return ret;
}

typedef int doe_fast_op_helper(struct yk3_doe_priv *doe_priv,
			       struct yk3_tbl_entry *param,
			       struct yk3_doe_cmd *cmd,
			       u8 call_mode);
static int
doe_fast_op(struct yk3_doe_priv *doe_priv,
	    struct yk3_tbl_entry *param,
	    doe_fast_op_helper *helper,
	    const char *s_helper,
	    u8 call_mode)
{
	struct yk3_doe_if *doe_if = doe_if_get_w(doe_priv);
	struct yk3_doe_cmd *cmd;

	cmd = doe_cmd_get(doe_if, DOE_CMD_F_FAST);
	if (!cmd) {
		atomic64_inc(&doe_priv->fast.n_user_nil);
		return doe_error(-E_DOE_NOCMD, "no cmd @ %s", s_helper);
	}

	return (*helper)(doe_priv, param, cmd, call_mode);
}

static int
doe_meter_store_helper(struct yk3_doe_priv *doe_priv,
		       struct yk3_tbl_entry *param,
		       struct yk3_doe_cmd *cmd,
		       u8 call_mode)
{
	int ret;

	cmd->opcode	= YK3_DOE_SW_ARRAY_STORE;
	cmd->tbl_id	= param->tbl_id;
	cmd->index	= param->meter.index;
	cmd->high_pri	= param->meter.high_pri;
	yk3_objcpy(&cmd->config, &param->meter.config);

	if (IS_YK3_DOE_WAIT_SLEEP(call_mode, param->flags))
		cmd->flags |= DOE_CMD_F_WAIT_SLEEP;

	ret = doe_kernel_call(doe_priv, cmd, call_mode);
	doe_debug(DOE_DEBUG_RET, "%s ret: %d", __func__, ret);

	return ret;
}

static inline int
doe_meter_store_sync(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	struct yk3_doe_cmd cmd = DOE_CMD_STACK_W;

	return doe_meter_store_helper(doe_priv, param, &cmd, call_mode);
}

static inline int
doe_meter_store_fast(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	return doe_fast_op(doe_priv, param, doe_meter_store_helper, "meter-store", call_mode);
}

static int
doe_meter_store(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	switch (YK3_DOE_CALL_MODE(call_mode)) {
	case YK3_DOE_SYNC_CALL:
		return doe_meter_store_sync(doe_priv, param, call_mode);
	case YK3_DOE_FAST_CALL:
		return doe_meter_store_fast(doe_priv, param, call_mode);
	case YK3_DOE_ASYNC_CALL:
		return -E_DOE_NOTSUPP;
	default:
		return -E_DOE_INVALID;
	}
}

static int
doe_meter_load(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	struct yk3_doe_cmd cmd = DOE_CMD_STACK_R;
	int ret;

	cmd.opcode	= YK3_DOE_SW_ARRAY_LOAD;
	cmd.tbl_id	= param->tbl_id;
	cmd.index	= param->meter.index;

	if (IS_YK3_DOE_WAIT_SLEEP(call_mode, param->flags))
		cmd.flags |= DOE_CMD_F_WAIT_SLEEP;

	ret = doe_kernel_call(doe_priv, &cmd, call_mode);
	doe_debug(DOE_DEBUG_RET, "%s ret: %d", __func__, ret);
	if (!ret) {
		// rewrite config
		yk3_objcpy(&param->meter.config, &cmd.config);
	}

	return ret;
}

static int
doe_array_store_helper(struct yk3_doe_priv *doe_priv,
		       struct yk3_tbl_entry *param,
		       struct yk3_doe_cmd *cmd,
		       u8 call_mode)
{
	int value_len = doe_priv->param[param->tbl_id].dov_len;
	int ret;

	cmd->opcode	= YK3_DOE_SW_ARRAY_STORE;
	cmd->tbl_id	= param->tbl_id;
	cmd->index	= param->array.index;
	cmd->high_pri	= param->array.high_pri;

	if (IS_YK3_DOE_WAIT_SLEEP(call_mode, param->flags))
		cmd->flags |= DOE_CMD_F_WAIT_SLEEP;

	if (IS_YK3_DOE_USER_CALL(call_mode)) {
		// user call use value
		memcpy(cmd->value, param->array.value,  value_len);
	} else {
		// kernel call use _value
		memcpy(cmd->value, param->array._value, value_len);
	}

	ret = doe_kernel_call(doe_priv, cmd, call_mode);
	doe_debug(DOE_DEBUG_RET, "%s ret: %d", __func__, ret);

	return ret;
}

static inline int
doe_array_store_sync(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	struct yk3_doe_cmd cmd = DOE_CMD_STACK_W;

	return doe_array_store_helper(doe_priv, param, &cmd, call_mode);
}

static inline int
doe_array_store_fast(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	return doe_fast_op(doe_priv, param, doe_array_store_helper, "array-store", call_mode);
}

static int
doe_array_store(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	switch (YK3_DOE_CALL_MODE(call_mode)) {
	case YK3_DOE_SYNC_CALL:
		return doe_array_store_sync(doe_priv, param, call_mode);
	case YK3_DOE_FAST_CALL:
		return doe_array_store_fast(doe_priv, param, call_mode);
	case YK3_DOE_ASYNC_CALL:
		return -E_DOE_NOTSUPP;
	default:
		return -E_DOE_INVALID;
	}
}

static int
doe_array_load(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	struct yk3_doe_cmd cmd = DOE_CMD_STACK_R;
	int value_len = doe_priv->param[param->tbl_id].dov_len;
	int ret;

	cmd.opcode	= YK3_DOE_SW_ARRAY_LOAD;
	cmd.tbl_id	= param->tbl_id;
	cmd.index	= param->array.index;

	if (IS_YK3_DOE_WAIT_SLEEP(call_mode, param->flags))
		cmd.flags |= DOE_CMD_F_WAIT_SLEEP;

	ret = doe_kernel_call(doe_priv, &cmd, call_mode);
	doe_debug(DOE_DEBUG_RET, "%s ret: %d", __func__, ret);
	if (!ret) {
		if (IS_YK3_DOE_USER_CALL(call_mode)) {
			// user call use value
			memcpy(param->array.value,  cmd.value, value_len);
		} else {
			// kernel call use _value
			memcpy(param->array._value, cmd.value, value_len);
		}

		param->array.value_len = value_len;
	} else {
		// failed, set value_len 0
		param->array.value_len = 0;
	}

	return ret;
}

static int
doe_hash_insert_helper(struct yk3_doe_priv *doe_priv,
		       struct yk3_tbl_entry *param,
		       struct yk3_doe_cmd *cmd,
		       u8 call_mode)
{
	int key_len	= doe_priv->param[param->tbl_id].key_len;
	int value_len	= doe_priv->param[param->tbl_id].dov_len;
	int ret;

	cmd->opcode	= YK3_DOE_SW_HASH_INSERT;
	cmd->tbl_id	= param->tbl_id;
	cmd->high_pri	= param->hash.high_pri;

	if (IS_YK3_DOE_WAIT_SLEEP(call_mode, param->flags))
		cmd->flags |= DOE_CMD_F_WAIT_SLEEP;

	if (IS_YK3_DOE_USER_CALL(call_mode)) {
		// user call use key/value
		memcpy(cmd->hash.key,   param->hash.key,    key_len);
		memcpy(cmd->hash.value, param->hash.value,  value_len);
	} else {
		// kernel call use _key/_value
		memcpy(cmd->hash.key,   param->hash._key,   key_len);
		memcpy(cmd->hash.value, param->hash._value, value_len);
	}

	ret = doe_kernel_call(doe_priv, cmd, call_mode);
	doe_debug(DOE_DEBUG_RET, "%s ret: %d", __func__, ret);

	return ret;
}

static inline int
doe_hash_insert_sync(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	struct yk3_doe_cmd cmd = DOE_CMD_STACK_W;

	return doe_hash_insert_helper(doe_priv, param, &cmd, call_mode);
}

static inline int
doe_hash_insert_fast(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	return doe_fast_op(doe_priv, param, doe_hash_insert_helper, "hash-insert", call_mode);
}

static int
doe_hash_insert(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	switch (YK3_DOE_CALL_MODE(call_mode)) {
	case YK3_DOE_SYNC_CALL:
		return doe_hash_insert_sync(doe_priv, param, call_mode);
	case YK3_DOE_FAST_CALL:
		return doe_hash_insert_fast(doe_priv, param, call_mode);
	case YK3_DOE_ASYNC_CALL:
		return -E_DOE_NOTSUPP;
	default:
		return -E_DOE_INVALID;
	}
}

static int
doe_hash_delete_helper(struct yk3_doe_priv *doe_priv,
		       struct yk3_tbl_entry *param,
		       struct yk3_doe_cmd *cmd,
		       u8 call_mode)
{
	int key_len = doe_priv->param[param->tbl_id].key_len;
	int ret;

	cmd->opcode	= YK3_DOE_SW_HASH_DELETE;
	cmd->tbl_id	= param->tbl_id;

	if (IS_YK3_DOE_WAIT_SLEEP(call_mode, param->flags))
		cmd->flags |= DOE_CMD_F_WAIT_SLEEP;

	if (IS_YK3_DOE_USER_CALL(call_mode)) {
		// user call use key/value
		memcpy(cmd->hash.key, param->hash.key,  key_len);
	} else {
		// kernel call use _key/_value
		memcpy(cmd->hash.key, param->hash._key, key_len);
	}

	ret = doe_kernel_call(doe_priv, cmd, call_mode);
	doe_debug(DOE_DEBUG_RET, "%s ret: %d", __func__, ret);

	return ret;
}

static inline int
doe_hash_delete_sync(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	struct yk3_doe_cmd cmd = DOE_CMD_STACK_W;

	return doe_hash_delete_helper(doe_priv, param, &cmd, call_mode);
}

static inline int
doe_hash_delete_fast(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	return doe_fast_op(doe_priv, param, doe_hash_delete_helper, "hash-delete", call_mode);
}

static int
doe_hash_delete(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	switch (YK3_DOE_CALL_MODE(call_mode)) {
	case YK3_DOE_SYNC_CALL:
		return doe_hash_delete_sync(doe_priv, param, call_mode);
	case YK3_DOE_FAST_CALL:
		return doe_hash_delete_fast(doe_priv, param, call_mode);
	case YK3_DOE_ASYNC_CALL:
		return -E_DOE_NOTSUPP;
	default:
		return -E_DOE_INVALID;
	}
}

static int
doe_hash_update_helper(struct yk3_doe_priv *doe_priv,
		       struct yk3_tbl_entry *param,
		       struct yk3_doe_cmd *cmd,
		       u8 call_mode)
{
	int key_len     = doe_priv->param[param->tbl_id].key_len;
	int value_len   = doe_priv->param[param->tbl_id].dov_len;
	int ret;

	cmd->opcode = YK3_DOE_SW_HASH_UPDATE;
	cmd->tbl_id = param->tbl_id;

	if (IS_YK3_DOE_WAIT_SLEEP(call_mode, param->flags))
		cmd->flags |= DOE_CMD_F_WAIT_SLEEP;

	if (IS_YK3_DOE_USER_CALL(call_mode)) {
		// user call use key/value
		memcpy(cmd->hash.key,   param->hash.key,    key_len);
		memcpy(cmd->hash.value, param->hash.value,  value_len);
	} else {
		// kernel call use _key/_value
		memcpy(cmd->hash.key,   param->hash._key,   key_len);
		memcpy(cmd->hash.value, param->hash._value, value_len);
	}

	ret = doe_kernel_call(doe_priv, cmd, call_mode);
	doe_debug(DOE_DEBUG_RET, "%s ret: %d", __func__, ret);

	return ret;
}

static inline int
doe_hash_update_sync(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	struct yk3_doe_cmd cmd = DOE_CMD_STACK_W;

	return doe_hash_update_helper(doe_priv, param, &cmd, call_mode);
}

static inline int
doe_hash_update_fast(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	return doe_fast_op(doe_priv, param, doe_hash_update_helper, "hash-update", call_mode);
}

static int
doe_hash_update(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	switch (YK3_DOE_CALL_MODE(call_mode)) {
	case YK3_DOE_SYNC_CALL:
		return doe_hash_update_sync(doe_priv, param, call_mode);
	case YK3_DOE_FAST_CALL:
		return doe_hash_update_fast(doe_priv, param, call_mode);
	case YK3_DOE_ASYNC_CALL:
		return -E_DOE_NOTSUPP;
	default:
		return -E_DOE_INVALID;
	}
}

static int
doe_hash_save_helper(struct yk3_doe_priv *doe_priv,
		     struct yk3_tbl_entry *param,
		     struct yk3_doe_cmd *cmd,
		     u8 call_mode)
{
	int key_len     = doe_priv->param[param->tbl_id].key_len;
	int value_len   = doe_priv->param[param->tbl_id].dov_len;
	int ret;

	cmd->opcode	= YK3_DOE_SW_HASH_SAVE;
	cmd->tbl_id	= param->tbl_id;

	if (IS_YK3_DOE_WAIT_SLEEP(call_mode, param->flags))
		cmd->flags |= DOE_CMD_F_WAIT_SLEEP;

	if (IS_YK3_DOE_USER_CALL(call_mode)) {
		// user call use key/value
		memcpy(cmd->hash.key,   param->hash.key,    key_len);
		memcpy(cmd->hash.value, param->hash.value,  value_len);
	} else {
		// kernel call use _key/_value
		memcpy(cmd->hash.key,   param->hash._key,   key_len);
		memcpy(cmd->hash.value, param->hash._value, value_len);
	}

	ret = doe_kernel_call(doe_priv, cmd, call_mode);
	doe_debug(DOE_DEBUG_RET, "%s ret: %d", __func__, ret);

	return ret;
}

static inline int
doe_hash_save_sync(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	struct yk3_doe_cmd cmd = DOE_CMD_STACK_W;

	return doe_hash_save_helper(doe_priv, param, &cmd, call_mode);
}

static inline int
doe_hash_save_fast(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	return doe_fast_op(doe_priv, param, doe_hash_save_helper, "hash-save", call_mode);
}

static int
doe_hash_save(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	switch (YK3_DOE_CALL_MODE(call_mode)) {
	case YK3_DOE_SYNC_CALL:
		return doe_hash_save_sync(doe_priv, param, call_mode);
	case YK3_DOE_FAST_CALL:
		return doe_hash_save_fast(doe_priv, param, call_mode);
	case YK3_DOE_ASYNC_CALL:
		return -E_DOE_NOTSUPP;
	default:
		return -E_DOE_INVALID;
	}
}

static int
doe_hash_query(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	struct yk3_doe_cmd cmd = DOE_CMD_STACK_R;
	int key_len     = doe_priv->param[param->tbl_id].key_len;
	int value_len   = doe_priv->param[param->tbl_id].dov_len;
	int ret;

	cmd.opcode	= YK3_DOE_SW_HASH_QUERY;
	cmd.tbl_id	= param->tbl_id;

	if (IS_YK3_DOE_WAIT_SLEEP(call_mode, param->flags))
		cmd.flags |= DOE_CMD_F_WAIT_SLEEP;

	if (IS_YK3_DOE_USER_CALL(call_mode)) {
		// user call use key/value
		memcpy(cmd.hash.key, param->hash.key,  key_len);
	} else {
		// kernel call use _key/_value
		memcpy(cmd.hash.key, param->hash._key, key_len);
	}

	ret = doe_kernel_call(doe_priv, &cmd, call_mode);
	doe_debug(DOE_DEBUG_RET, "%s ret: %d", __func__, ret);
	if (!ret) {
		if (IS_YK3_DOE_USER_CALL(call_mode)) {
			// user call use key/value
			memcpy(param->hash.value, cmd.hash.value,  value_len);
		} else {
			// kernel call use _key/_value
			memcpy(param->hash._value, cmd.hash.value, value_len);
		}
		param->hash.value_len = value_len;
	} else {
		// failed, set value_len 0
		param->hash.value_len = 0;
	}

	return ret;
}

static int
doe_counter_enable_helper(struct yk3_doe_priv *doe_priv,
			  struct yk3_tbl_entry *param,
			  struct yk3_doe_cmd *cmd,
			  u8 call_mode)
{
	int ret;

	cmd->opcode	= YK3_DOE_SW_COUNTER_ENABLE;
	cmd->tbl_id	= param->tbl_id;
	cmd->index	= param->counter.update.index;
	cmd->enable	= param->counter.update.enable;
	cmd->high_pri	= param->counter.update.high_pri |
				(!param->counter.update.clear_data ? 0x4 : 0x0);

	if (IS_YK3_DOE_WAIT_SLEEP(call_mode, param->flags))
		cmd->flags |= DOE_CMD_F_WAIT_SLEEP;

	ret = doe_kernel_call(doe_priv, cmd, call_mode);
	doe_debug(DOE_DEBUG_RET, "%s table-id[%d] index[%d] ret: %d",
		  __func__, cmd->tbl_id, cmd->index, ret);

	return ret;
}

static inline int
doe_counter_enable_sync(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	struct yk3_doe_cmd cmd = DOE_CMD_STACK_W;

	return doe_counter_enable_helper(doe_priv, param, &cmd, call_mode);
}

static inline int
doe_counter_enable_fast(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	return doe_fast_op(doe_priv, param, doe_counter_enable_helper,
			   "counter-enable", call_mode);
}

static int
doe_counter_enable(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	switch (YK3_DOE_CALL_MODE(call_mode)) {
	case YK3_DOE_SYNC_CALL:
		return doe_counter_enable_sync(doe_priv, param, call_mode);
	case YK3_DOE_FAST_CALL:
		return doe_counter_enable_fast(doe_priv, param, call_mode);
	case YK3_DOE_ASYNC_CALL:
		return -E_DOE_NOTSUPP;
	default:
		return -E_DOE_INVALID;
	}
}

static int
doe_counter_load(struct yk3_doe_priv *doe_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
	struct yk3_doe_cmd cmd = DOE_CMD_STACK_R;
	int ret;

	cmd.opcode	= YK3_DOE_SW_ARRAY_LOAD;
	cmd.tbl_id	= param->tbl_id;
	cmd.index	= param->counter.query.index;

	if (IS_YK3_DOE_WAIT_SLEEP(call_mode, param->flags))
		cmd.flags |= DOE_CMD_F_WAIT_SLEEP;

	ret = doe_kernel_call(doe_priv, &cmd, call_mode);
	doe_debug(DOE_DEBUG_RET, "%s ret: %d", __func__, ret);
	if (!ret) {
		// {}
		yk3_objcpy(&param->counter.query.entry, &cmd.counter);
	}

	return ret;
}

/**************************************************************************************************/
int yk3_doe_create_tbl(struct yk3_pdev_priv *pdev_priv, struct yk3_tbl_cfg *cfg)
{
	struct yk3_doe_priv *doe_priv;
	int tbl_type = cfg->tbl_type;
	int tbl_id = cfg->tbl_id;
	int ret;

	doe_debug(DOE_DEBUG_INIT, "create table type[%u] id[%u] depth[%u] ...",
		  tbl_type, tbl_id, cfg->depth);

	doe_priv = doe_priv_get(pdev_priv);
	if (!doe_priv)
		return -EOPNOTSUPP;

	ret = doe_hw_check(doe_priv);
	if (ret < 0)
		return ret;

	ret = doe_table_type_check(doe_priv, cfg->tbl_type);
	if (ret < 0)
		return ret;

	if (!is_good_doe_table_id(tbl_id))
		return doe_error(-E_DOE_INVALID, "invalid user table ID: %d", tbl_id);

#if YK3_SKIP_DOE
	if (!doe_table_try_enable(doe_priv, tbl_id))
		return doe_error(-E_DOE_EXIST, "table %d exist", tbl_id);
#else
	switch (tbl_type) {
	case DOE_TABLE_NORMAL_ARRAY:
	case DOE_TABLE_LOCK:
	case DOE_TABLE_SMALL_ARRAY:
		ret = doe_create_arraytbl(doe_priv, cfg);
		break;
	case DOE_TABLE_BIG_HASH:
	case DOE_TABLE_SMALL_HASH:
		ret = doe_create_hashtbl(doe_priv, cfg);
		break;
	case DOE_TABLE_COUNTER:
		ret = doe_create_countertbl(doe_priv, cfg);
		break;
	case DOE_TABLE_METER:
		ret = doe_create_metertbl(doe_priv, cfg);
		break;
	default:
		return doe_error(-E_DOE_INVALID, "create table: invalid table type: %u", tbl_type);
	}

	if (ret) {
		// error message
		return doe_error(ret, "create table type[%u] id[%u] depth[%u]",
				 tbl_type, tbl_id, cfg->depth);
	}

	// crit message
	doe_info("create table type[%u] id[%u] depth[%u]", tbl_type, tbl_id, cfg->depth);
#endif
	return ret;
}

int yk3_doe_delete_tbl(struct yk3_pdev_priv *pdev_priv, int tbl_id)
{
	struct yk3_doe_priv *doe_priv;
	int ret, tbl_type;

	doe_debug(DOE_DEBUG_INIT, "delete table id[%u] ...", tbl_id);

	if (!is_good_doe_table_id(tbl_id))
		return doe_error(-E_DOE_INVALID, "invalid user table ID %d", tbl_id);

#if YK3_SKIP_DOE
	if (!doe_table_try_disable(doe_priv, tbl_id))
		return doe_error(-E_DOE_NOTEXIST, "table %d NOT exist", tbl_id);
#else
	doe_priv = doe_priv_get(pdev_priv);
	if (!doe_priv)
		return -EOPNOTSUPP;

	ret = doe_hw_check(doe_priv);
	if (ret < 0)
		return ret;

	tbl_type = doe_tbl_type(doe_priv, tbl_id);
	switch (tbl_type) {
	case DOE_TABLE_NORMAL_ARRAY:
	case DOE_TABLE_LOCK:
	case DOE_TABLE_SMALL_ARRAY:
	case DOE_TABLE_COUNTER:
	case DOE_TABLE_METER:
		ret = doe_delete_arraytbl(doe_priv, tbl_id);
		break;
	case DOE_TABLE_BIG_HASH:
	case DOE_TABLE_SMALL_HASH:
		ret = doe_delete_hashtbl(doe_priv, tbl_id);
		break;
	default:
		return doe_error(-E_DOE_INVALID, "delete table: invalid table type: %u", tbl_type);
	}

	if (ret) {
		// error message
		return doe_error(ret, "delete table type[%u] id[%u]", tbl_type, tbl_id);
	}

	// crit message
	doe_info("delete table type[%u] id[%u]", tbl_type, tbl_id);
#endif

	return ret;
}

int yk3_doe_entry_add(struct yk3_pdev_priv *pdev_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
#if YK3_SKIP_DOE
	return 0;
#else
	struct yk3_doe_priv *doe_priv;
	int tbl_type, tbl_id = param->tbl_id;

	doe_priv = doe_priv_get(pdev_priv);
	if (!doe_priv)
		return -E_DOE_NOTSUPP;

	if (is_doe_hw_failure(doe_priv))
		return -E_DOE_HWCLOSED;

	if (!is_good_doe_table_id(tbl_id))
		return doe_error(-E_DOE_INVALID, "invalid user table ID %d", tbl_id);

	if (!is_doe_table_enable(doe_priv, tbl_id))
		return doe_error(-E_DOE_NOTEXIST, "table[%u] NOT exist", tbl_id);

	tbl_type = doe_tbl_type(doe_priv, tbl_id);
	switch (tbl_type) {
	case DOE_TABLE_BIG_HASH:
	case DOE_TABLE_SMALL_HASH:
		return doe_hash_insert(doe_priv, param, call_mode);
	default:
		return doe_error(-E_DOE_INVALID,
				 "table[%u] type[%u] NOT hash, insert entry: only support hash",
				 tbl_id, tbl_type);
	}
#endif
}

int yk3_doe_entry_delete(struct yk3_pdev_priv *pdev_priv,
			 struct yk3_tbl_entry *param,
			 u8 call_mode)
{
#if YK3_SKIP_DOE
	return 0;
#else
	struct yk3_doe_priv *doe_priv;
	int tbl_type, tbl_id = param->tbl_id;

	doe_priv = doe_priv_get(pdev_priv);
	if (!doe_priv)
		return -E_DOE_NOTSUPP;

	if (is_doe_hw_failure(doe_priv))
		return -E_DOE_HWCLOSED;

	if (!is_good_doe_table_id(tbl_id))
		return doe_error(-E_DOE_INVALID, "invalid user table ID %d", tbl_id);

	if (!is_doe_table_enable(doe_priv, tbl_id))
		return doe_error(-E_DOE_NOTEXIST, "table[%u] NOT exist", tbl_id);

	tbl_type = doe_tbl_type(doe_priv, tbl_id);
	switch (tbl_type) {
	case DOE_TABLE_BIG_HASH:
	case DOE_TABLE_SMALL_HASH:
		return doe_hash_delete(doe_priv, param, call_mode);
	default:
		return doe_error(-E_DOE_INVALID,
				 "table[%u] type[%u] NOT hash, insert entry: only support hash",
				 tbl_id, tbl_type);
	}
#endif
}

int yk3_doe_entry_update(struct yk3_pdev_priv *pdev_priv,
			 struct yk3_tbl_entry *param, u8 call_mode)
{
#if YK3_SKIP_DOE
	return 0;
#else
	struct yk3_doe_priv *doe_priv;
	int tbl_type, tbl_id = param->tbl_id;

	doe_priv = doe_priv_get(pdev_priv);
	if (!doe_priv)
		return -E_DOE_NOTSUPP;

	if (is_doe_hw_failure(doe_priv))
		return -E_DOE_HWCLOSED;

	if (!is_good_doe_table_id(tbl_id))
		return doe_error(-E_DOE_INVALID, "invalid user table ID %d", tbl_id);

	if (!is_doe_table_enable(doe_priv, tbl_id))
		return doe_error(-E_DOE_NOTEXIST, "table[%u] NOT exist", tbl_id);

	tbl_type = doe_tbl_type(doe_priv, tbl_id);
	switch (tbl_type) {
	case DOE_TABLE_NORMAL_ARRAY:
	case DOE_TABLE_SMALL_ARRAY:
	case DOE_TABLE_LOCK:
		return doe_array_store(doe_priv, param, call_mode);
	case DOE_TABLE_BIG_HASH:
	case DOE_TABLE_SMALL_HASH:
		if (param->hash.hash_save)
			return doe_hash_save(doe_priv, param, call_mode);
		else
			return doe_hash_update(doe_priv, param, call_mode);
	case DOE_TABLE_METER:
		return doe_meter_store(doe_priv, param, call_mode);
	case DOE_TABLE_COUNTER:
		return doe_counter_enable(doe_priv, param, call_mode);
	default:
		return doe_error(-E_DOE_INVALID, "invalid table type: %d @ update entry", tbl_type);
	}
#endif
}

//访表操作，表项query
//(hash表、array表、counter表、meter表可用)
int yk3_doe_entry_query(struct yk3_pdev_priv *pdev_priv, struct yk3_tbl_entry *param, u8 call_mode)
{
#if YK3_SKIP_DOE
	return 0;
#else
	struct yk3_doe_priv *doe_priv;
	int tbl_type, tbl_id = param->tbl_id;

	if (!IS_YK3_DOE_SYNC_CALL(call_mode))
		return -E_DOE_INVALID;

	doe_priv = doe_priv_get(pdev_priv);
	if (!doe_priv)
		return -E_DOE_NOTSUPP;

	if (is_doe_hw_failure(doe_priv))
		return -E_DOE_HWCLOSED;

	if (!is_good_doe_table_id(tbl_id))
		return doe_error(-E_DOE_INVALID, "invalid user table ID %d", tbl_id);

	if (!is_doe_table_enable(doe_priv, tbl_id))
		return doe_error(-E_DOE_NOTEXIST, "table[%u] NOT exist", tbl_id);

	tbl_type = doe_tbl_type(doe_priv, tbl_id);
	switch (tbl_type) {
	case DOE_TABLE_NORMAL_ARRAY:
	case DOE_TABLE_SMALL_ARRAY:
	case DOE_TABLE_LOCK:
		return doe_array_load(doe_priv, param, call_mode);
	case DOE_TABLE_BIG_HASH:
	case DOE_TABLE_SMALL_HASH:
		return doe_hash_query(doe_priv, param, call_mode);
	case DOE_TABLE_METER:
		return doe_meter_load(doe_priv, param, call_mode);
	case DOE_TABLE_COUNTER:
		if (doe_priv->hmc.enable)
			return doe_hmc_counter_load(doe_priv,
						    param->tbl_id,
						    param->counter.query.index,
						    &param->counter.query.entry);
		else
			return doe_counter_load(doe_priv, param, call_mode);
	default:
		return doe_error(-E_DOE_INVALID, "invalid table type: %d @ query entry", tbl_type);
	}
#endif
}

int yk3_doe_ctl_get(struct yk3_pdev_priv *pdev_priv, struct yk3_tbl_ctl *param)
{
	struct yk3_doe_priv *doe_priv;

	doe_priv = doe_priv_get(pdev_priv);
	if (!doe_priv)
		return -EOPNOTSUPP;

	switch (param->action_type) {
	case YK3_DOE_TLB_EXISTED:
		// needn't hw check
		return doe_table_existed(doe_priv, param->tbl_existed.tbl_id);
	case YK3_DOE_GET_PROTECT:
		return doe_get_np_protect(doe_priv);
	case YK3_DOE_GET_CACHE_INFO:
		// needn't hw check
		return doe_get_cache_info(doe_priv, &param->cache_info);
	case YK3_DOE_GET_COUNTER_ZIP:
		return doe_get_counter_zip(doe_priv);
	case YK3_DOE_GET_HCODE_MODE:
		return doe_get_hcode_mode(doe_priv, param);
	case YK3_DOE_GET_CACHE_ISOLATION:
		return doe_get_cache_ioslation(doe_priv, param);
	case YK3_DOE_GET_HASH_TABLE_MAX:
		return doe_priv->chip_spec->hash_table_limit;
	case YK3_DOE_GET_HASH_ENTRY_COUNT:
		return doe_get_hash_entry_count(doe_priv, param);
	default:
		return doe_error(-E_DOE_INVALID, "invalid action: %d @ get", param->action_type);
	}
}

int yk3_doe_ctl_set(struct yk3_pdev_priv *pdev_priv, struct yk3_tbl_ctl *param)
{
	struct yk3_doe_priv *doe_priv;
	int ret = 0;

	doe_priv = doe_priv_get(pdev_priv);
	if (!doe_priv)
		return -EOPNOTSUPP;

	// all need hw check
	ret = doe_hw_check(doe_priv);
	if (ret < 0)
		return ret;

	switch (param->action_type) {
	case YK3_DOE_SET_PROTECT:
		return doe_set_np_protect(doe_priv, param->protect.status);
	case YK3_DOE_SET_CACHE_MODE:
		return doe_set_cache_mode(doe_priv, &param->cache_mode);
	case YK3_DOE_SET_COUNTER_ZIP:
		return doe_set_counter_zip(doe_priv, param->counter.zip);
	case YK3_DOE_SET_HCODE_MODE:
		return doe_set_hcode_mode(doe_priv, param);
	case YK3_DOE_SET_CACHE_ISOLATION:
		return doe_set_cache_ioslation(doe_priv, param);
	default:
		return doe_error(-E_DOE_INVALID, "invalid action: %d @ set", param->action_type);
	}
}
