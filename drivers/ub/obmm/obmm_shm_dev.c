// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2025. All rights reserved.
 * Description：OBMM Framework's implementations.
 */

#include <linux/cacheflush.h>
#include <asm/tlbflush.h>
#include <linux/kernel.h>
#include <linux/mm.h>

#include "obmm_cache.h"
#include "obmm_export_region_ops.h"
#include "obmm_import.h"
#include "obmm_shm_dev.h"

static dev_t obmm_devt;

static const char *obmm_shm_region_name = "OBMM_SHMDEV";
static const char *obmm_shm_rootdev_name = "obmm";
static struct device *obmm_shm_rootdev;
/**
 * Convert VM flags to mem state
 */
static unsigned long get_vma_mem_state(const vm_flags_t vm_flags, bool cacheable)
{
	unsigned long mem_state;

	if (vm_flags & VM_WRITE)
		mem_state = OBMM_SHM_MEM_READWRITE;
	else if ((vm_flags & VM_READ) && (vm_flags & VM_EXEC))
		mem_state = OBMM_SHM_MEM_READEXEC;
	else if (vm_flags & VM_READ)
		mem_state = OBMM_SHM_MEM_READONLY;
	else
		mem_state = OBMM_SHM_MEM_NO_ACCESS;

	if (cacheable && mem_state != OBMM_SHM_MEM_NO_ACCESS)
		mem_state |= OBMM_SHM_MEM_NORMAL;
	else
		mem_state |= OBMM_SHM_MEM_NORMAL_NC;
	pr_debug("VMA init mem_state: vma_flags=0x%lx, cacheable=%d, mem_state=0x%lx\n",
		 vm_flags, cacheable, mem_state);
	return mem_state;
}

/* VMA operations for obmm-mmaped VMA */
static void obmm_vma_open(struct vm_area_struct *vma)
{
	pr_debug("VMA opened range (0x%lx-0x%lx)\n", vma->vm_start, vma->vm_end);
}

static void obmm_vma_close(struct vm_area_struct *vma)
{
	struct obmm_region *reg;

	reg = (struct obmm_region *)vma->vm_file->private_data;

	mutex_lock(&reg->state_mutex);
	reg->mmap_count--;
	if (reg->mmap_count == 0) {
		/* reset mmap_mode */
		reg->mmap_mode = OBMM_MMAP_INIT;
	}
	mutex_unlock(&reg->state_mutex);
	pr_debug("obmm_shmdev munmap: mem_id=%d pid=%d vma=[%#lx, %#lx]\n", reg->regionid,
		 current->pid, vma->vm_start, vma->vm_end);
}

static int obmm_vma_may_split(struct vm_area_struct *vma __always_unused,
			      unsigned long addr __always_unused)
{
	/* not supported */
	pr_err("VMA may split at 0x%lx (range: 0x%lx-0x%lx), but split not supported\n", addr,
	       vma->vm_start, vma->vm_end);
	return -EOPNOTSUPP;
}

static int obmm_vma_mremap(struct vm_area_struct *vma __always_unused)
{
	pr_warn("mremap not supported\n");
	return -EOPNOTSUPP;
}

static int obmm_vma_mprotect(struct vm_area_struct *vma __always_unused,
			     unsigned long start __always_unused, unsigned long end __always_unused,
			     unsigned long newflags __always_unused)
{
	pr_warn("mprotect not supported\n");
	return -EOPNOTSUPP;
}
static vm_fault_t obmm_vma_fault(struct vm_fault *vmf __always_unused)
{
	pr_warn("Unexpected fault\n");
	return VM_FAULT_SIGBUS;
}
static int obmm_vma_access(struct vm_area_struct *vma __always_unused,
			   unsigned long addr __always_unused, void *buf __always_unused,
			   int len __always_unused, int write __always_unused)
{
	pr_warn("access not supported\n");
	return -EOPNOTSUPP;
}
static const char *obmm_vma_name(struct vm_area_struct *vma __always_unused)
{
	return "OBMM_SHM";
}

static const struct vm_operations_struct obmm_vm_ops = {
	.open = obmm_vma_open,
	.close = obmm_vma_close,
	.may_split = obmm_vma_may_split,
	.mremap = obmm_vma_mremap,
	.mprotect = obmm_vma_mprotect,
	.fault = obmm_vma_fault,
	.access = obmm_vma_access,
	.name = obmm_vma_name,
};

