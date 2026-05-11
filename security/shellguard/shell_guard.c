#define pr_fmt(fmt) "shell_guard: " fmt
#include <linux/module.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/binfmts.h>
#include <linux/security.h>
#include <linux/glob.h>
#include <linux/highmem.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/ctype.h>
#include <crypto/md5.h>
#include <crypto/hash.h>
#include <linux/lsm_hooks.h>
#include <uapi/linux/prctl.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
struct shell_policy_target {
	struct list_head list;
	char *comm;
};
struct shell_policy_file {
struct list_head list;
int version;
char *name;
char *comm;
char *hash;
};

static int shellguard_enable;
static int shellguard_block;

static int whitelist_list_count;
static LIST_HEAD(whitelist_list_files);
static DEFINE_MUTEX(whitelist_list_mutex);

static int target_list_count;
static LIST_HEAD(target_list_files);
static DEFINE_MUTEX(target_list_mutex);

/****************************************************************
 *
 * MD5SUM: used by configure and verify
 *
 ****************************************************************/

struct sdesc {
	struct shash_desc shash;
	char ctx[];
};

static struct sdesc *init_sdesc(struct crypto_shash *alg)
{
	struct sdesc *sdesc;
	int size;

	size = sizeof(struct shash_desc) + crypto_shash_descsize(alg);
	sdesc = kmalloc(size, GFP_KERNEL);
	if (!sdesc)
		return ERR_PTR(-ENOMEM);
	sdesc->shash.tfm = alg;
	return sdesc;
}

static int calc_hash(struct crypto_shash *alg,
					const unsigned char *data, unsigned int datalen,
					unsigned char *digest)
{
	struct sdesc *sdesc;
	int ret;

	sdesc = init_sdesc(alg);
	if (IS_ERR(sdesc)) {
			pr_warn("can't alloc sdesc\n");
			return PTR_ERR(sdesc);
	}

	ret = crypto_shash_digest(&sdesc->shash, data, datalen, digest);
	kfree(sdesc);
	return ret;
}

static int test_hash(const unsigned char *data, unsigned int datalen,
					unsigned char *digest)
{
	struct crypto_shash *alg;
	char *hash_alg_name = "md5";
	int ret;

	alg = crypto_alloc_shash(hash_alg_name, 0, 0);
	if (IS_ERR(alg)) {
			pr_warn("can't alloc alg %s\n", hash_alg_name);
			return PTR_ERR(alg);
	}
	ret = calc_hash(alg, data, datalen, digest);
	crypto_free_shash(alg);
	return ret;
}

static int shellguard_digest_file(const char *file_name, char *hash)
{
	void *file_buf = NULL;
	size_t file_size;
	char digest[MD5_DIGEST_SIZE];
	int ret, i;

	pr_debug("%s: file:%s\n", __func__, file_name);
	ret = kernel_read_file_from_path(file_name, 0, &file_buf, INT_MAX,
			NULL, READING_UNKNOWN);
	if (ret < 0) {
		pr_warn("%s: read file failed:%s\n", __func__, file_name);
		goto out;
	}
	file_size = ret;
	ret = test_hash(file_buf, file_size, digest);
	if (ret) {
		pr_warn("%s: digest file failed:%s\n", __func__, file_name);
		goto out_free;
	}

	for (i = 0; i < MD5_DIGEST_SIZE; ++i)
		snprintf(hash + i * 2, 3, "%02x", digest[i] & 0xffu);
	pr_debug("%s: file:%s digest:%s\n", __func__, file_name, hash);

	ret = 0;

out_free:
	vfree(file_buf);
out:
	return ret;
}

/****************************************************************
 *
 * Shell Guard Part
 *
 ****************************************************************/

static inline bool shellguard_enabled(void)
{
	return shellguard_enable == 1;
}

static inline bool shellguard_block_enabled(void)
{
	return shellguard_block == 1;
}



