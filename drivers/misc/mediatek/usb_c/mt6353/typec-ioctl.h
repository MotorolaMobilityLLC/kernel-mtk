#ifndef _TYPEC_IOCTL_H
#define _TYPEC_IOCTL_H

int typec_cdev_init(struct device *parent, struct typec_hba *hba, int id);
void typec_cdev_remove(struct typec_hba *hba);
int typec_cdev_module_init(void);
void typec_cdev_module_exit(void);

#endif /* _TYPEC_IOCTL_H */
