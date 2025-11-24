/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PTP_H
#define _LINUX_PTP_H

#include <asm/haoc/haoc-def.h>
#include <asm/pgtable_types.h>

extern unsigned long long ptp_rw_gate(int flag, ...);

static inline void ptp_set_pte(pte_t *ptep, pte_t pte)
{
	compiletime_assert_rwonce_type(*ptep);
	ptp_rw_gate(IEE_OP_SET_PTE, ptep, pte);
}

static inline void ptp_set_pmd(pmd_t *pmdp, pmd_t pmd)
{
	compiletime_assert_rwonce_type(*pmdp);
	ptp_rw_gate(IEE_OP_SET_PMD, pmdp, pmd);
}

static inline void ptp_set_pud(pud_t *pudp, pud_t pud)
{
	compiletime_assert_rwonce_type(*pudp);
	ptp_rw_gate(IEE_OP_SET_PUD, pudp, pud);
}

static inline void ptp_set_p4d(p4d_t *p4dp, p4d_t p4d)
{
	compiletime_assert_rwonce_type(*p4dp);
	ptp_rw_gate(IEE_OP_SET_P4D, p4dp, p4d);
}

static inline void ptp_set_pgd(pgd_t *pgdp, pgd_t pgd)
{
	compiletime_assert_rwonce_type(*pgdp);
	ptp_rw_gate(IEE_OP_SET_PGD, pgdp, pgd);
}

static inline void ptp_set_pte_text_poke(pte_t *ptep, pte_t pte)
{
	compiletime_assert_rwonce_type(*ptep);
	ptp_rw_gate(IEE_OP_SET_PTE_TEXT_POKE, ptep, pte);
}

extern pgprotval_t ptp_xchg(pgprotval_t *pgprotp, pgprotval_t pgprotval);
extern pgprotval_t ptp_try_cmpxchg(pgprotval_t *pgprotp,
			pgprotval_t old_pgprot, pgprotval_t new_pgprotval);
extern void ptp_mark_all_pgtable_ro(void);
extern struct pg_cache pgd_cache;

#include <linux/percpu.h>
struct iee_disable_t {
	/* Writable but considered safe to expose */
	unsigned long disabled_cnt;
};

DECLARE_PER_CPU(struct iee_disable_t, iee_disables);
extern void ptp_iee_disable_init(void);
extern void ptp_disable_iee(unsigned long *reg);
extern void ptp_enable_iee(unsigned long reg);
extern void ptp_context_enable_iee(int *disabled_cnt, unsigned long *reg);
extern void ptp_context_restore_iee(int disabled_cnt, unsigned long reg);
#endif
