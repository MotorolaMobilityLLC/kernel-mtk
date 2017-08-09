#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/module.h>
#include <linux/poll.h>

void __weak ccci_power_off(void)
{

}
int __weak exec_ccci_kern_func_by_md_id(int md_id, unsigned int id, char *buf, unsigned int len)
{
	pr_debug("[ccci/dummy] %s is not supported!\n", __func__);
	return 0;
}

int __weak switch_sim_mode(int id, char *buf, unsigned int len)
{
	pr_debug("[ccci/dummy] %s is not supported!\n", __func__);
	return 0;
}

unsigned int __weak get_sim_switch_type(void)
{
	pr_debug("[ccci/dummy] %s is not supported!\n", __func__);
	return 0;
}

unsigned int __weak get_modem_is_enabled(int md_id)
{
	pr_debug("[ccci/dummy] %s is not supported!\n", __func__);
	return 0;
}

int __weak register_ccci_sys_call_back(int md_id, unsigned int id, int (*func) (int, int))
{
	pr_debug("[ccci/dummy] %s is not supported!\n", __func__);
	return 0;
}

unsigned int __weak mt_irq_get_pending(unsigned int irq)
{
	pr_debug("[ccci/dummy] %s is not supported!\n", __func__);
	return 0;
}

void __weak notify_time_update(void)
{
	pr_debug("[ccci/dummy] %s is not supported!\n", __func__);
}

unsigned long __weak ccci_get_md_boot_count(int md_id)
{
	pr_debug("[ccci/dummy] %s is not supported!\n", __func__);
	return 0;
}
char * __weak ccci_get_ap_platform(void)
{
	pr_debug("[ccci/dummy] %s is not supported!\n", __func__);
	return "MTxxxxE1";
}
