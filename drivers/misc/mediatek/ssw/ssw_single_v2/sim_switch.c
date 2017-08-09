
#include <ssw.h>
#include <mach/mt_ccci_common.h>
#if defined(CONFIG_MTK_LEGACY)
#include "cust_gpio_usage.h"
#endif
/*--------------Feature option---------------*/
#define __ENABLE_SSW_SYSFS 1

/*--------------SIM mode list----------------*/
#define SINGLE_TALK_MDSYS		(0x1)
#define SINGLE_TALK_MDSYS_LITE	(0x2)

/*----------------variable define-----------------*/
unsigned int sim_mode_curr = SINGLE_TALK_MDSYS;

struct mutex sim_switch_mutex;

#if !defined(CONFIG_MTK_LEGACY)
struct pinctrl *ssw_pinctrl = NULL;

struct pinctrl_state *hot_plug_mode1 = NULL;
struct pinctrl_state *hot_plug_mode2 = NULL;
struct pinctrl_state *two_sims_bound_to_md1 = NULL;
struct pinctrl_state *sim1_md3_sim2_md1 = NULL;
#endif

static int set_sim_gpio(unsigned int mode);
static int get_current_ssw_mode(void)
{
	return sim_mode_curr;
}

unsigned int get_sim_switch_type(void)
{
	pr_debug("[ccci/ssw]ssw_single_v2\n");
	return SSW_INTERN;
}

/*---------------------------------------------------------------------------*/
/*define sysfs entry for configuring debug level and sysrq*/
static ssize_t ssw_attr_show(struct kobject *kobj, struct attribute *attr,
			     char *buffer);
static ssize_t ssw_attr_store(struct kobject *kobj, struct attribute *attr,
			      const char *buffer, size_t size);
static ssize_t ssw_mode_show(struct kobject *kobj, char *page);
static ssize_t ssw_mode_store(struct kobject *kobj, const char *page,
			      size_t size);

const struct sysfs_ops ssw_sysfs_ops = {
	.show = ssw_attr_show,
	.store = ssw_attr_store,
};

struct ssw_sys_entry {
	struct attribute attr;
	 ssize_t (*show)(struct kobject *kobj, char *page);
	 ssize_t (*store)(struct kobject *kobj, const char *page, size_t size);
};

static struct ssw_sys_entry mode_entry = {
	{.name = "mode", .mode = S_IRUGO | S_IWUSR},	/*remove  .owner = NULL, */
	ssw_mode_show,
	ssw_mode_store,
};

struct attribute *ssw_attributes[] = {
	&mode_entry.attr,
	NULL,
};

struct kobj_type ssw_ktype = {
	.sysfs_ops = &ssw_sysfs_ops,
	.default_attrs = ssw_attributes,
};

static struct ssw_sysobj_t {
	struct kobject kobj;
} ssw_sysobj;

int ssw_sysfs_init(void)
{
	struct ssw_sysobj_t *obj = &ssw_sysobj;

	memset(&obj->kobj, 0x00, sizeof(obj->kobj));

	obj->kobj.parent = kernel_kobj;
	if (kobject_init_and_add(&obj->kobj, &ssw_ktype, NULL, "mtk_ssw")) {
		kobject_put(&obj->kobj);
		return -ENOMEM;
	}
	kobject_uevent(&obj->kobj, KOBJ_ADD);

	return 0;
}

static ssize_t ssw_attr_show(struct kobject *kobj, struct attribute *attr,
			     char *buffer)
{
	struct ssw_sys_entry *entry =
	    container_of(attr, struct ssw_sys_entry, attr);

	return entry->show(kobj, buffer);
}

static ssize_t ssw_attr_store(struct kobject *kobj, struct attribute *attr,
			      const char *buffer, size_t size)
{
	struct ssw_sys_entry *entry =
	    container_of(attr, struct ssw_sys_entry, attr);

	return entry->store(kobj, buffer, size);
}

static ssize_t ssw_mode_show(struct kobject *kobj, char *buffer)
{
	int remain = PAGE_SIZE;
	int len;
	char *ptr = buffer;

	len = scnprintf(ptr, remain, "0x%x\n", get_current_ssw_mode());
	ptr += len;
	remain -= len;
	SSW_DBG("ssw_mode_show\n");

	return PAGE_SIZE - remain;
}

static ssize_t ssw_mode_store(struct kobject *kobj, const char *buffer,
			      size_t size)
{
	unsigned int mode;
	int res = kstrtoint(buffer, 0, &mode);
	unsigned int type;

	if (res != 1) {
		SSW_DBG("%s: expect 1 numbers\n", __func__);
	} else {
		SSW_DBG("ssw_mode_store %x\n", mode);
		/*Switch sim mode */
		type = (mode & 0xFFFF0000) >> 16;
		mode = mode & 0x0000FFFF;
		if (type == 0) {	/*Internal */
			SSW_DBG("Internal sim switch: %d-->%d\n", sim_mode_curr,
				mode);
			if ((sim_mode_curr != mode)
			    && (SSW_SUCCESS == set_sim_gpio(mode)))
				sim_mode_curr = mode;
		}
	}
	return size;
}

