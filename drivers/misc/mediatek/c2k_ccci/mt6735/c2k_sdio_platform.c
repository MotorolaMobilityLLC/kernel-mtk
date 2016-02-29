
#include <mach/mt_c2k_sdio.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

static pm_callback_t via_sdio_pm_cb;
static void *via_sdio_pm_data;

void c2k_sdio_register_pm(pm_callback_t pm_cb, void *data)
{
	pr_info("c2k_sdio_register_pm (0x%p, 0x%p)\n", pm_cb, data);
	/* register pm change callback */
	via_sdio_pm_cb = pm_cb;
	via_sdio_pm_data = data;
}

#ifndef C2K_USE_EINT
#define C2K_USE_EINT
#endif

#ifdef C2K_USE_EINT

static int c2k_sdio_eirq_num = 262;
static sdio_irq_handler_t *c2k_sdio_eirq_handler;
static void *c2k_sdio_eirq_data;
/*static int interrupt_count_c2k;*/
static atomic_t irq_installed;

void c2k_sdio_enable_eirq(void)
{
	/*pr_info("[C2K] interrupt enable from %ps\n", __builtin_return_address(0));*/
	if (atomic_read(&irq_installed))
		enable_irq(c2k_sdio_eirq_num);
}

void c2k_sdio_disable_eirq(void)
{
	/*pr_info("[C2K] interrupt disable from %ps\n", __builtin_return_address(0));*/
	if (atomic_read(&irq_installed))
		disable_irq_nosync(c2k_sdio_eirq_num);
}

static irqreturn_t c2k_sdio_eirq_handler_stub(int irq, void *data)
{
	/*pr_info("[C2K] interrupt %d\n", interrupt_count_c2k++);*/
	if (atomic_read(&irq_installed) && c2k_sdio_eirq_handler)
		c2k_sdio_eirq_handler(c2k_sdio_eirq_data);

	return IRQ_HANDLED;
}

void c2k_sdio_request_eirq(sdio_irq_handler_t irq_handler, void *data)
{
	pr_info("[C2K] request interrupt %d from %ps\n", c2k_sdio_eirq_num, __builtin_return_address(0));
	/*
	ret = request_irq(c2k_sdio_eirq_num, c2k_sdio_eirq_handler_stub, IRQF_TRIGGER_LOW, "C2K_CCCI", NULL);
	disable_irq(c2k_sdio_eirq_num);
	*/
	c2k_sdio_eirq_handler = irq_handler;
	c2k_sdio_eirq_data    = data;
}

void c2k_sdio_install_eirq(void)
{
	int ret, irq_num;
	struct device_node *node = NULL;

	node = of_find_node_by_name(NULL, "c2k_sdio");
	if (node) {
		/* get IRQ ID */
		irq_num = irq_of_parse_and_map(node, 0);
		c2k_sdio_eirq_num = irq_num;
	} else {
		pr_err("c2k no device node\n");
		return;
	}
	atomic_set(&irq_installed, 1);
	pr_info("[C2K] interrupt install start from %ps, irq num = %d(%d)\n", __builtin_return_address(0),
		irq_num, c2k_sdio_eirq_num);
	ret = request_irq(irq_num, c2k_sdio_eirq_handler_stub, 0, "C2K_CCCI", NULL);

	pr_info("[C2K] interrupt install(%d) from %ps\n", ret, __builtin_return_address(0));
	if (!ret)
		disable_irq(c2k_sdio_eirq_num);
	else
		atomic_set(&irq_installed, 0);
}

void c2k_sdio_uninstall_eirq(void)
{
	pr_info("[C2K] interrupt uninstall from %ps\n", __builtin_return_address(0));
	atomic_set(&irq_installed, 0);
	free_irq(c2k_sdio_eirq_num, NULL);
}
#else
void c2k_sdio_install_eirq(void)
{
	pr_info("[C2K] skip install: not eirq\n");
}
void c2k_sdio_uninstall_eirq(void)
{
	pr_info("[C2K] skip uninstall: not eirq\n");
}
#endif

void via_sdio_on(int sdio_port_num)
{
	pm_message_t state = { .event = PM_EVENT_USER_RESUME };

	pr_info("via_sdio_on (%d)\n", sdio_port_num);

	/* 1. disable sdio eirq */

	/* 2. call sd callback */
	if (via_sdio_pm_cb)
		via_sdio_pm_cb(state, via_sdio_pm_data);
	else
		pr_warn("via_sdio_on no sd callback!!\n");

}
EXPORT_SYMBOL(via_sdio_on);

void via_sdio_off(int sdio_port_num)
{
	pm_message_t state = {.event = PM_EVENT_USER_SUSPEND};

	pr_info("via_sdio_off (%d)\n", sdio_port_num);

	/* 0. disable sdio eirq */
#ifdef C2K_USE_EINT
	c2k_sdio_disable_eirq();
#endif

	/* 1. call sd callback */
	if (via_sdio_pm_cb)
		via_sdio_pm_cb(state, via_sdio_pm_data);
	else
		pr_warn("via_sdio_off no sd callback!!\n");
}
EXPORT_SYMBOL(via_sdio_off);