static int get_process_argvs(struct linux_binprm *bprm, char *buf, int size)
{
	unsigned long pos = bprm->p;
	int offset = pos % PAGE_SIZE;
	int argv_count = bprm->argc;
	struct page *page;
	char *start = buf;
	int the_first = 1;
	int len;

	len = snprintf(buf, size, "%s", bprm->filename);
	if (len >= size)
		return 0;
	buf += len;
	size -= len;

	pr_debug("%s **************************\n", __func__);
	pr_debug("argc %d filename: %s\n", argv_count, bprm->filename);
	pr_debug("command line: >%s<\n", start);

	if (argv_count == 1)
		return len;

	while (argv_count && size) {
		/* Same with get_arg_page(bprm, pos, 0) in fs/exec.c */
#ifdef CONFIG_MMU
		mmap_read_lock(bprm->mm);
		if (get_user_pages_remote(bprm->mm, pos, 1,
				0, &page, NULL) <= 0) {
			mmap_read_unlock(bprm->mm);
			return buf - start;
		}
		mmap_read_unlock(bprm->mm);
#else
		page = bprm->page[pos / PAGE_SIZE];
#endif
		if (page) {
			char *kaddr = kmap_atomic(page);

			pos += PAGE_SIZE - offset;
			while (offset < PAGE_SIZE && size) {
				const unsigned char c = kaddr[offset++];
				if (c) {
					if (!the_first) {
						*buf++ = c;
						size--;
					}
					continue;
				}
				the_first = 0;
				pr_debug("argc %d, size %d, offset %d\n",
					 argv_count, size, offset);
				pr_debug("command line: >%s<\n", start);
				argv_count--;
				if (argv_count != 0 && size) {
					*buf++ = ' ';
					size--;
				} else
					break;
			}
			offset = 0;
			kunmap_atomic(kaddr);
#ifdef CONFIG_MMU
			put_page(page);
#endif
		}
	}
	pr_debug("%s: command_line:>%s< size:%ld\n", __func__, start,
			strlen(start));
	return buf - start;
}

static int is_shellguard_file_hash_match(char *whitehash,
		struct linux_binprm *bprm)
{
	int ret;
	char hash[MD5_DIGEST_SIZE * 2 + 1];

	ret = shellguard_digest_file(bprm->filename, hash);
	if (ret)
		return ret;

	ret = strncmp(hash, whitehash, MD5_DIGEST_SIZE * 2 + 1);
	if (ret)
		return -EACCES;

	return 0;
}

static int is_shellguard_permitted_file(char *file_name,
		struct linux_binprm *bprm)
{
	struct shell_policy_file *entry;
	int ret = -EACCES;

	mutex_lock(&whitelist_list_mutex);
	list_for_each_entry(entry, &whitelist_list_files, list) {
		if (glob_match(entry->comm, current->comm) &&
			glob_match(entry->name, file_name)) {
			pr_debug("%s: comm:%s name:>%s<\n", __func__,
					entry->comm, entry->name);
			ret = is_shellguard_file_hash_match(entry->hash, bprm);

			/*
			 * If more than one command line matches, which will happed
			 * because '*' or '/', we may have more than one md5sum.
			 */
			if (ret == -EACCES)
				continue;
			else
				break;
		}
	}
	mutex_unlock(&whitelist_list_mutex);
	return ret;
}

static bool is_shellguard_target_comm(struct task_struct *tsk)
{
	struct shell_policy_target *entry;
	bool ret = false;

	mutex_lock(&target_list_mutex);
	list_for_each_entry(entry, &target_list_files, list) {
		if (glob_match(entry->comm, tsk->comm)) {
			ret = true;
			goto out;
		}
	}

out:
	mutex_unlock(&target_list_mutex);
	return ret;
}

int shellguard_bprm_check(struct linux_binprm *bprm)
{
	int ret;
	char *command_line = NULL;

	if (!shellguard_enabled())
		return 0;

	if (!is_shellguard_target_comm(current))
		return 0;

	command_line = (char *)kzalloc(1024, GFP_KERNEL);
	if (!command_line) {
		ret = -ENOMEM;
		goto out;
	}

	if (!get_process_argvs(bprm, command_line, 1024 - 1)) {
		ret = -EINVAL;
		goto out_free;
	}

	ret = is_shellguard_permitted_file(command_line, bprm);
	if (!ret)
		goto out_free;

	pr_info("%d execute >%s<, blocked: %s.\n",
		task_ppid_nr(current), command_line, shellguard_block_enabled() ? "true" : "false");

	if (!shellguard_block_enabled())
		ret = 0;

out_free:
	if (command_line)
		kfree(command_line);
out:
	return ret;
}

