
#include <linux/sysctl.h>
#include <linux/memcontrol.h>
#include <linux/rue.h>

#ifdef CONFIG_MEMCG
int sysctl_memcg_async;
#define MG_MAGIC 0x379f54b
int memcg_async_sysctl_handler(struct ctl_table *table, int write,
		void *buffer, size_t *lenp, loff_t *ppos)
{
	int error;

	mutex_lock(&rue_mutex);
	if (write && !READ_ONCE(rue_installed)) {
		error = -EBUSY;
		pr_info("RUE: rue kernel module is not enabled or installed.");
		goto out;
	}

	error = proc_dointvec_minmax(table, write, buffer, lenp, ppos);
	if (error)
		goto out;

	if (write) {
		if (sysctl_memcg_async) {
			if (!sysctl_vm_memory_qos)
				sysctl_vm_memory_qos = MG_MAGIC;
		} else {
			if (sysctl_vm_memory_qos == MG_MAGIC)
				sysctl_vm_memory_qos = 0;
		}

		memory_qos_update();
	}

	mutex_unlock(&rue_mutex);
	return 0;

out:
	mutex_unlock(&rue_mutex);
	return error;
}
#endif