/*---------------------------------------------------------------------------*/

/*************************************************************************/
/*sim switch hardware operation	 */
/*************************************************************************/

static int set_sim_gpio(unsigned int mode)
{
	SSW_DBG("set_sim_gpio: %d\n", mode);
	switch (mode) {
	case SINGLE_TALK_MDSYS:
#if defined(CONFIG_MTK_LEGACY)
#if (defined(GPIO_SIM1_HOT_PLUG) && defined(GPIO_SIM2_HOT_PLUG))
		mt_set_gpio_mode(GPIO_SIM1_HOT_PLUG,
				 GPIO_SIM1_HOT_PLUG_M_MDEINT);
		mt_set_gpio_mode(GPIO_SIM2_HOT_PLUG,
				 GPIO_SIM2_HOT_PLUG_M_MDEINT);
#endif
		/*SIM1=> MD1 SIM1IF */
		mt_set_gpio_mode(GPIO_SIM1_SCLK, GPIO_SIM1_SCLK_M_CLK);
		mt_set_gpio_mode(GPIO_SIM1_SRST, GPIO_SIM1_SRST_M_MD_SIM1_SRST);
		mt_set_gpio_mode(GPIO_SIM1_SIO, GPIO_SIM1_SIO_M_MD_SIM1_SDAT);
		/*SIM2=> MD1 SIM2IF */
		mt_set_gpio_mode(GPIO_SIM2_SCLK, GPIO_SIM2_SCLK_M_CLK);
		mt_set_gpio_mode(GPIO_SIM2_SRST, GPIO_SIM2_SRST_M_MD_SIM2_SRST);
		mt_set_gpio_mode(GPIO_SIM2_SIO, GPIO_SIM2_SIO_M_MD_SIM2_SDAT);
#else

		if (NULL != hot_plug_mode1)
			pinctrl_select_state(ssw_pinctrl, hot_plug_mode1);
		else
			SSW_DBG("hot_plug_mode1 not exist.\n");

		/*SIM1=> MD1 SIM1IF */
		/*SIM2=> MD1 SIM2IF */

		pinctrl_select_state(ssw_pinctrl, two_sims_bound_to_md1);

#endif
		break;
	case SINGLE_TALK_MDSYS_LITE:
#if defined(CONFIG_MTK_LEGACY)
#if (defined(GPIO_SIM1_HOT_PLUG) && defined(GPIO_SIM2_HOT_PLUG))
		mt_set_gpio_mode(GPIO_SIM1_HOT_PLUG,
				 GPIO_SIM1_HOT_PLUG_M_C2K_UIM0_HOT_PLUG_IN);
		mt_set_gpio_mode(GPIO_SIM2_HOT_PLUG,
				 GPIO_SIM2_HOT_PLUG_M_MDEINT);
#endif
		/*SIM1=> MD3 SIM1IF */
		mt_set_gpio_mode(GPIO_SIM1_SCLK, GPIO_SIM1_SCLK_M_UIM0_CLK);
		mt_set_gpio_mode(GPIO_SIM1_SRST, GPIO_SIM1_SRST_M_UIM0_RST);
		mt_set_gpio_mode(GPIO_SIM1_SIO, GPIO_SIM1_SIO_M_UIM0_IO);
		/*SIM2=> MD1 SIM2IF */
		mt_set_gpio_mode(GPIO_SIM2_SCLK, GPIO_SIM2_SCLK_M_CLK);
		mt_set_gpio_mode(GPIO_SIM2_SRST, GPIO_SIM2_SRST_M_MD_SIM2_SRST);
		mt_set_gpio_mode(GPIO_SIM2_SIO, GPIO_SIM2_SIO_M_MD_SIM2_SDAT);
#else
		if (NULL != hot_plug_mode2)
			pinctrl_select_state(ssw_pinctrl, hot_plug_mode2);
		else
			SSW_DBG("hot_plug_mode2 not exist.\n");

		/*SIM1=> MD1 SIM1IF */
		/*SIM2=> MD1 SIM2IF */
		pinctrl_select_state(ssw_pinctrl, sim1_md3_sim2_md1);

#endif
		break;

	default:
		SSW_DBG("[Error] Invalid Mode(%d)", mode);
		return SSW_INVALID_PARA;
	}
#if defined(CONFIG_MTK_LEGACY)
#if (defined(GPIO_SIM1_HOT_PLUG) && defined(GPIO_SIM2_HOT_PLUG))
	SSW_DBG
	    ("mode(%d),SIM1(eint=%d, sclk=%d, srst=%d , sio=%d) SIM2(eint=%d,sclk=%d, srst=%d , sio=%d)\n",
	     mode, mt_get_gpio_mode(GPIO_SIM1_HOT_PLUG),
	     mt_get_gpio_mode(GPIO_SIM1_SCLK), mt_get_gpio_mode(GPIO_SIM1_SRST),
	     mt_get_gpio_mode(GPIO_SIM1_SIO),
	     mt_get_gpio_mode(GPIO_SIM2_HOT_PLUG),
	     mt_get_gpio_mode(GPIO_SIM2_SCLK), mt_get_gpio_mode(GPIO_SIM2_SRST),
	     mt_get_gpio_mode(GPIO_SIM2_SIO));
#else
	SSW_DBG
	    ("mode(%d),SIM1(sclk=%d, srst=%d , sio=%d) SIM2(sclk=%d, srst=%d , sio=%d)\n",
	     mode, mt_get_gpio_mode(GPIO_SIM1_SCLK),
	     mt_get_gpio_mode(GPIO_SIM1_SRST), mt_get_gpio_mode(GPIO_SIM1_SIO),
	     mt_get_gpio_mode(GPIO_SIM2_SCLK), mt_get_gpio_mode(GPIO_SIM2_SRST),
	     mt_get_gpio_mode(GPIO_SIM2_SIO));

#endif
#endif
	return SSW_SUCCESS;
}