/****************************************************************
 *
 * prctl syscall Guard Part
 *
 ****************************************************************/
static bool task_is_process(struct task_struct *tsk)
{
	return tsk->pid == tsk->tgid;
}
/*
 * Return 0 on success, -ve on error.  -ENOSYS is returned when shellguard
 * does not handle the given option.
 */
int set_task_comm_check(int option, unsigned long arg2, unsigned long arg3,
		   unsigned long arg4, unsigned long arg5)
{
	int ret = -ENOSYS;
	unsigned char comm[sizeof(current->comm)];

	if (option != PR_SET_NAME)
		return ret;

	if (!shellguard_enabled())
		return ret;

	if (!task_is_process(current)) // thread, skip
		return ret;

	if (!is_shellguard_target_comm(current))
		return ret;

	comm[sizeof(current->comm) - 1] = 0;
	if (strncpy_from_user(comm, (char __user *)arg2,
		sizeof(current->comm) - 1) < 0)
		return -EFAULT; // invalid

	pr_info("%s (PID: %d) execute setname: %s -> %s (PID: %d), block: %s.\n", current->comm, current->pid, current->comm, comm, current->pid, shellguard_block_enabled() ? "true" : "false");

	if (shellguard_block_enabled())
		ret = -EPERM;

	return ret;
}

/******************************************************************
 *
 * Control Part
 *
 *****************************************************************/

#define MAX_WHITE_FILE_COUNT    (1024)
#define MAX_TARGET_FILE_COUNT	(1024)

static struct dentry *shellguard_dir;
static struct dentry *shellguard_dentry_enable;
static struct dentry *shellguard_dentry_block;
static struct dentry *shellguard_dentry_whitelist;
static struct dentry *shellguard_dentry_target;

static int shellguard_enable_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", shellguard_enable);
	return 0;
}

static ssize_t shellguard_enable_write(struct file *file,
				       const char __user *buf, size_t count,
				       loff_t *ppos)
{
	char str[16] = { 0 };
	if (count >= 1) {
		if (copy_from_user(str, buf, 1))
			goto out;

		if (strcmp(str, "1") == 0) {
			shellguard_enable = 1;
		} else if (strcmp(str, "0") == 0) {
			shellguard_enable = 0;
		}
	}

 out:
	return count;
}

static int shellguard_enable_open(struct inode *inode, struct file *file)
{
	return single_open(file, shellguard_enable_show, NULL);
}

static const struct file_operations shellguard_enable_fops = {
	.open = shellguard_enable_open,
	.read = seq_read,
	.write = shellguard_enable_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int shellguard_block_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", shellguard_block);
	return 0;
}

static ssize_t shellguard_block_write(struct file *file, const char __user *buf,
				      size_t count, loff_t *ppos)
{
	char str[16] = { 0 };
	if (count >= 1) {
		if (copy_from_user(str, buf, 1))
			goto out;

		if (strcmp(str, "1") == 0) {
			shellguard_block = 1;
		} else if (strcmp(str, "0") == 0) {
			shellguard_block = 0;
		}
	}

 out:
	return count;
}

static int shellguard_block_open(struct inode *inode, struct file *file)
{
	return single_open(file, shellguard_block_show, NULL);
}

