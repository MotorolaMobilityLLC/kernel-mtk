
#include <mt-plat/mt_gpio.h>
#include <mt-plat/mt_gpio_core.h>
#include <mach/mt_gpio_fpga.h>

int mt_set_gpio_dir_base(unsigned long pin, unsigned long dir)
{
	int ret = RSUCCESS;
	return ret;
}
int mt_get_gpio_dir_base(unsigned long pin)
{
	return GPIO_DIR_UNSUPPORTED;
}
int mt_set_gpio_pull_enable_base(unsigned long pin, unsigned long enable)    {return RSUCCESS; }
int mt_get_gpio_pull_enable_base(unsigned long pin)                {return GPIO_PULL_EN_UNSUPPORTED; }
int mt_set_gpio_pull_select_base(unsigned long pin, unsigned long select)    {return RSUCCESS; }
int mt_get_gpio_pull_select_base(unsigned long pin)                {return GPIO_PULL_UNSUPPORTED; }
int mt_set_gpio_inversion_base(unsigned long pin, unsigned long enable)      {return RSUCCESS; }
int mt_get_gpio_inversion_base(unsigned long pin)                  {return GPIO_DATA_INV_UNSUPPORTED; }
int mt_set_gpio_smt_base(unsigned long pin, unsigned long enable)	 {return RSUCCESS; }
int mt_get_gpio_smt_base(unsigned long pin)                  {return GPIO_SMT_UNSUPPORTED; }
int mt_set_gpio_ies_base(unsigned long pin, unsigned long enable)	 {return RSUCCESS; }
int mt_get_gpio_ies_base(unsigned long pin)                  {return GPIO_IES_UNSUPPORTED; }

int mt_set_gpio_out_base(unsigned long pin, unsigned long output)
{
	int ret = RSUCCESS;
	return ret;
}
int mt_get_gpio_out_base(unsigned long pin)
{
	return GPIO_OUT_UNSUPPORTED;
}
int mt_get_gpio_in_base(unsigned long pin)
{
	return GPIO_IN_UNSUPPORTED;
}
int mt_set_gpio_mode_base(unsigned long pin, unsigned long mode)
{
	return RSUCCESS;
}
int mt_get_gpio_mode_base(unsigned long pin)
{
	return GPIO_MODE_UNSUPPORTED;
}
int mt_set_clock_output_base(unsigned long num, unsigned long src, unsigned long div)
{
	return RSUCCESS;
}
int mt_get_clock_output_base(unsigned long num, unsigned long *src, unsigned long *div)
{
	return -1;
}

/*---------------------------------------------------------------------------*/
void get_gpio_vbase(struct device_node *node)
{
	/* compatible with HAL */
}
/*---------------------------------------------------------------------------*/
void get_io_cfg_vbase(void)
{
	/* compatible with HAL */
}
/*---------------------------------------------------------------------------*/
#ifdef CONFIG_MD32_SUPPORT
/*---------------------------------------------------------------------------*/
void md32_gpio_handle_init(void)
{
	/* compatible with HAL */
}
/*---------------------------------------------------------------------------*/
#endif /*CONFIG_MD32_SUPPORT*/
/*---------------------------------------------------------------------------*/
#ifdef CONFIG_PM
/*---------------------------------------------------------------------------*/
void mt_gpio_suspend(void)
{
	/* compatible with HAL */
}
/*---------------------------------------------------------------------------*/
void mt_gpio_resume(void)
{
	/* compatible with HAL */
}
/*---------------------------------------------------------------------------*/
#endif /*CONFIG_PM*/