static int obmm_shm_fops_open(struct inode *inode, struct file *file)
{
	struct obmm_region *reg;
	bool cacheable;

	reg = container_of(inode->i_cdev, struct obmm_region, cdevice);
	file->private_data = reg;

	pr_debug("obmm_shmdev open: mem_id=%d pid=%d f_mode=%#x f_flags=%#x\n", reg->regionid,
		 current->pid, file->f_mode, file->f_flags);

	cacheable = !(file->f_flags & O_SYNC);
	if (cacheable && !(reg->mem_cap & OBMM_MEM_ALLOW_CACHEABLE_MMAP)) {
		pr_err("Noncacheable region %d cannot be mmaped with cachable mode.\n",
		       reg->regionid);
		return -EPERM;
	}
	if (!cacheable && !(reg->mem_cap & OBMM_MEM_ALLOW_NONCACHEABLE_MMAP)) {
		pr_err("Cacheable region %d cannot be mmaped with noncachable mode.\n",
		       reg->regionid);
		return -EPERM;
	}
	if (try_get_obmm_region(reg) == NULL) {
		pr_err("obmm_shmdev open: The device is in creation or destruction process. Open failed.\n");
		return -EAGAIN;
	}

	pr_debug("obmm_shmdev open: mem_id=%d pid=%d completed.\n", reg->regionid, current->pid);

	return 0;
}

static int obmm_shm_fops_flush(struct file *file __always_unused, fl_owner_t owner __always_unused)
{
	return 0;
}

static int obmm_shm_fops_release(struct inode *inode __always_unused, struct file *file)
{
	struct obmm_region *reg = (struct obmm_region *)file->private_data;

	pr_debug("obmm_shmdev release: mem_id=%d pid=%d\n", reg->regionid, current->pid);
	put_obmm_region(reg);

	return 0;
}

static int map_obmm_region(struct vm_area_struct *vma, struct obmm_region *reg,
			   enum obmm_mmap_granu mmap_granu)
{
	struct obmm_export_region *e_reg;
	struct obmm_import_region *i_reg;

	pr_debug("mmap region %d: size=%#llx\n", reg->regionid, reg->mem_size);
	if (reg->type == OBMM_IMPORT_REGION) {
		i_reg = container_of(reg, struct obmm_import_region, region);
		return map_import_region(vma, i_reg, mmap_granu);
	}

	e_reg = container_of(reg, struct obmm_export_region, region);
	return map_export_region(vma, e_reg, mmap_granu);
}

/* Return page table protection bits.
 * @mem_state must be validated by caller.
 */
static pgprot_t mem_state_to_pgprot(unsigned long mem_state)
{
	pgprot_t pgprot;

	/* initialize pgprot to be normal memory pgprot with certain access rights */
	if ((mem_state & OBMM_SHM_MEM_ACCESS_MASK) == OBMM_SHM_MEM_READONLY)
		pgprot = PAGE_READONLY;
	else if ((mem_state & OBMM_SHM_MEM_ACCESS_MASK) == OBMM_SHM_MEM_READEXEC)
		pgprot = PAGE_READONLY_EXEC;
	else if ((mem_state & OBMM_SHM_MEM_ACCESS_MASK) == OBMM_SHM_MEM_READWRITE)
		pgprot.pgprot = _PAGE_READONLY & ~PTE_RDONLY;
	else
		pgprot = PAGE_NONE;

	/* modify cacheability attribute if necessary */
	if ((mem_state & OBMM_SHM_MEM_CACHE_MASK) == OBMM_SHM_MEM_NORMAL_NC)
		pgprot = pgprot_writecombine(pgprot);
	else if ((mem_state & OBMM_SHM_MEM_CACHE_MASK) == OBMM_SHM_MEM_DEVICE)
		pgprot = pgprot_noncached(pgprot);

	return pgprot;
}

static void print_mmap_param(const struct file *file, const struct vm_area_struct *vma)
{
	const struct obmm_region *reg = (struct obmm_region *)file->private_data;
	const char *vm_flags_desc, *f_flags_desc;

	pr_debug("obmm_shmdev mmap: mem_id=%d pid=%d vma=[%#lx, %#lx] pgoff=%#lx ", reg->regionid,
		 current->pid, vma->vm_start, vma->vm_end, vma->vm_pgoff);

	if (vma->vm_flags & VM_WRITE)
		vm_flags_desc = "W";
	else if ((vma->vm_flags & VM_READ) && (vma->vm_flags & VM_EXEC))
		vm_flags_desc = "RX";
	else if (vma->vm_flags & VM_READ)
		vm_flags_desc = "R";
	else
		vm_flags_desc = "N";

	if (file->f_flags & O_SYNC)
		f_flags_desc = "O_SYNC";
	else
		f_flags_desc = "not O_SYNC";

	pr_debug("vm_flags=%#lx(%s) f_flags=%#x(%s)\n", vma->vm_flags, vm_flags_desc, file->f_flags,
		 f_flags_desc);
}