static const struct file_operations shellguard_block_fops = {
	.open = shellguard_block_open,
	.read = seq_read,
	.write = shellguard_block_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static void *whitelist_m_start(struct seq_file *m, loff_t *pos)
{
	mutex_lock(&whitelist_list_mutex);
	return seq_list_start(&whitelist_list_files, *pos);
}

static void *whitelist_m_next(struct seq_file *m, void *p, loff_t *pos)
{
	return seq_list_next(p, &whitelist_list_files, pos);
}

static void whitelist_m_stop(struct seq_file *m, void *p)
{
	mutex_unlock(&whitelist_list_mutex);
}

static int whitelist_m_show(struct seq_file *m, void *p)
{
	struct shell_policy_file *entry;
	entry = list_entry(p, struct shell_policy_file, list);
	seq_printf(m, "%s:%s:%s\n", entry->comm, entry->name, entry->hash);
	return 0;
}

static const struct seq_operations whitelist_list_files_op = {
	.start = whitelist_m_start,
	.next = whitelist_m_next,
	.stop = whitelist_m_stop,
	.show = whitelist_m_show
};

static int whitelist_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &whitelist_list_files_op);
}

#define SHELLGUARD_ADD		"add"
#define SHELLGUARD_DEL		"del"
#define SHELLGUARD_DES		"destroy"
#define SHELLGUARD_DES_ALL	":destroy"

static int shellguard_digest_file_control(char *pfile, char *phash)
{
	char *file;
	int ret, i;

	file = kstrndup(pfile, PATH_MAX, GFP_KERNEL);
	if (!file)
		return -ENOMEM;

	for (i = 0; i < strlen(file); ++i) {
		if (file[i] == ' ') {
			file[i] = '\0';
			break;
		}
	}
	pr_debug("%s: locate file: %s\n", __func__, file);

	ret = shellguard_digest_file(file, phash);

	kfree(file);
	return ret;
}

static int add_whitelist(char **cmd, bool calculate)
{
	int ret = 0;
	struct shell_policy_file *entry;
	char *pcomm, *pfile, *phash, *pcomd, *uhash;

	pcomm = cmd[0];
	pfile = cmd[1];

	if (calculate) {
		pcomd = cmd[2];
	} else {
		uhash = cmd[2];
		pcomd = cmd[3];
	}

	if (!strlen(pcomm) || !strlen(pfile))
		return -EINVAL;
	if (strcmp(pcomd, SHELLGUARD_ADD))
		return -EINVAL;

	phash = kmalloc(MD5_DIGEST_SIZE * 2 + 1, GFP_KERNEL);
	if (!phash)
		return -ENOMEM;

	if (calculate) {
		ret = shellguard_digest_file_control(pfile, phash);
		if (ret)
			goto calc_failed;
	} else {
		strncpy(phash, uhash, MD5_DIGEST_SIZE * 2);
		phash[MD5_DIGEST_SIZE * 2] = '\0';
	}

	mutex_lock(&whitelist_list_mutex);
	if (whitelist_list_count >= MAX_WHITE_FILE_COUNT) {
		ret = -ENOSPC;
		goto failed;
	}

	list_for_each_entry(entry, &whitelist_list_files, list) {
		if (!strcmp(entry->name, pfile) &&
			!strcmp(entry->comm, pcomm)) {
			ret = -EEXIST;
			goto failed;
		}
	}

	entry = (void *)kzalloc(sizeof(*entry), GFP_KERNEL);
	if (!entry) {
		ret = -ENOMEM;
		goto failed;
	}

	entry->comm = pcomm;
	entry->name = pfile;
	entry->hash = phash;
	/* eat phash */
	phash = NULL;
	list_add(&entry->list, &whitelist_list_files);
	whitelist_list_count += 1;

failed:
	mutex_unlock(&whitelist_list_mutex);
calc_failed:
	/* kfree() will handle NULL pointer */
	kfree(phash);
	return ret;
}

static int del_whitelist(char **cmd)
{
	struct shell_policy_file *entry, *n;
	char *pcomm, *pfile, *pcomd;

	pcomm = cmd[0];
	pfile = cmd[1];
	pcomd = cmd[2];

	if (!strlen(pcomm) || !strlen(pfile))
		return -EINVAL;
	if (strcmp(pcomd, SHELLGUARD_DEL))
		return -EINVAL;

	mutex_lock(&whitelist_list_mutex);
	list_for_each_entry_safe(entry, n, &whitelist_list_files, list) {
		if (!strcmp(entry->name, pfile) &&
		    !strcmp(entry->comm, pcomm)) {
			list_del(&entry->list);
			kfree(entry->comm);
			kfree(entry->hash);
			kfree(entry);
			whitelist_list_count -= 1;
			break;
		}
	}
	mutex_unlock(&whitelist_list_mutex);

	return 0;
}

