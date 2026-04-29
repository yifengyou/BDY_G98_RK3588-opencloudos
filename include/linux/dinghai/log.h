#ifndef __KERNEL_LOG_H__
#define __KERNEL_LOG_H__

#include <linux/kernel.h>
#include <linux/printk.h>

#define MODULE_CMD            "zxdh_cmd"
#define MODULE_NP             "zxdh_np"
#define MODULE_PF             "zxdh_pf"
#define MODULE_PTP            "zxdh_ptp"
#define MODULE_TSN            "zxdh_tsn"
#define MODULE_LAG            "zxdh_lag"
#define MODULE_DHTOOLS        "zxdh_tool"
#define MODULE_SEC            "zxdh_sec"
#define MODULE_MPF            "zxdh_mpf"
#define MODULE_FUC_HP         "zxdh_func_hp"
#define MODULE_UACCE          "zxdh_uacce"
#define MODULE_HEAL           "zxdh_health"

extern int debug_print;
#define DH_LOG_EMERG(module, fmt, arg...)       \
    printk(KERN_EMERG "[%s][%s][%d] "fmt"", module, __func__, __LINE__, ##arg);

#define DH_LOG_ALERT(module, fmt, arg...)       \
    printk(KERN_ALERT "[%s][%s][%d] "fmt"", module, __func__, __LINE__, ##arg);

#define DH_LOG_CRIT(module, fmt, arg...)       \
    printk(KERN_CRIT "[%s][%s][%d] "fmt"", module, __func__, __LINE__, ##arg);

#define DH_LOG_ERR(module, fmt, arg...)        \
    printk(KERN_ERR "[%s][%s][%d] "fmt"", module, __func__, __LINE__, ##arg);

#define DH_LOG_WARNING(module, fmt, arg...)    \
    printk(KERN_WARNING "[%s][%s][%d] "fmt"", module, __func__, __LINE__, ##arg);

#define DH_LOG_INFO(module, fmt, arg...)       \
    printk(KERN_INFO "[%s][%s][%d] "fmt"", module, __func__, __LINE__, ##arg);

#define DH_LOG_DEBUG(module, fmt, arg...)      \
do {                                       \
    if (debug_print)                     \
        printk(KERN_DEBUG "[%s][%s][%d] "fmt"", module, __func__, __LINE__, ##arg); \
} while (0)

#endif /* __KERNEL_LOG_H__ */