static bool validate_perm(struct file *file, vm_flags_t vm_flags)
{
	if (((vm_flags & VM_READ) && !(file->f_mode & FMODE_READ)) ||
	    ((vm_flags & VM_WRITE) && !(file->f_mode & FMODE_WRITE)) ||
	    ((vm_flags & VM_EXEC) && !(file->f_mode & FMODE_READ))) {
		pr_err("%s false: vm_flags: %#lx, f_mode: %#x\n", __func__, vm_flags, file->f_mode);
		return false;
	}
	return true;
}

static int obmm_shm_fops_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct obmm_region *reg = (struct obmm_region *)file->private_data;
	unsigned long size, offset;
	uint8_t mem_state;
	enum obmm_mmap_mode old_mmap_mode;
	enum obmm_mmap_granu mmap_granu;
	int ret;
	bool cacheable, o_sync;

	print_mmap_param(file, vma);
	if (!region_allow_mmap(reg)) {
		pr_err("mmap region %d: not allow to be mmaped", reg->regionid);
		return -EPERM;
	}

	if (!validate_perm(file, vma->vm_flags)) {
		pr_err("mmap region %d: invalid vma permission", reg->regionid);
		return -EPERM;
	}

	o_sync = file->f_flags & O_SYNC;
	size = vma->vm_end - vma->vm_start;
	offset = vma->vm_pgoff << PAGE_SHIFT;

	if (offset & OBMM_MMAP_FLAG_HUGETLB_PMD) {
		pr_debug("trying hugepage mmap");
		mmap_granu = OBMM_MMAP_GRANU_PMD;
		offset &= ~OBMM_MMAP_FLAG_HUGETLB_PMD;
	} else {
		mmap_granu = OBMM_MMAP_GRANU_PAGE;
	}
	if (reg->mmap_granu == OBMM_MMAP_GRANU_NONE) {
		reg->mmap_granu = mmap_granu;
	} else if (reg->mmap_granu != mmap_granu) {
		pr_err("map with PAGE_SIZE and PMD_SIZE granu should not be mixed on the same region\n");
		return -EINVAL;
	}

	vma->vm_pgoff = offset >> PAGE_SHIFT;

	if (offset >= reg->mem_size || size > reg->mem_size - offset) {
		pr_err("mmap region %d: offset:%#lx, size:%#lx over region size: %#llx",
		       reg->regionid, offset, size, reg->mem_size);
		return -EINVAL;
	}

	/*
	 * VM flags considerations
	 * Compared to legacy device memory, OBMM memory has many different properties:
	 *   1. does not have side-effects on access (VM_IO not set)
	 *   2. may be used for core dump output (VM_DONTDUMP not set)
	 * On the other hand, OBMM and traditional device memory do have some similarities:
	 *   3. the mapping cannot be inherited on process fork (VM_DONTCOPY set) for now
	 *   4. VMA merging and expanding makes no sense (VM_DONTEXPAND set)
	 *   5. the VMA should not be swapped out (VM_LOCKED set)
	 *   6. mappable import region does not has struct page; mappable export region haves struct
	 *      page, but cannot work as expected since its kernel linear mapping might be modified
	 *      (VM_PFNMAP set)
	 */
	vm_flags_set(vma, VM_DONTCOPY | VM_DONTEXPAND | VM_LOCKED | VM_PFNMAP);
	cacheable = o_sync ? false : true;
	mem_state = get_vma_mem_state(vma->vm_flags, cacheable);

	/* initial VMA page prot used by the mapping process -- will be changed later */
	vma->vm_page_prot = mem_state_to_pgprot(mem_state);

	mutex_lock(&reg->state_mutex);
	old_mmap_mode = reg->mmap_mode;

	if ((o_sync && reg->mmap_mode == OBMM_MMAP_NORMAL) ||
	    (!o_sync && reg->mmap_mode == OBMM_MMAP_OSYNC)) {
		pr_err("region cannot be mapped to cc and nc at the same time");
		ret = -EPERM;
		goto err_mutex_unlock;
	}
	if (reg->mmap_mode == OBMM_MMAP_INIT)
		reg->mmap_mode = o_sync ? OBMM_MMAP_OSYNC : OBMM_MMAP_NORMAL;

	ret = map_obmm_region(vma, reg, mmap_granu);
	if (ret) {
		pr_err("Failed to mmap region %d. ret=%pe\n", reg->regionid, ERR_PTR(ret));
		goto reset_cur_osync;
	}

	reg->mmap_count++;
	mutex_unlock(&reg->state_mutex);
	/*
	 * since OBMM allows changing protection by pages and we will not split
	 * VMA in near future. Therefore a mismatch between PTE protection and
	 * VMA flags is inevitable. Our current approach is to avoid all
	 * possible faults to change the PTE protection on the fly. Here we
	 * just set the page protection to the most restrictive one to guard
	 * against unexpected access.
	 */
	vma->vm_page_prot = vm_get_page_prot(VM_NONE);
	if (!cacheable)
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	vm_flags_clear(vma, VM_READ | VM_WRITE | VM_MAYREAD | VM_MAYWRITE);

	vma->vm_ops = &obmm_vm_ops;

	pr_debug("obmm_shmdev mmap: mem_id=%d pid=%d vma=[%#lx, %#lx] mapped: mem_state=%#x.\n",
		 reg->regionid, current->pid, vma->vm_start, vma->vm_end, mem_state);

	return 0;

