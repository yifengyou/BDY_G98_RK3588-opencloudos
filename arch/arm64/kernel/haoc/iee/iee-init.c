// SPDX-License-Identifier: GPL-2.0
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Lyu Jinglin <lvjl2022@zgclab.edu.cn>
 *          Zhang Shiyang <zhangsy2023@zgclab.edu.cn>
 */

#include <asm/haoc/iee.h>
#include <linux/memblock.h>
#include <asm/cpufeature.h>
#include <asm/haoc/iee-mmu.h>
#include <asm/haoc/iee-asm.h>

__aligned(PAGE_SIZE) DEFINE_PER_CPU(u64*[(PAGE_SIZE/8)],
				iee_cpu_stack_ptr);

bool __ro_after_init iee_init_done;
bool __ro_after_init haoc_enabled;

/* Allocate pages from IEE data pool to use as per-cpu IEE stack. */
static void __init iee_stack_alloc(void)
{
	int cpu;

	pr_info("IEE: Starting IEE stack allocation for ARM64\n");

	for_each_possible_cpu(cpu) {
		u64 *cpu_stack_ptr = (u64 *)(SHIFT_PERCPU_PTR(iee_cpu_stack_ptr,
				__per_cpu_offset[cpu]));
		u64 *new_pages  = __va(early_iee_stack_alloc(IEE_STACK_ORDER));

		*cpu_stack_ptr = __virt_to_iee((u64)new_pages + IEE_STACK_SIZE);
		pr_info("IEE: cpu %d, iee_stack 0x%llx\n", cpu, *cpu_stack_ptr);
	}

	flush_tlb_all();
	pr_info("IEE: IEE stack allocation completed\n");
}

/* Setup TCR for this cpu and move ASID from ttbr1 to ttbr0 */
void iee_setup_asid(void)
{
	unsigned long asid, ttbr0, ttbr1;

	pr_info("IEE: Setting up ASID for CPU %d\n", smp_processor_id());

	ttbr1 = read_sysreg(ttbr1_el1);
	asid = FIELD_GET(TTBR_ASID_MASK, ttbr1);
	ttbr0 = read_sysreg(ttbr0_el1) | FIELD_PREP(TTBR_ASID_MASK, asid);
	ttbr1 |= FIELD_PREP(TTBR_ASID_MASK, IEE_ASID);
	write_sysreg(ttbr1, ttbr1_el1);
	write_sysreg(ttbr0, ttbr0_el1);
	write_sysreg(read_sysreg(tcr_el1) & ~TCR_A1, tcr_el1);
	isb();

	/* Flush tlb to enable IEE. */
	local_flush_tlb_all();
	pr_info("IEE: ASID setup completed for CPU %d (ASID: 0x%lx)\n",
		smp_processor_id(), asid);
}

void __init iee_init_post(void)
{
	if (!haoc_enabled) {
		pr_info("IEE: HAOC is not enabled, skipping IEE initialization\n");
		return;
	}

	pr_info("IEE: Starting IEE post-initialization for ARM64\n");

	iee_setup_asid();

	iee_init_done = true;
	pr_info("IEE: IEE initialization completed successfully\n");
}

void __init iee_stack_init(void)
{
	if (!haoc_enabled) {
		pr_info("IEE: HAOC is not enabled, skipping IEE stack initialization\n");
		return;
	}

	pr_info("IEE: HAOC is enabled, initializing IEE stack\n");
	iee_stack_alloc();
}

static int __init parse_haoc_enabled(char *str)
{
	int ret = kstrtobool(str, &haoc_enabled);
	if (ret == 0)
		pr_info("IEE: HAOC parameter set to %s\n",
			haoc_enabled ? "enabled" : "disabled");
	return ret;
}
early_param("haoc", parse_haoc_enabled);
