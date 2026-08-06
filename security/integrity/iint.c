// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2008 IBM Corporation
 *
 * Authors:
 * Mimi Zohar <zohar@us.ibm.com>
 *
 * File: integrity_iint.c
 *	- implements the integrity hooks: integrity_inode_alloc,
 *	  integrity_inode_free
 *	- cache integrity information associated with an inode
 *	  using a rbtree tree.
 */
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/rbtree.h>
#include <linux/file.h>
#include <linux/uaccess.h>
#include <linux/security.h>
#include <linux/lsm_hooks.h>
#include "integrity.h"

static struct rb_root integrity_iint_tree = RB_ROOT;
static DEFINE_RWLOCK(integrity_iint_lock);
static struct kmem_cache *iint_cache __read_mostly;

struct dentry *integrity_dir;

bool ima_enabled = false;
bool evm_enabled = false;
bool dim_enabled = false;

EXPORT_SYMBOL(ima_enabled);
EXPORT_SYMBOL(evm_enabled);
EXPORT_SYMBOL(dim_enabled);

/*
 * integrity_param - parse "integrity=" parameter to enable
 * integrity subsystems explictly
*/
static int __init integrity_param(char *str) {
	char *token, *p = str;

	while ((token = strsep(&p, "|")) != NULL) {
		if (strncmp(token, "all", 3) == 0 || strncmp(token, "1", 1) == 0) {
			ima_enabled = true;
			evm_enabled = true;
			dim_enabled = true;
			break;
		} else if (strncmp(token, "ima", 3) == 0) {
			ima_enabled = true;
		} else if (strncmp(token, "evm", 3) == 0) {
			/* EVM requires IMA to be enabled first */
			ima_enabled = true;
			evm_enabled = true;
		} else if (strncmp(token, "dim", 3) == 0) {
			dim_enabled = true;
		} else {
			pr_err("integrity: Invalid option '%s'\n", token);
			return -EINVAL;
		}
	}

	return 0;
}
__setup("integrity=", integrity_param);

/**
 * integrity_read_dim_enabled - read() for <securityfs>/integrity/dim_enabled
 * @filp: file pointer, not actually used
 * @buf: where to put the result
 * @count: maximum to send along
 * @ppos: where to start
 *
 * Returns number of bytes read or error code, as appropriate
 * Used to read the dim_enabled status
 */
static ssize_t integrity_read_dim_enabled(struct file *filp, char __user *buf,
				size_t count, loff_t *ppos)
{
	char temp[3];
	ssize_t rc;

	if (*ppos != 0)
		return 0;

	snprintf(temp, sizeof(temp), "%d\n", dim_enabled);
	rc = simple_read_from_buffer(buf, count, ppos, temp, strlen(temp));

	return rc;
}

/**
 * integrity_write_dim_enabled - write() for <securityfs>/integrity/dim_enabled
 * @file: file pointer, not actually used
 * @buf: where to get the data from
 * @count: bytes sent
 * @ppos: where to start
 *
 * Returns number of bytes written or error code, as appropriate
 * Used to set the dim_enabled status
 */
static ssize_t integrity_write_dim_enabled(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	unsigned int val;
	int ret;

	if (count == 0)
		return -EINVAL;

	if(!capable(CAP_SYS_ADMIN))
		return -EPERM;

	ret = kstrtouint_from_user(buf, count, 10, &val);

	if (ret)
		return ret;

	/* Reject invalid values*/
	if (val > 1)
		return -EINVAL;

	dim_enabled = val;

	return count;
}

static const struct file_operations dim_enabled_ops = {
	.read	= integrity_read_dim_enabled,
	.write	= integrity_write_dim_enabled,
	.llseek	= generic_file_llseek,
};

/*
 * __integrity_iint_find - return the iint associated with an inode
 */
static struct integrity_iint_cache *__integrity_iint_find(struct inode *inode)
{
	struct integrity_iint_cache *iint;
	struct rb_node *n = integrity_iint_tree.rb_node;

	while (n) {
		iint = rb_entry(n, struct integrity_iint_cache, rb_node);

		if (inode < iint->inode)
			n = n->rb_left;
		else if (inode > iint->inode)
			n = n->rb_right;
		else
			return iint;
	}

	return NULL;
}

/*
 * integrity_iint_find - return the iint associated with an inode
 */
struct integrity_iint_cache *integrity_iint_find(struct inode *inode)
{
	struct integrity_iint_cache *iint;

	if (!IS_IMA(inode))
		return NULL;

	read_lock(&integrity_iint_lock);
	iint = __integrity_iint_find(inode);
	read_unlock(&integrity_iint_lock);

	return iint;
}

#define IMA_MAX_NESTING (FILESYSTEM_MAX_STACK_DEPTH+1)

/*
 * It is not clear that IMA should be nested at all, but as long is it measures
 * files both on overlayfs and on underlying fs, we need to annotate the iint
 * mutex to avoid lockdep false positives related to IMA + overlayfs.
 * See ovl_lockdep_annotate_inode_mutex_key() for more details.
 */
static inline void iint_lockdep_annotate(struct integrity_iint_cache *iint,
					 struct inode *inode)
{
#ifdef CONFIG_LOCKDEP
	static struct lock_class_key iint_mutex_key[IMA_MAX_NESTING];

	int depth = inode->i_sb->s_stack_depth;

