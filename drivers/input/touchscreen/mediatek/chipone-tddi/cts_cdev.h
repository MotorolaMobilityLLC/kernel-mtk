#ifndef CTS_CDEV_H
#define CTS_CDEV_H

#include "cts_config.h"

struct chipone_ts_data;

#ifdef CONFIG_CTS_CDEV
extern int cts_init_cdev(struct chipone_ts_data *cts_data);
extern int cts_deinit_cdev(struct chipone_ts_data *cts_data);
#else /* CONFIG_CTS_CDEV */
static inline int cts_init_cdev(struct chipone_ts_data *cts_data)
{
    return -ENOTSUPP;
}
static inline int cts_deinit_cdev(struct chipone_ts_data *cts_data)
{
    return -ENOTSUPP;
}
#endif /* CONFIG_CTS_CDEV */

#endif /* CTS_CDEV_H */


