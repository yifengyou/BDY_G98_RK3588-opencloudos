// SPDX-License-Identifier: GPL-2.0
/* Minimal RUE provider used only by the tkernel kselftests. */
#include <linux/blk-cgroup.h>
#include <linux/memcontrol.h>
#include <linux/module.h>
#include <linux/rue.h>
#include <net/cls_cgroup.h>

#ifdef CONFIG_CGROUP_NET_CLASSID
static int rue_test_net_read(struct cgroup_subsys_state *css,
			     struct seq_file *sf)
{
	return 0;
}

static void rue_test_net_dump(struct seq_file *sf)
{
}

static void rue_test_net_limit_dump(struct cgroup_subsys_state *css,
				    struct seq_file *sf)
{
}

static void rue_test_net_set_limit(struct cls_token_bucket *tb, u64 rate)
{
}

static int rue_test_net_write_limit(int ifindex, u64 min, u64 max, int all)
{
	return 0;
}

static int rue_test_net_write_online_max(int ifindex, u64 max)
{
	return 0;
}

static int rue_test_net_write_online_min(struct cgroup_cls_state *cs,
					 int ifindex, u64 rate)
{
	return 0;
}

static int rue_test_net_list_del(struct cgroup_cls_state *cs)
{
	return 0;
}

static int rue_test_net_write_rwnd(struct cgroup_subsys_state *css,
				   struct cftype *cft, u64 value)
{
	return 0;
}

static u64 rue_test_net_read_rwnd(struct cgroup_subsys_state *css,
				  struct cftype *cft)
{
	return 0;
}

static u32 rue_test_net_adjust_wnd(struct sock *sk, u32 wnd, u32 mss,
				   u16 wscale)
{
	return wnd;
}

static int rue_test_net_factor(const struct sock *sk)
{
	return 1;
}

static bool rue_test_net_is_low_prio(struct sock *sk)
{
	return false;
}

static struct rue_net_ops rue_test_net_ops = {
	.read_rx_stat = rue_test_net_read,
	.read_tx_stat = rue_test_net_read,
	.read_perdev_stat = rue_test_net_read,
	.dump_rx_tb = rue_test_net_dump,
	.dump_tx_tb = rue_test_net_dump,
	.dump_rx_limit_tb = rue_test_net_limit_dump,
	.dump_tx_limit_tb = rue_test_net_limit_dump,
	.cgroup_set_rx_limit = rue_test_net_set_limit,
	.cgroup_set_tx_limit = rue_test_net_set_limit,
	.write_rx_bps_minmax = rue_test_net_write_limit,
	.write_tx_bps_minmax = rue_test_net_write_limit,
	.write_rx_online_bps_max = rue_test_net_write_online_max,
	.write_tx_online_bps_max = rue_test_net_write_online_max,
	.write_rx_online_bps_min = rue_test_net_write_online_min,
	.write_tx_online_bps_min = rue_test_net_write_online_min,
	.rx_online_list_del = rue_test_net_list_del,
	.tx_online_list_del = rue_test_net_list_del,
	.write_rx_min_rwnd_segs = rue_test_net_write_rwnd,
	.read_rx_min_rwnd_segs = rue_test_net_read_rwnd,
	.cls_cgroup_adjust_wnd = rue_test_net_adjust_wnd,
	.cls_cgroup_factor = rue_test_net_factor,
	.is_low_prio = rue_test_net_is_low_prio,
};
#endif

#ifdef CONFIG_MEMCG
static bool rue_test_mem_need_reclaim(struct mem_cgroup *memcg)
{
	return false;
}

static void rue_test_mem_notify_alloc(struct mem_cgroup *memcg,
				      unsigned int nr_pages)
{
}

static bool rue_test_mem_notify_reclaim(struct mem_cgroup *memcg,
					unsigned int nr_pages)
{
	return false;
}

static struct rue_mem_ops rue_test_mem_ops = {
	.mem_cgroup_prio_need_reclaim = rue_test_mem_need_reclaim,
	.mem_cgroup_notify_alloc = rue_test_mem_notify_alloc,
	.mem_cgroup_notify_reclaim = rue_test_mem_notify_reclaim,
};
#endif

#ifdef CONFIG_BLK_CGROUP
static void rue_test_io_update_bandwidth(struct blkcg *blkcg)
{
}

static void rue_test_io_cgroup_sync(struct mem_cgroup *memcg)
{
}

static uint64_t rue_test_io_bps_limit(struct throtl_data *td,
				      struct throtl_grp *tg,
				      struct blkcg_gq *blkg, int rw,
				      uint64_t limit)
{
	return limit;
}

static unsigned int
rue_test_io_iops_limit(struct throtl_data *td, struct throtl_grp *tg,
		       struct blkcg_gq *blkg, int rw, unsigned int limit)
{
	return limit;
}

static int rue_test_io_dynamic_ratio(struct throtl_grp *tg)
{
	return 0;
}

static bool rue_test_io_scale_up(struct wbt_throtl_info *ti, bool force_max)
{
	return false;
}

static bool rue_test_io_scale_down(struct wbt_throtl_info *ti,
				   bool hard_throttle)
{
	return false;
}

static void rue_test_io_calc_limit(struct wbt_throtl_info *ti)
{
}

static struct rue_io_module_ops rue_test_io_ops = {
	.blkcg_update_bandwidth = rue_test_io_update_bandwidth,
	.cgroup_sync = rue_test_io_cgroup_sync,
	.calc_readwrite_bps_limit = rue_test_io_bps_limit,
	.calc_readwrite_iops_limit = rue_test_io_iops_limit,
	.new_dynamic_ratio = rue_test_io_dynamic_ratio,
	.throtl_info_scale_up = rue_test_io_scale_up,
	.throtl_info_scale_down = rue_test_io_scale_down,
	.throtl_info_calc_limit = rue_test_io_calc_limit,
};
#endif

static struct rue_ops rue_test_ops = {
#ifdef CONFIG_CGROUP_NET_CLASSID
	.net = &rue_test_net_ops,
#endif
#ifdef CONFIG_MEMCG
	.mem = &rue_test_mem_ops,
#endif
#ifdef CONFIG_BLK_CGROUP
	.io = &rue_test_io_ops,
#endif
};

static int __init rue_testmod_init(void)
{
	return register_rue_ops(&rue_test_ops);
}

static void __exit rue_testmod_exit(void)
{
	int ret;

	ret = try_unregister_rue_ops();
	WARN_ON(ret);
}

module_init(rue_testmod_init);
module_exit(rue_testmod_exit);

MODULE_DESCRIPTION("RUE provider for tkernel kselftests");
MODULE_LICENSE("GPL");
