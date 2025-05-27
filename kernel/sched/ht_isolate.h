#ifndef _HT_ISOLATE_H
#define _HT_ISOLATE_H

#define HT_TOLERATE_TYPE 0
#define HT_SENSITIV_TYPE 1
#define HT_SYS_JUDG_TYPE 2

#define HT_INITIAL_RUN 0
#define HT_IDLE_RUN 1
#define HT_OFFLINE_RUN 2
#define HT_ONLINE_RUN 3

extern int sysctl_ht_isolate;
extern void kick_sibling_offline_task(void);
extern void kick_sibling_offline_run(void);
#ifdef CONFIG_HT_ISOLATE
extern bool sensitive_running_on_sibling(int cpu);
#else
bool sensitive_running_on_sibling(int cpu) { return false; }
#endif
#endif
