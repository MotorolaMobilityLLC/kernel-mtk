#ifndef _DEV_H
#define _DEV_H

int init_fastpath_dev(void);
int dest_fastpath_dev(void);

/*----------------------------------------------------------------------------*/
/* MD Direct Tethering only supports some specified network devices,          */
/* which are defined below.                                                   */
/*----------------------------------------------------------------------------*/
extern const char *fastpath_support_dev_names[];
extern const int fastpath_support_dev_num;
bool fastpath_is_support_dev(const char *dev_name);
int fastpath_dev_name_to_id(char *dev_name);
const char *fastpath_id_to_dev_name(int id);
const char *fastpath_data_usage_id_to_dev_name(int id);
bool fastpath_is_lan_dev(char *dev_name);

#endif