static int destroy_whitelist_one(char **cmd)
{
	struct shell_policy_file *entry, *n;
	char *pcomm, *pcomd;

	pcomm = cmd[0];
	pcomd = cmd[1];

	if (!strlen(pcomm))
		return -EINVAL;
	if (strcmp(pcomd, SHELLGUARD_DES))
		return -EINVAL;

	mutex_lock(&whitelist_list_mutex);
	list_for_each_entry_safe(entry, n, &whitelist_list_files, list) {
		if (!strcmp(entry->comm, pcomm)) {
			list_del(&entry->list);
			kfree(entry->comm);
			kfree(entry->hash);
			kfree(entry);
			whitelist_list_count -= 1;
		}
	}
	mutex_unlock(&whitelist_list_mutex);

	return 0;
}

static void destroy_whitelist(void)
{
	struct shell_policy_file *entry, *n;

	mutex_lock(&whitelist_list_mutex);
	list_for_each_entry_safe(entry, n, &whitelist_list_files, list) {
		list_del(&entry->list);
		kfree(entry->comm);
		kfree(entry->hash);
		kfree(entry);
		whitelist_list_count -= 1;
	}
	WARN_ON(whitelist_list_count);
	mutex_unlock(&whitelist_list_mutex);
}

static int shellguard_write_parser(char *str, char **cmd, int max_cmd)
{
	int cnt = 0;

	while (str) {
		if (cnt >= max_cmd)
			return -EINVAL;

		cmd[cnt++] = str;

		str = strstr(str, ":");
		if (!str)
			break;
		*str = '\0';
		str += 1;
	}

	if (cnt == 2)
		pr_debug("%s: [0]:%s [1]:%s\n", __func__, cmd[0], cmd[1]);
	else if (cnt == 3)
		pr_debug("%s: [0]:%s [1]:%s [2]:%s\n", __func__, cmd[0],
			cmd[1], cmd[2]);
	else if (cnt == 4)
		pr_debug("%s: [0]:%s [1]:%s [2]:%s [3]:%s\n", __func__, cmd[0],
			cmd[1], cmd[2], cmd[3]);
	else
		pr_warn("%s: cnt:%d\n", __func__, cnt);
	return cnt;
}

static ssize_t whitelist_write(struct file *file, const char __user *buf,
			       size_t count, loff_t *ppos)
{
	/*
	 * comm:file:hash:add
	 * comm:file:add	// kernel calculates md5sum itself
	 * comm:file:del
	 * comm:destroy
	 */
	char *cmd[4];
	char *str = NULL;
	int len, ret;

	if (count) {
		len = min((size_t)1024, count + 1);
		str = (char *)kzalloc(len, GFP_KERNEL);
		if (!str) {
			ret = -ENOMEM;
			goto failed;
		}

		if (copy_from_user(str, buf, len - 1)) {
			ret = -EINVAL;
			goto failed;
		}

		/* comm must point to the begin of str */
		if (isspace(str[0])) {
			ret = -EINVAL;
			goto failed;
		}
		str = strim(str);

		if (!strcmp(str, SHELLGUARD_DES_ALL)) {
			destroy_whitelist();
			goto out;
		}

		ret = shellguard_write_parser(str, cmd, 4);
		if (ret < 0)
			goto failed;

		if (ret == 4 && !strcmp(cmd[3], SHELLGUARD_ADD)) {
			ret = add_whitelist(cmd, false);
			if (ret)
				goto failed;
			str = NULL;
		} else if (ret == 3 && !strcmp(cmd[2], SHELLGUARD_ADD)) {
			ret = add_whitelist(cmd, true);
			if (ret)
				goto failed;
			str = NULL;
		} else if (ret == 3 && !strcmp(cmd[2], SHELLGUARD_DEL)) {
			ret = del_whitelist(cmd);
			if (ret)
				goto failed;
		} else if (ret == 2) {
			ret = destroy_whitelist_one(cmd);
			if (ret)
				goto failed;
		} else {
			ret = -EINVAL;
			goto failed;
		}
	}

out:
	ret = count;
failed:
	/* kfree will handle NULL pointers */
	kfree(str);
	return ret;
}