	if (WARN_ON_ONCE(depth < 0 || depth >= IMA_MAX_NESTING))
		depth = 0;

	lockdep_set_class(&iint->mutex, &iint_mutex_key[depth]);
#endif
}

static void iint_init_always(struct integrity_iint_cache *iint,
			     struct inode *inode)
{
	iint->ima_hash = NULL;
	iint->version = 0;
	iint->flags = 0UL;
	iint->atomic_flags = 0UL;
	iint->ima_file_status = INTEGRITY_UNKNOWN;
	iint->ima_mmap_status = INTEGRITY_UNKNOWN;
	iint->ima_bprm_status = INTEGRITY_UNKNOWN;
	iint->ima_read_status = INTEGRITY_UNKNOWN;
	iint->ima_creds_status = INTEGRITY_UNKNOWN;
	iint->evm_status = INTEGRITY_UNKNOWN;
	iint->measured_pcrs = 0;
	mutex_init(&iint->mutex);
	iint_lockdep_annotate(iint, inode);
}

static void iint_free(struct integrity_iint_cache *iint)
{
	kfree(iint->ima_hash);
	mutex_destroy(&iint->mutex);
	kmem_cache_free(iint_cache, iint);
}

/**
 * integrity_inode_get - find or allocate an iint associated with an inode
 * @inode: pointer to the inode
 * @return: allocated iint
 *
 * Caller must lock i_mutex
 */
struct integrity_iint_cache *integrity_inode_get(struct inode *inode)
{
	struct rb_node **p;
	struct rb_node *node, *parent = NULL;
	struct integrity_iint_cache *iint, *test_iint;

	iint = integrity_iint_find(inode);
	if (iint)
		return iint;

	iint = kmem_cache_alloc(iint_cache, GFP_NOFS);
	if (!iint)
		return NULL;

	iint_init_always(iint, inode);

	write_lock(&integrity_iint_lock);

	p = &integrity_iint_tree.rb_node;
	while (*p) {
		parent = *p;
		test_iint = rb_entry(parent, struct integrity_iint_cache,
				     rb_node);
		if (inode < test_iint->inode) {
			p = &(*p)->rb_left;
		} else if (inode > test_iint->inode) {
			p = &(*p)->rb_right;
		} else {
			write_unlock(&integrity_iint_lock);
			kmem_cache_free(iint_cache, iint);
			return test_iint;
		}
	}

	iint->inode = inode;
	node = &iint->rb_node;
	inode->i_flags |= S_IMA;
	rb_link_node(node, parent, p);
	rb_insert_color(node, &integrity_iint_tree);

	write_unlock(&integrity_iint_lock);
	return iint;
}

/**
 * integrity_inode_free - called on security_inode_free
 * @inode: pointer to the inode
 *
 * Free the integrity information(iint) associated with an inode.
 */
void integrity_inode_free(struct inode *inode)
{
	struct integrity_iint_cache *iint;

	if (!IS_IMA(inode))
		return;

	write_lock(&integrity_iint_lock);
	iint = __integrity_iint_find(inode);
	rb_erase(&iint->rb_node, &integrity_iint_tree);
	write_unlock(&integrity_iint_lock);

	iint_free(iint);
}

static void iint_init_once(void *foo)
{
	struct integrity_iint_cache *iint = (struct integrity_iint_cache *) foo;

	memset(iint, 0, sizeof(*iint));
}

static int __init integrity_iintcache_init(void)
{
	iint_cache =
	    kmem_cache_create("iint_cache", sizeof(struct integrity_iint_cache),
			      0, SLAB_PANIC, iint_init_once);
	return 0;
}
DEFINE_LSM(integrity) = {
	.name = "integrity",
	.init = integrity_iintcache_init,
	.order = LSM_ORDER_LAST,
};


/*
 * integrity_kernel_read - read data from the file
 *
 * This is a function for reading file content instead of kernel_read().
 * It does not perform locking checks to ensure it cannot be blocked.
 * It does not perform security checks because it is irrelevant for IMA.
 *
 */
int integrity_kernel_read(struct file *file, loff_t offset,
			  void *addr, unsigned long count)
{
	return __kernel_read(file, addr, count, &offset);
}

/*
 * integrity_load_keys - load integrity keys hook
 *
 * Hooks is called from init/main.c:kernel_init_freeable()
 * when rootfs is ready
 */
void __init integrity_load_keys(void)
{
	ima_load_x509();

	if (!IS_ENABLED(CONFIG_IMA_LOAD_X509))
		evm_load_x509();
}

static int __init integrity_fs_init(void)
{
	struct dentry *dim_file;

	integrity_dir = securityfs_create_dir("integrity", NULL);
	if (IS_ERR(integrity_dir)) {
		int ret = PTR_ERR(integrity_dir);

		if (ret != -ENODEV)
			pr_err("Unable to create integrity sysfs dir: %d\n",
			       ret);
		integrity_dir = NULL;
		return ret;
	}

	dim_file = securityfs_create_file("dim_enabled", 0640, integrity_dir,
		NULL, &dim_enabled_ops);
	if (IS_ERR(dim_file)) {
		int ret = PTR_ERR(dim_file);

		if (ret != -ENODEV)
			pr_err("Unable to create dim_enabled sysfs file: %d\n", ret);
	}

	return 0;
}

late_initcall(integrity_fs_init)
