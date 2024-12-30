/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_ASYNC_FORK_H
#define _LINUX_ASYNC_FORK_H

#ifdef CONFIG_ASYNC_FORK

#define ASYNC_FORK_CANDIDATE	0
#define ASYNC_FORK_PENDING	1
#define ASYNC_FORK_FALLBACK	2

DECLARE_STATIC_KEY_FALSE(async_fork_enabled_key);
extern atomic_t async_fork_staging;
extern struct async_fork_ops dummy_async_fork_ops;

struct async_fork_ops {
	int  (*async_fork_prepare)(struct mm_struct *oldmm,
			struct mm_struct *mm);
	void (*async_fork_mm_bind)(struct mm_struct *oldmm,
			struct mm_struct *mm, int err);
	int  (*async_fork_fast)(struct vm_area_struct *dst_vma,
			struct vm_area_struct *src_vma);
	void (*async_fork_fast_done)(struct mm_struct *mm, int err);
	void (*async_fork_rest)(struct mm_struct *mm);
	int  (*async_fork_fixup_pmd)(struct vm_area_struct *vma, pmd_t *pmd,
			unsigned long addr);
	void (*async_fork_fixup_vma)(struct vm_area_struct *vma);
	void (*async_fork_fixup_vmas)(struct mm_struct *mm);
	void (*async_fork_madvise_vma)(struct vm_area_struct *vma,
			unsigned long start, unsigned long end);
};

extern struct async_fork_ops *async_fork_ops;

#define ASYNC_FORK_VOID(func, args...) ({		\
	typeof(async_fork_ops->func) __func =		\
		READ_ONCE(async_fork_ops->func);	\
	if (__func)					\
		__func(args);				\
})

#define ASYNC_FORK_INT(func, args...) ({		\
	typeof(async_fork_ops->func) __func =		\
		READ_ONCE(async_fork_ops->func);	\
	int ret = 0;					\
	if (__func)					\
		ret = __func(args);			\
	ret;						\
})

static inline bool async_fork_enabled(void)
{
	return static_branch_unlikely(&async_fork_enabled_key);
}

static inline bool is_pmd_async_fork(pmd_t pmd)
{
	if (is_swap_pmd(pmd) || pmd_trans_huge(pmd) || pmd_devmap(pmd))
		return false;

	return !pmd_none(pmd) && !pmd_write(pmd);
}

extern int is_async_fork_task(struct task_struct *p);

static inline bool is_async_fork_pending(struct mm_struct *mm)
{
	return mm && test_bit(ASYNC_FORK_PENDING, &mm->async_fork_flags);
}

static inline bool is_async_fork_candidate(struct mm_struct *mm)
{
	return mm && test_bit(ASYNC_FORK_CANDIDATE, &mm->async_fork_flags);
}

static inline bool is_async_fork_fallback(struct mm_struct *mm)
{
	return mm && test_bit(ASYNC_FORK_FALLBACK, &mm->async_fork_flags);
}

static inline bool is_vma_async_fork(struct vm_area_struct *vma)
{
	return !!vma->async_fork_vma;
}

static inline void async_fork_set_flags(struct mm_struct *mm, int flags)
{
	set_bit(flags, &mm->async_fork_flags);
}

static inline bool async_fork_is_orphan(struct mm_struct *oldmm,
			struct mm_struct *mm)
{
	return mm && oldmm->async_fork_mm != mm;
}

static inline void async_fork_finished(void)
{
	atomic_dec(&async_fork_staging);
}

static inline int async_fork_fast(struct vm_area_struct *dst_vma,
			struct vm_area_struct *src_vma)
{
	return ASYNC_FORK_INT(async_fork_fast, dst_vma, src_vma);
}

/*
 * @mm belongs to the child process
 */
static inline void async_fork_fast_done(struct mm_struct *mm, int err)
{
	if (unlikely(is_async_fork_pending(mm))) {
		ASYNC_FORK_VOID(async_fork_fast_done, mm, err);
		if (err)
			async_fork_finished();
	}
}

static inline void async_fork_mm_bind(struct mm_struct *oldmm,
			struct mm_struct *mm, int err)
{
	ASYNC_FORK_VOID(async_fork_mm_bind, oldmm, mm, err);
	if (err)
		async_fork_finished();
}

static inline void async_fork_rest(struct mm_struct *mm)
{
	if (unlikely(is_async_fork_pending(mm))) {
		ASYNC_FORK_VOID(async_fork_rest, mm);
		async_fork_finished();
	}
}

static inline void async_fork_fixup_pmd(struct vm_area_struct *vma, pmd_t *pmd,
			unsigned long addr)
{
	if (unlikely(is_vma_async_fork(vma) && is_pmd_async_fork(*pmd)))
		ASYNC_FORK_VOID(async_fork_fixup_pmd, vma, pmd, addr);
}

static inline void async_fork_fixup_vma(struct vm_area_struct *vma)
{
	if (unlikely(is_vma_async_fork(vma)))
		ASYNC_FORK_VOID(async_fork_fixup_vma, vma);
}

static inline void async_fork_fixup_vmas(struct mm_struct *mm)
{
	if (unlikely(is_async_fork_candidate(mm)))
		ASYNC_FORK_VOID(async_fork_fixup_vmas, mm);
}

static inline int async_fork_prepare(struct mm_struct *oldmm,
			struct mm_struct *mm)
{
	int ret = ASYNC_FORK_INT(async_fork_prepare, oldmm, mm);

	if (ret)
		async_fork_finished();
	return ret;
}

static inline void async_fork_madvise_vma(struct vm_area_struct *vma,
			unsigned long start, unsigned long end)
{
	if (unlikely(is_vma_async_fork(vma)))
		ASYNC_FORK_VOID(async_fork_madvise_vma, vma, start, end);
}

extern int copy_pte_range_atom(struct vm_area_struct *dst_vma,
			       struct vm_area_struct *src_vma,
			       pmd_t *dst_pmd, pmd_t *src_pmd,
			       unsigned long addr, unsigned long end,
			       unsigned long *prealloc_addr,
			       struct folio **prealloc);

#else /* CONFIG_ASYNC_FORK */

static inline bool async_fork_enabled(void)
{
	return false;
}

static inline bool is_pmd_async_fork(pmd_t pmd)
{
	return false;
}

static inline int is_async_fork_task(struct task_struct *p)
{
	return 0;
}

static inline int async_fork_fast(struct vm_area_struct *dst_vma,
			struct vm_area_struct *src_vma)
{
	return -EOPNOTSUPP;
}

static inline void async_fork_fast_done(struct mm_struct *mm, int err)
{
}

static inline void async_fork_mm_bind(struct mm_struct *old_mm,
			struct mm_struct *new_mm, int err)
{
}

static inline void async_fork_rest(struct mm_struct *mm)
{
}

static inline void async_fork_fixup_pmd(struct vm_area_struct *vma, pmd_t *pmd,
			unsigned long addr)
{
}

static inline void async_fork_fixup_vma(struct vm_area_struct *vma)
{
}

static inline void async_fork_fixup_vmas(struct mm_struct *mm)
{
}

static inline int async_fork_prepare(struct mm_struct *old_mm,
			struct mm_struct *new_mm)
{
	return 0;
}

static inline void async_fork_madvise_vma(struct vm_area_struct *vma,
			unsigned long start, unsigned long end)
{
}

#endif /* CONFIG_ASYNC_FORK */
#endif /* _LINUX_ASYNC_FORK_H */