static const struct file_operations shellguard_whitelist_fops = {
	.open = whitelist_open,
	.read = seq_read,
	.write = whitelist_write,
	.llseek = seq_lseek,
	.release = seq_release,
};

static void *target_m_start(struct seq_file *m, loff_t *pos)
{
	mutex_lock(&target_list_mutex);
	return seq_list_start(&target_list_files, *pos);
}

static void *target_m_next(struct seq_file *m, void *p, loff_t *pos)
{
	return seq_list_next(p, &target_list_files, pos);
}

static void target_m_stop(struct seq_file *m, void *p)
{
	mutex_unlock(&target_list_mutex);
}

static int target_m_show(struct seq_file *m, void *p)
{
	struct shell_policy_target *entry;
	entry = list_entry(p, struct shell_policy_target, list);
	seq_printf(m, "%s\n", entry->comm);
	return 0;
}

static const struct seq_operations target_list_files_op = {
	.start = target_m_start,
	.next = target_m_next,
	.stop = target_m_stop,
	.show = target_m_show
};

static int target_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &target_list_files_op);
}

static int add_target(char *str)
{
	int ret = 0;
	struct shell_policy_target *entry;

	if (!strlen(str))
		return -EINVAL;

	mutex_lock(&target_list_mutex);
	if (target_list_count >= MAX_TARGET_FILE_COUNT) {
		ret = -ENOSPC;
		goto failed;
	}

	list_for_each_entry(entry, &target_list_files, list) {
		if (!strcmp(entry->comm, str)) {
			ret = -EEXIST;
			goto failed;
		}
	}

	entry = (void *)kzalloc(sizeof(*entry), GFP_KERNEL);
	if (!entry) {
		ret = -ENOMEM;
		goto failed;
	}

	entry->comm = str;
	list_add(&entry->list, &target_list_files);
	target_list_count += 1;

failed:
	mutex_unlock(&target_list_mutex);
	return ret;
}

static int del_target(char *str)
{
	struct shell_policy_target *entry, *n;

	if (!strlen(str))
		return -EINVAL;

	mutex_lock(&target_list_mutex);
	list_for_each_entry_safe(entry, n, &target_list_files, list) {
		if (!strcmp(entry->comm, str)) {
			list_del(&entry->list);
			kfree(entry->comm);
			kfree(entry);
			target_list_count -= 1;
			break;
		}
	}
	mutex_unlock(&target_list_mutex);
	return 0;
}

static void destroy_target(void)
{
	struct shell_policy_target *entry, *n;

	mutex_lock(&target_list_mutex);
	list_for_each_entry_safe(entry, n, &target_list_files, list) {
		list_del(&entry->list);
		kfree(entry->comm);
		kfree(entry);
		target_list_count -= 1;
	}
	WARN_ON(target_list_count);
	mutex_unlock(&target_list_mutex);
}

static ssize_t target_write(struct file *file, const char __user *buf,
			    size_t count, loff_t *ppos)
{
	char *cmd[2];
	char *str = NULL;
	int len, ret;

	if (count) {
		len = min((size_t)1024, count + 1);
		str = (char *)kzalloc(len, GFP_KERNEL);
		if (!str) {
			ret = -ENOMEM;
			goto failed;
		}

		if (copy_from_user(str, buf, len - 1)) {
			ret = -EINVAL;
			goto failed;
		}

		/* str should never be forward */
		if (isspace(str[0])) {
			ret = -EINVAL;
			goto failed;
		}
		str = strim(str);

		if (!strcmp(str, SHELLGUARD_DES_ALL)) {
			destroy_target();
			goto out;
		}

		ret = shellguard_write_parser(str, cmd, 2);
		if (ret < 0)
			goto failed;
		if (ret != 2) {
			ret = -EINVAL;
			goto failed;
		}

		if (!strcmp(cmd[1], SHELLGUARD_ADD)) {
			ret = add_target(cmd[0]);
			if (ret)
				goto failed;
			str = NULL;
		} else if (!strcmp(cmd[1], SHELLGUARD_DEL)) {
			ret = del_target(cmd[0]);
			if (ret)
				goto failed;
		} else {
			ret = -EINVAL;
			goto failed;
		}
	}

out:
	ret = count;
failed:
	/* kfree() will handle NULL pointers */
	kfree(str);
	return ret;
}

