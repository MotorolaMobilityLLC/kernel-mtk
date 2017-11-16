/******************************************************************************
 * mt_gpio_base.c - MTKLinux GPIO Device Driver
 *
 * Copyright 2008-2009 MediaTek Co.,Ltd.
 *
 * DESCRIPTION:
 *     This file provid the other drivers GPIO relative functions
 *
 ******************************************************************************/


#include "6580_gpio.h"
#include <linux/types.h>
#include "mt-plat/sync_write.h"
#include <mt-plat/mt_gpio.h>
#include <mt-plat/mt_gpio_core.h>
#include "mt_gpio_base.h"
/* autogen */
#include "gpio_cfg.h"
#ifdef CONFIG_OF
#include <linux/of_address.h>
#endif
#ifdef CONFIG_MD32_SUPPORT
#include <linux/workqueue.h>
#include <mach/md32_ipi.h>
#endif

spinlock_t mtk_gpio_lock;

#ifdef CONFIG_MD32_SUPPORT
struct GPIO_IPI_Packet {
	u32 touch_pin;
};
static struct work_struct work_md32_cust_pin;
static struct workqueue_struct *wq_md32_cust_pin;
#endif

/* unsigned long GPIO_COUNT=0; */
/* unsigned long uart_base; */

struct mt_gpio_vbase gpio_vbase;
static GPIO_REGS *gpio_reg;
/*---------------------------------------------------------------------------*/
int mt_set_gpio_dir_base(unsigned long pin, unsigned long dir)
{
	unsigned long pos;
	unsigned long bit;
	GPIO_REGS *reg = gpio_reg;

	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	if (dir == GPIO_DIR_IN)
		GPIO_SET_BITS((1L << bit), &reg->dir[pos].rst);
	else
		GPIO_SET_BITS((1L << bit), &reg->dir[pos].set);
	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_dir_base(unsigned long pin)
{
	unsigned long pos;
	unsigned long bit;
	unsigned long data;
	GPIO_REGS *reg = gpio_reg;

	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	data = GPIO_RD32(&reg->dir[pos].val);
	return (((data & (1L << bit)) != 0) ? 1 : 0);
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_pull_enable_base(unsigned long pin, unsigned long enable)
{

	if (PULLEN_offset[pin].offset == -1)
		return GPIO_PULL_EN_UNSUPPORTED;

	if (enable == GPIO_PULL_DISABLE)
		GPIO_SET_BITS((1L << (PULLEN_offset[pin].offset)), PULLEN_addr[pin].addr + 8);
	else
		GPIO_SET_BITS((1L << (PULLEN_offset[pin].offset)), PULLEN_addr[pin].addr + 4);

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_pull_enable_base(unsigned long pin)
{
	unsigned long data;

	if (PULLEN_offset[pin].offset == -1)
		return GPIO_PULL_EN_UNSUPPORTED;

	data = GPIO_RD32(PULLEN_addr[pin].addr);
	return (((data & (1L << (PULLEN_offset[pin].offset))) != 0) ? 1 : 0);
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_smt_base(unsigned long pin, unsigned long enable)
{
	if (SMT_offset[pin].offset == -1)
		return GPIO_SMT_UNSUPPORTED;

	if (enable == GPIO_SMT_DISABLE)
		GPIO_SET_BITS((1L << (SMT_offset[pin].offset)), SMT_addr[pin].addr + 8);
	else
		GPIO_SET_BITS((1L << (SMT_offset[pin].offset)), SMT_addr[pin].addr + 4);

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_smt_base(unsigned long pin)
{
	unsigned long data;

	if (SMT_offset[pin].offset == -1)
		return GPIO_SMT_UNSUPPORTED;

	data = GPIO_RD32(SMT_addr[pin].addr);
	return (((data & (1L << (SMT_offset[pin].offset))) != 0) ? 1 : 0);
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_ies_base(unsigned long pin, unsigned long enable)
{
	if (IES_offset[pin].offset == -1)
		return GPIO_IES_UNSUPPORTED;

	if (enable == GPIO_IES_DISABLE)
		GPIO_SET_BITS((1L << (IES_offset[pin].offset)), IES_addr[pin].addr + 8);
	else
		GPIO_SET_BITS((1L << (IES_offset[pin].offset)), IES_addr[pin].addr + 4);

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_ies_base(unsigned long pin)
{
	unsigned long data;

	if (IES_offset[pin].offset == -1)
		return GPIO_IES_UNSUPPORTED;

	data = GPIO_RD32(IES_addr[pin].addr);
	return (((data & (1L << (IES_offset[pin].offset))) != 0) ? 1 : 0);

}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_pull_select_base(unsigned long pin, unsigned long select)
{
	unsigned long flags;

	if ((PULL_offset[pin].offset == -1) && (PU_offset[pin].offset == -1)) {
		return GPIO_PULL_UNSUPPORTED;
	} else if (PULL_offset[pin].offset == -1) {
		spin_lock_irqsave(&mtk_gpio_lock, flags);
		if (select == GPIO_PULL_DOWN) {
			GPIO_CLR_BITS((1L << (PU_offset[pin].offset)), PU_addr[pin].addr);
			GPIO_SW_SET_BITS((1L << (PD_offset[pin].offset)), PD_addr[pin].addr);
		} else {
			GPIO_CLR_BITS((1L << (PD_offset[pin].offset)), PD_addr[pin].addr);
			GPIO_SW_SET_BITS((1L << (PU_offset[pin].offset)), PU_addr[pin].addr);
		}
		spin_unlock_irqrestore(&mtk_gpio_lock, flags);
	} else {
		if (((pin >= 41) && (pin < 58)) || (pin == 64) || (pin == 65) || (pin == 67)
		    || (pin == 69)) {
			if (select == GPIO_PULL_DOWN)
				GPIO_SET_BITS((1L << (PULL_offset[pin].offset)),
					      PULL_addr[pin].addr + 4);
			else
				GPIO_SET_BITS((1L << (PULL_offset[pin].offset)),
					      PULL_addr[pin].addr + 8);
		} else {
			if (select == GPIO_PULL_DOWN)
				GPIO_SET_BITS((1L << (PULL_offset[pin].offset)),
					      PULL_addr[pin].addr + 8);
			else
				GPIO_SET_BITS((1L << (PULL_offset[pin].offset)),
					      PULL_addr[pin].addr + 4);
		}
	}

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_pull_select_base(unsigned long pin)
{
	unsigned long data;

	if ((PULL_offset[pin].offset == -1) && (PU_offset[pin].offset == -1))
		return GPIO_PULL_UNSUPPORTED;

	if (PULL_offset[pin].offset == -1) {
		data = GPIO_RD32(PU_addr[pin].addr);

		return (((data & (1L << (PU_offset[pin].offset))) != 0) ? 1 : 0);
	}

	if (((pin >= 41) && (pin < 58)) || (pin == 64) || (pin == 65) || (pin == 67)
	    || (pin == 69)) {
		data = GPIO_RD32(PULL_addr[pin].addr);

		return (((data & (1L << (PULL_offset[pin].offset))) != 0) ? 0 : 1);
	}

	data = GPIO_RD32(PULL_addr[pin].addr);
	return (((data & (1L << (PULL_offset[pin].offset))) != 0) ? 1 : 0);
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_inversion_base(unsigned long pin, unsigned long enable)
{				/*FIX-ME
				 */
	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_inversion_base(unsigned long pin)
{				/*FIX-ME */
	return 0;		/* FIX-ME */
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_out_base(unsigned long pin, unsigned long output)
{
	unsigned long pos;
	unsigned long bit;
	GPIO_REGS *reg = gpio_reg;

	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	if (output == GPIO_OUT_ZERO)
		GPIO_SET_BITS((1L << bit), &reg->dout[pos].rst);
	else
		GPIO_SET_BITS((1L << bit), &reg->dout[pos].set);
	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_out_base(unsigned long pin)
{
	unsigned long pos;
	unsigned long bit;
	unsigned long data;
	GPIO_REGS *reg = gpio_reg;

	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	data = GPIO_RD32(&reg->dout[pos].val);
	return (((data & (1L << bit)) != 0) ? 1 : 0);
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_in_base(unsigned long pin)
{
	unsigned long pos;
	unsigned long bit;
	unsigned long data;
	GPIO_REGS *reg = gpio_reg;

	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	data = GPIO_RD32(&reg->din[pos].val);
	return (((data & (1L << bit)) != 0) ? 1 : 0);
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_mode_base(unsigned long pin, unsigned long mode)
{
	unsigned long pos;
	unsigned long bit;
	unsigned long data;
	/* unsigned long mask = (1L << GPIO_MODE_BITS) - 1; */
	GPIO_REGS *reg = gpio_reg;


	pos = pin / MAX_GPIO_MODE_PER_REG;
	bit = pin % MAX_GPIO_MODE_PER_REG;

#if 0
	data = GPIO_RD32(&reg->mode[pos].val);

	data &= ~(mask << (GPIO_MODE_BITS * bit));
	data |= (mode << (GPIO_MODE_BITS * bit));

	GPIO_WR32(&reg->mode[pos].val, data);
#endif

	data = ((1L << (GPIO_MODE_BITS * bit)) << 3) | (mode << (GPIO_MODE_BITS * bit));

	GPIO_WR32(&reg->mode[pos]._align1, data);

	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_mode_base(unsigned long pin)
{
	unsigned long pos;
	unsigned long bit;
	unsigned long data;
	unsigned long mask = (1L << GPIO_MODE_BITS) - 1;
	GPIO_REGS *reg = gpio_reg;


	pos = pin / MAX_GPIO_MODE_PER_REG;
	bit = pin % MAX_GPIO_MODE_PER_REG;

	data = GPIO_RD32(&reg->mode[pos].val);

	return ((data >> (GPIO_MODE_BITS * bit)) & mask);

}
/*---------------------------------------------------------------------------*/
int mt_set_gpio_slew_rate_base(unsigned long pin, unsigned long enable)
{
	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
int mt_get_gpio_slew_rate_base(unsigned long pin)
{
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
int mt_set_gpio_pull_resistor_base(unsigned long pin, unsigned long resistors)
{
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_pull_resistor_base(unsigned long pin)
{
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
int mt_set_gpio_driving_base(unsigned long pin, unsigned long strength)
{
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_driving_base(unsigned long pin)
{
	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
void fill_addr_reg_array(PIN_addr *addr, PIN_addr_t *addr_t)
{
	unsigned long i;

	for (i = 0; i < MT_GPIO_BASE_MAX; i++) {
		if (strcmp(addr_t->s1, "GPIO_BASE") == 0)
			addr->addr = GPIO_BASE_1 + addr_t->offset;
		else if (strcmp(addr_t->s1, "IO_CFG_L_BASE") == 0)
			addr->addr = IO_CFG_L_BASE_1 + addr_t->offset;
		else if (strcmp(addr_t->s1, "IO_CFG_B_BASE") == 0)
			addr->addr = IO_CFG_B_BASE_1 + addr_t->offset;
		else if (strcmp(addr_t->s1, "IO_CFG_R_BASE") == 0)
			addr->addr = IO_CFG_R_BASE_1 + addr_t->offset;
		else if (strcmp(addr_t->s1, "IO_CFG_T_BASE") == 0)
			addr->addr = IO_CFG_T_BASE_1 + addr_t->offset;
/*	else if (strcmp(addr_t->s1,"MIPI_TX0_BASE")==0)
		addr->addr = MIPI_TX0_BASE_1 + addr_t->offset;
	else if (strcmp(addr_t->s1,"MIPI_RX_ANA_CSI0_BASE")==0)
		addr->addr = MIPI_RX_ANA_CSI0_BASE_1 + addr_t->offset;
	else if (strcmp(addr_t->s1,"MIPI_RX_ANA_CSI1_BASE")==0)
		addr->addr = MIPI_RX_ANA_CSI1_BASE_1 + addr_t->offset;
*/
		else
			addr->addr = 0;

		/* GPIOLOG("GPIO_COUNT=%d ,addr->addr=0x%lx, i=%d\n", GPIO_COUNT, addr->addr, i); */

		addr += 1;
		addr_t += 1;
	}
	/* GPIO_COUNT++; */
}

/*---------------------------------------------------------------------------*/
void get_gpio_vbase(struct device_node *node)
{
	/* Setup IO addresses */
	gpio_vbase.gpio_regs = of_iomap(node, 0);
	GPIOLOG("gpio_vbase.gpio_regs=0x%p\n", gpio_vbase.gpio_regs);

	gpio_reg = (GPIO_REGS *) (GPIO_BASE_1);

	/* printk("[DTS] gpio_regs=0x%lx\n", gpio_vbase.gpio_regs); */
	spin_lock_init(&mtk_gpio_lock);
}

/*---------------------------------------------------------------------------*/
void get_io_cfg_vbase(void)
{
	struct device_node *node = NULL;
	/* unsigned long i; */

	node = of_find_compatible_node(NULL, NULL, "mediatek,IOCFG_L");
	if (node) {
		/* Setup IO addresses */
		gpio_vbase.IOCFG_L_regs = of_iomap(node, 0);
		/* GPIOLOG("gpio_vbase.IOCFG_L_regs=0x%lx\n", gpio_vbase.IOCFG_L_regs); */
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,IOCFG_B");
	if (node) {
		/* Setup IO addresses */
		gpio_vbase.IOCFG_B_regs = of_iomap(node, 0);
		/* GPIOLOG("gpio_vbase.IOCFG_B_regs=0x%lx\n", gpio_vbase.IOCFG_B_regs); */
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,IOCFG_R");
	if (node) {
		/* Setup IO addresses */
		gpio_vbase.IOCFG_R_regs = of_iomap(node, 0);
		/* GPIOLOG("gpio_vbase.IOCFG_R_regs=0x%lx\n", gpio_vbase.IOCFG_R_regs); */
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,IOCFG_T");
	if (node) {
		/* Setup IO addresses */
		gpio_vbase.IOCFG_T_regs = of_iomap(node, 0);
		/* GPIOLOG("gpio_vbase.IOCFG_T_regs=0x%lx\n", gpio_vbase.IOCFG_T_regs); */
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,MIPI_TX0");
	if (node) {
		/* Setup IO addresses */
		gpio_vbase.MIPI_TX0_regs = of_iomap(node, 0);
		/* GPIOLOG("gpio_vbase.MIPI_TX0_regs=0x%lx\n", gpio_vbase.MIPI_TX0_regs); */
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,MIPI_RX_ANA_CSI0");
	if (node) {
		/* Setup IO addresses */
		gpio_vbase.MIPI_RX_CSI0_regs = of_iomap(node, 0);
		/* GPIOLOG("gpio_vbase.MIPI_RX_CSI0_regs=0x%lx\n", gpio_vbase.MIPI_RX_CSI0_regs); */
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,MIPI_RX_ANA_CSI1");
	if (node) {
		/* Setup IO addresses */
		gpio_vbase.MIPI_RX_CSI1_regs = of_iomap(node, 0);
		/* GPIOLOG("gpio_vbase.MIPI_RX_CSI1_regs=0x%lx\n", gpio_vbase.MIPI_RX_CSI1_regs); */
	}


	fill_addr_reg_array(IES_addr, IES_addr_t);
	fill_addr_reg_array(SMT_addr, SMT_addr_t);
	fill_addr_reg_array(PULLEN_addr, PULLEN_addr_t);
	fill_addr_reg_array(PULL_addr, PULL_addr_t);
	fill_addr_reg_array(PU_addr, PU_addr_t);
	fill_addr_reg_array(PD_addr, PD_addr_t);
#if 0
	for (i = 0; i < MT_GPIO_BASE_MAX; i++) {
		if (strcmp(IES_addr_t[i].s1, "GPIO_BASE") == 0)
			IES_addr[i].addr = GPIO_BASE_1 + IES_addr_t[i].offset;
		else if (strcmp(IES_addr_t[i].s1, "IOCFG_L_BASE") == 0)
			IES_addr[i].addr = IOCFG_L_BASE_1 + IES_addr_t[i].offset;
		else if (strcmp(IES_addr_t[i].s1, "IOCFG_B_BASE") == 0)
			IES_addr[i].addr = IOCFG_B_BASE_1 + IES_addr_t[i].offset;
		else if (strcmp(IES_addr_t[i].s1, "IOCFG_R_BASE") == 0)
			IES_addr[i].addr = IOCFG_R_BASE_1 + IES_addr_t[i].offset;
		else if (strcmp(IES_addr_t[i].s1, "IOCFG_T_BASE") == 0)
			IES_addr[i].addr = IOCFG_T_BASE_1 + IES_addr_t[i].offset;
		else if (strcmp(IES_addr_t[i].s1, "MIPI_TX0_BASE") == 0)
			IES_addr[i].addr = MIPI_TX0_BASE_1 + IES_addr_t[i].offset;
		else if (strcmp(IES_addr_t[i].s1, "MIPI_RX_ANA_CSI0_BASE") == 0)
			IES_addr[i].addr = MIPI_RX_ANA_CSI0_BASE_1 + IES_addr_t[i].offset;
		else if (strcmp(IES_addr_t[i].s1, "MIPI_RX_ANA_CSI1_BASE") == 0)
			IES_addr[i].addr = MIPI_RX_ANA_CSI1_BASE_1 + IES_addr_t[i].offset;
		else
			IES_addr[i].addr = 0;
	}
#endif
}

/*---------------------------------------------------------------------------*/
#ifdef CONFIG_MD32_SUPPORT
/*---------------------------------------------------------------------------*/
void md32_send_cust_pin_from_irq(void)
{
	ipi_status ret = BUSY;
	struct GPIO_IPI_Packet ipi_pkt;

	GPIOLOG("Enter ipi wq function to send cust pin...\n");
	ipi_pkt.touch_pin = (GPIO_CTP_EINT_PIN & ~(0x80000000));

	ret = md32_ipi_send(IPI_GPIO, &ipi_pkt, sizeof(ipi_pkt), true);
	if (ret != DONE)
		GPIOLOG("MD32 side IPI busy... %d\n", ret);

	/* GPIOLOG("IPI get touch pin num = %d ...\n", GPIO_RD32(uart_base+0x40)); */
}

/*---------------------------------------------------------------------------*/
void gpio_ipi_handler(int id, void *data, uint len)
{
	GPIOLOG("Enter gpio_ipi_handler...\n");
	queue_work(wq_md32_cust_pin, &work_md32_cust_pin);
}

/*---------------------------------------------------------------------------*/
void md32_gpio_handle_init(void)
{
	/* struct device_node *node = NULL; */
	md32_ipi_registration(IPI_GPIO, gpio_ipi_handler, "GPIO IPI");
	wq_md32_cust_pin = create_workqueue("MD32_CUST_PIN_WQ");
	INIT_WORK(&work_md32_cust_pin, (void (*)(void))md32_send_cust_pin_from_irq);

	/* node = of_find_compatible_node(NULL, NULL, "mediatek,AP_UART1"); */
	/* if(node){ */
	/*  iomap registers */
	    /* uart_base = (unsigned long)of_iomap(node, 0); */
	    /* } */
}

/*---------------------------------------------------------------------------*/
#endif				/*CONFIG_MD32_SUPPORT */
/*---------------------------------------------------------------------------*/
#ifdef CONFIG_PM
/*---------------------------------------------------------------------------*/
void mt_gpio_suspend(void)
{
	u32 val;
	u32 mask;

	/* set TDSEL for SRCLKENA0, WATCHDOG */

	mask = 0xff0;
	val = GPIO_RD32(IO_CFG_L_BASE_1 + 0x60);
	val &= ~(mask);
	val |= (0xff0 & mask);
	GPIO_WR32(IO_CFG_L_BASE_1 + 0x60, val);
}

/*---------------------------------------------------------------------------*/
void mt_gpio_resume(void)
{
	u32 val;
	u32 mask;

	/* set TDSEL for SRCLKENA0, WATCHDOG */

	mask = 0xff0;
	val = GPIO_RD32(IO_CFG_L_BASE_1 + 0x60);
	val &= ~(mask);
	val |= (0x000 & mask);
	GPIO_WR32(IO_CFG_L_BASE_1 + 0x60, val);
}

/*---------------------------------------------------------------------------*/
#endif				/*CONFIG_PM */
/*---------------------------------------------------------------------------*/