int switch_sim_mode(int id, char *buf, unsigned int len)
{
	unsigned int mode = *((unsigned int *)buf);
	unsigned int type = (mode & 0xFFFF0000) >> 16;

	SSW_DBG("switch_sim_mode:mode=0x%x, type=%d\n", mode, type);
	mode = mode & 0x0000FFFF;
	mutex_lock(&sim_switch_mutex);
	if (type == 0) {	/*Internal */
		SSW_DBG("Internal sim switch: %d --> %d\n", sim_mode_curr,
			mode);
		if ((sim_mode_curr != mode)
		    && (SSW_SUCCESS == set_sim_gpio(mode)))
			sim_mode_curr = mode;
	}
	mutex_unlock(&sim_switch_mutex);
	SSW_DBG("sim switch sim_mode_curr(%d)OK\n", sim_mode_curr);

	return 0;

}

/*To decide sim mode according to compile option */
static int get_sim_mode_init(void)
{
	unsigned int sim_mode = 0;
#ifdef CONFIG_MTK_SVLTE_SUPPORT
	sim_mode = SINGLE_TALK_MDSYS;
#else
#ifdef CONFIG_EVDO_DT_SUPPORT
	sim_mode = SINGLE_TALK_MDSYS_LITE;
#else
	sim_mode = SINGLE_TALK_MDSYS;
#endif
#endif
	return sim_mode;
}

/*sim switch hardware initial */
static int sim_switch_init(void)
{
	SSW_DBG("sim_switch_init\n");
#if defined(CONFIG_MTK_LEGACY)
	/*init sim1 */
	mt_set_gpio_dir(GPIO_SIM1_SCLK, 1);
	mt_set_gpio_dir(GPIO_SIM1_SRST, 1);
	mt_set_gpio_pull_enable(GPIO_SIM1_SIO, 1);
	mt_set_gpio_pull_select(GPIO_SIM1_SIO, 1);
	mt_set_gpio_dir(GPIO_SIM1_SIO, 0);
	/*init sim2 */
	mt_set_gpio_dir(GPIO_SIM2_SCLK, 1);
	mt_set_gpio_dir(GPIO_SIM2_SRST, 1);
	mt_set_gpio_pull_enable(GPIO_SIM2_SIO, 1);
	mt_set_gpio_pull_select(GPIO_SIM2_SIO, 1);
	mt_set_gpio_dir(GPIO_SIM2_SIO, 0);
	SSW_DBG("SIM1 sclk= en=%d,dir=%d,in=%d,out=%d\n",
		mt_get_gpio_pull_enable(GPIO_SIM1_SCLK),
		mt_get_gpio_dir(GPIO_SIM1_SCLK), mt_get_gpio_in(GPIO_SIM1_SCLK),
		mt_get_gpio_out(GPIO_SIM1_SCLK));
	SSW_DBG("SIM1 srst= en=%d,dir=%d,in=%d,out=%d\n",
		mt_get_gpio_pull_enable(GPIO_SIM1_SRST),
		mt_get_gpio_dir(GPIO_SIM1_SRST), mt_get_gpio_in(GPIO_SIM1_SRST),
		mt_get_gpio_out(GPIO_SIM1_SRST));
	SSW_DBG("SIM1 sio= en=%d,dir=%d,in=%d,out=%d\n",
		mt_get_gpio_pull_enable(GPIO_SIM1_SIO),
		mt_get_gpio_dir(GPIO_SIM1_SIO), mt_get_gpio_in(GPIO_SIM1_SIO),
		mt_get_gpio_out(GPIO_SIM1_SIO));

	SSW_DBG("SIM2 sclk= en=%d,dir=%d,in=%d,out=%d\n",
		mt_get_gpio_pull_enable(GPIO_SIM2_SCLK),
		mt_get_gpio_dir(GPIO_SIM2_SCLK), mt_get_gpio_in(GPIO_SIM2_SCLK),
		mt_get_gpio_out(GPIO_SIM2_SCLK));
	SSW_DBG("SIM2 srst= en=%d,dir=%d,in=%d,out=%d\n",
		mt_get_gpio_pull_enable(GPIO_SIM2_SRST),
		mt_get_gpio_dir(GPIO_SIM2_SRST), mt_get_gpio_in(GPIO_SIM2_SRST),
		mt_get_gpio_out(GPIO_SIM2_SRST));
	SSW_DBG("SIM2 sio= en=%d,dir=%d,in=%d,out=%d\n",
		mt_get_gpio_pull_enable(GPIO_SIM2_SIO),
		mt_get_gpio_dir(GPIO_SIM2_SIO), mt_get_gpio_in(GPIO_SIM2_SIO),
		mt_get_gpio_out(GPIO_SIM2_SIO));
#endif

	sim_mode_curr = get_sim_mode_init();
	if (SSW_SUCCESS != set_sim_gpio(sim_mode_curr)) {
		SSW_DBG("sim_switch_init fail\n");
		return SSW_INVALID_PARA;
	}
	return 0;
}