static const struct file_operations shellguard_target_fops = {
	.open = target_open,
	.read = seq_read,
	.write = target_write,
	.llseek = seq_lseek,
	.release = seq_release,
};

static void shellguard_secfs_exit(void)
{
	pr_crit("SecurityFS: Uninitializing...\n");
	/* securityfs_remove() will deal with NULL pointers */
	securityfs_remove(shellguard_dentry_target);
	securityfs_remove(shellguard_dentry_whitelist);
	securityfs_remove(shellguard_dentry_block);
	securityfs_remove(shellguard_dentry_enable);
	securityfs_remove(shellguard_dir);
	return;
}

static int shellguard_secfs_init(void)
{
	int ret = 0;

	pr_crit("SecurityFS: Initializing...\n");

	shellguard_dir = securityfs_create_dir("shellguard", NULL);
	if (IS_ERR(shellguard_dir)) {
		ret = PTR_ERR(shellguard_dir);
		goto err;
	}

	shellguard_dentry_enable =
	    securityfs_create_file("enable", S_IRUSR | S_IWUSR,
				   shellguard_dir, NULL,
				   &shellguard_enable_fops);
	if (IS_ERR(shellguard_dentry_enable)) {
		ret = PTR_ERR(shellguard_dentry_enable);
		goto err;
	}

	shellguard_dentry_block =
	    securityfs_create_file("block", S_IRUSR | S_IWUSR,
				   shellguard_dir, NULL,
				   &shellguard_block_fops);
	if (IS_ERR(shellguard_dentry_block)) {
		ret = PTR_ERR(shellguard_dentry_block);
		goto err;
	}

	shellguard_dentry_whitelist =
	    securityfs_create_file("whitelist", S_IRUSR | S_IWUSR,
				   shellguard_dir, NULL,
				   &shellguard_whitelist_fops);
	if (IS_ERR(shellguard_dentry_whitelist)) {
		ret = PTR_ERR(shellguard_dentry_whitelist);
		goto err;
	}

	shellguard_dentry_target =
		securityfs_create_file("target", S_IRUSR | S_IWUSR,
				       shellguard_dir, NULL,
				       &shellguard_target_fops);
	if (IS_ERR(shellguard_dentry_target)) {
		ret = PTR_ERR(shellguard_dentry_target);
		goto err;
	}

	pr_crit("SecurityFS: Initialized...\n");
	return 0;
err:
	shellguard_secfs_exit();
	return ret;
}

static struct security_hook_list shellguard_hooks[] __ro_after_init = {
	LSM_HOOK_INIT(task_prctl, set_task_comm_check),
	LSM_HOOK_INIT(bprm_check_security, shellguard_bprm_check),
};

static int __init shellguard_init(void)
{
	pr_crit("ShellGuard LSM hooks Initializing...\n");
	security_add_hooks(shellguard_hooks,
				ARRAY_SIZE(shellguard_hooks), "shellguard");
	pr_crit("ShellGuard LSM hooks Initialized.\n");

	return 0;
}

DEFINE_LSM(shellguard) = {
	.name = "shellguard",
	.init = shellguard_init,
};

/*
 * lsm is more earlier to init than securityfs, so we need to init
 * securityfs in fs_initcall
 */
static int __init shellguard_secfs_late_init(void)
{
	return shellguard_secfs_init();
}
fs_initcall(shellguard_secfs_late_init);