reset_cur_osync:
	if (old_mmap_mode == OBMM_MMAP_INIT)
		reg->mmap_mode = OBMM_MMAP_INIT;
err_mutex_unlock:
	mutex_unlock(&reg->state_mutex);
	return ret;
}

static long obmm_shm_fops_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return -ENOTTY;
}

const struct file_operations obmm_shm_fops = { .owner = THIS_MODULE,
					       .unlocked_ioctl = obmm_shm_fops_ioctl,
					       .mmap = obmm_shm_fops_mmap,
					       .open = obmm_shm_fops_open,
					       .flush = obmm_shm_fops_flush,
					       .release = obmm_shm_fops_release };

static void obmm_shm_dev_release(struct device *dev)
{
	struct obmm_region *reg;

	reg = container_of(dev, struct obmm_region, device);
	module_put(THIS_MODULE);
}

int obmm_shm_dev_add(struct obmm_region *reg)
{
	int ret;
	dev_t devt;

	if (!try_module_get(THIS_MODULE)) {
		pr_err("Module is dying. Reject all memory requests\n");
		return -EPERM;
	}

	reg->mmap_count = 0;
	reg->mmap_mode = OBMM_MMAP_INIT;

	devt = MKDEV(MAJOR(obmm_devt), reg->regionid);
	cdev_init(&reg->cdevice, &obmm_shm_fops);
	reg->cdevice.owner = THIS_MODULE;
	reg->device.devt = devt;
	reg->device.release = obmm_shm_dev_release;
	reg->device.parent = obmm_shm_rootdev;
	device_initialize(&reg->device);

	ret = dev_set_name(&reg->device, "obmm_shmdev%d", reg->regionid);
	if (ret) {
		pr_err("Failed to set name for shmdev %d. ret=%pe\n", reg->regionid, ERR_PTR(ret));
		goto err_put_dev;
	}

	ret = cdev_device_add(&reg->cdevice, &reg->device);
	if (ret) {
		pr_err("Failed to add shm device %d. ret=%pe\n", reg->regionid, ERR_PTR(ret));
		goto err_put_dev;
	}

	return 0;

	/* NOTE: If the device is properly initialized, the refcount of module
	 * should be maintained by device kobject (and the associated
	 * obmm_shm_dev_release function). The refcount of region is always
	 * recovered by kobject-triggered release function.
	 */
err_put_dev:
	put_device(&reg->device);
	return ret;
}

void obmm_shm_dev_del(struct obmm_region *reg)
{
	cdev_device_del(&reg->cdevice, &reg->device);
	put_device(&reg->device);
}

int obmm_shm_dev_init(void)
{
	int ret;

	pr_info("shmdev: root device initialization started\n");
	ret = alloc_chrdev_region(&obmm_devt, OBMM_MIN_VALID_REGIONID, OBMM_REGIONID_MAX_COUNT,
				  obmm_shm_region_name);
	if (ret) {
		pr_err("Failed to allocate char device ID. ret=%pe\n", ERR_PTR(ret));
		goto err_reg_alloc;
	}

	obmm_shm_rootdev = root_device_register(obmm_shm_rootdev_name);
	if (IS_ERR_OR_NULL(obmm_shm_rootdev)) {
		pr_err("error register obmm root device\n");
		ret = -ENOMEM;
		goto err_rootdev;
	}

	pr_info("shmdev: root device initialization completed\n");
	return 0;
err_rootdev:
	unregister_chrdev_region(obmm_devt, OBMM_REGIONID_MAX_COUNT);
err_reg_alloc:
	return ret;
}

void obmm_shm_dev_exit(void)
{
	pr_info("shmdev: root device starts shutting down\n");
	root_device_unregister(obmm_shm_rootdev);
	unregister_chrdev_region(obmm_devt, OBMM_REGIONID_MAX_COUNT);
	pr_info("shmdev: root device shut down completed\n");
}