static int sim_switch_probe(struct platform_device *dev)
{
#if !defined(CONFIG_MTK_LEGACY)
	ssw_pinctrl = devm_pinctrl_get(&dev->dev);
	if (IS_ERR(ssw_pinctrl)) {
		SSW_DBG("cannot find ssw pinctrl.\n");
		return PTR_ERR(ssw_pinctrl);
	}

	hot_plug_mode1 = pinctrl_lookup_state(ssw_pinctrl, "hot_plug_mode1");
	two_sims_bound_to_md1 =
	    pinctrl_lookup_state(ssw_pinctrl, "two_sims_bound_to_md1");

	hot_plug_mode2 = pinctrl_lookup_state(ssw_pinctrl, "hot_plug_mode2");
	sim1_md3_sim2_md1 =
	    pinctrl_lookup_state(ssw_pinctrl, "sim1_md3_sim2_md1");

#endif				/*!defined(CONFIG_MTK_LEGACY) */
	SSW_DBG("Enter sim_switch_probe\n");

	mutex_init(&sim_switch_mutex);

#if __ENABLE_SSW_SYSFS
	ssw_sysfs_init();
#endif

	sim_switch_init();

	return 0;
}

static int sim_switch_remove(struct platform_device *dev)
{
	/*SSW_DBG("sim_switch_remove\n"); */
	return 0;
}

static void sim_switch_shutdown(struct platform_device *dev)
{
	/*SSW_DBG("sim_switch_shutdown\n"); */
}

static int sim_switch_suspend(struct platform_device *dev, pm_message_t state)
{
	/*SSW_DBG("sim_switch_suspend\n"); */
	return 0;
}

static int sim_switch_resume(struct platform_device *dev)
{
	/*SSW_DBG("sim_switch_resume\n"); */
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id ssw_of_ids[] = {
	{.compatible = "mediatek,sim_switch",},
	{}
};
#endif

static struct platform_driver sim_switch_driver = {

	.driver = {
		   .name = "sim-switch",
		   },
	.probe = sim_switch_probe,
	.remove = sim_switch_remove,
	.shutdown = sim_switch_shutdown,
	.suspend = sim_switch_suspend,
	.resume = sim_switch_resume,
};

static int __init sim_switch_driver_init(void)
{
	int ret = 0;

#ifdef CONFIG_OF
	sim_switch_driver.driver.of_match_table = ssw_of_ids;
#endif

	SSW_DBG("sim_switch_driver_init\n");
	ret = platform_driver_register(&sim_switch_driver);
	if (ret) {
		SSW_DBG("ssw_driver register fail(%d)\n", ret);
		return ret;
	}
	return ret;
}

static void __exit sim_switch_driver_exit(void)
{
}

module_init(sim_switch_driver_init);
module_exit(sim_switch_driver_exit);

MODULE_DESCRIPTION("MTK SIM Switch Driver");
MODULE_AUTHOR("MTK");
MODULE_LICENSE("GPL");
