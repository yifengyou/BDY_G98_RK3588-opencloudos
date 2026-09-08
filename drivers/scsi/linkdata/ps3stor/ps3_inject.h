/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_INJECT_H_
#define _PS3_INJECT_H_

struct PS3Inject {
	struct mutex lock;
	struct list_head scsi_rw_list;
	struct list_head scsi_task_list;
	struct list_head mgr_list;
};

struct PS3HitCmd {
	struct list_head scsi_rw_list;
	struct list_head scsi_task_list;
	struct list_head mgr_list;
};

struct ps3_scsi_sense {
	unsigned char resp_code;
	unsigned char sense_key;
	unsigned char asc;
	unsigned char ascq;
};


struct PS3ScsiErrorReply {
	int result;
	struct ps3_scsi_sense sshdr;
};


struct PS3ScsiForceReply {
	unsigned int step;
};

union PS3CmdInjectDeal {
	struct PS3ScsiErrorReply errReply;
	struct PS3ScsiForceReply forceReply;
};

struct inject_scsi_cmds_t {
	unsigned int host_no;
	unsigned int id;
	unsigned int channel;
	struct scsi_device *device;
	unsigned long long lba;
	unsigned int len;
	unsigned int dealType;
	union PS3CmdInjectDeal cmdDeal;
	unsigned int inject_count;
};

struct inject_scsi_task_cmds_t {
	unsigned int host_no;
	unsigned int id;
	unsigned int channel;
	struct scsi_device *device;
	unsigned char cmd_type;
	unsigned char cmd_sub_type;
	unsigned int dealType;
	union PS3CmdInjectDeal cmdDeal;
	unsigned int inject_count;
};

struct inject_mgr_cmds_t {
	unsigned int host_no;
	unsigned char cmd_type;
	unsigned char cmd_sub_type;
	unsigned int dealType;
	unsigned int errType;
	unsigned int inject_count;
};

union PS3CmdInject {
	struct inject_scsi_cmds_t scsi_cmd;
	struct inject_scsi_task_cmds_t scsi_task_cmd;
	struct inject_mgr_cmds_t mgr_cmd;
};
struct inject_cmds_t {
	struct list_head list;
	union PS3CmdInject item;
};

struct inject_hit_cmds_t {
	struct list_head list;
	struct ps3_cmd *cmd;
	struct inject_cmds_t *pitem;
};

enum {
	PS3_SCSI_CMD_TIMEOUT_FORCE_REPLY = 1,
	PS3_SCSI_CMD_ERROR = 2,
	PS3_SCSI_TASK_CMD_NORMAL = 3,
	PS3_SCSI_TASK_CMD_TIMEOUT = 4,
	PS3_SCSI_TASK_CMD_ERROE = 5,
	PS3_MGR_CMD_NORMAL = 6,
	PS3_MGR_CMD_TIMEOUT = 7,
	PS3_MGR_CMD_ERROE = 8,
};
struct PS3Inject *get_inject(void);

#ifndef PS3_UT
int ps3_scsi_rw_cmd_filter_handle(struct scsi_cmnd *scmd);
int ps3_scsi_task_cmd_filter_handle(struct ps3_cmd *cmd);
int ps3_mgr_cmd_filter_handle(struct ps3_cmd *cmd);
#endif

unsigned char ps3_add_cmd_filter(struct ps3_instance *instance,
				 struct PS3CmdWord *cmd_word);

void ps3_delete_scsi_rw_inject(struct inject_cmds_t *this_pitem);
void ps3_delete_scsi_task_inject(struct inject_cmds_t *this_pitem);
void ps3_delete_mgr_inject(struct inject_cmds_t *this_pitem);

struct PS3HitCmd *get_hit_inject(void);

void ps3_inject_init(void);

void ps3_inject_exit(void);

#endif
