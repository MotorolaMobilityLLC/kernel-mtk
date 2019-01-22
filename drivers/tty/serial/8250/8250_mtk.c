/*
 * Mediatek 8250 driver.
 *
 * Copyright (c) 2014 MundoReader S.L.
 * Author: Matthias Brugger <matthias.bgg@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/clk.h>
#include <linux/console.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/serial_8250.h>
#include <linux/serial_reg.h>
#include <linux/delay.h>

#include "8250.h"

#define UART_MTK_DLH		0x01	/* baudrate Only when LCR.DLAB = 1 */
#define UART_MTK_HIGHS		0x09	/* Highspeed register */
#define UART_MTK_SAMPLE_COUNT	0x0a	/* Sample count register */
#define UART_MTK_SAMPLE_POINT	0x0b	/* Sample point register */
#define MTK_UART_RATE_FIX	0x0d	/* UART Rate Fix Register */
#define UART_MTK_GUARD		0x0f	/* guard time added register */
#define UART_MTK_ESCAPE_EN	0x11	/* escape enable register */
#define UART_MTK_FRACDIV_L	0x15	/* fractional divider LSB address */
#define UART_MTK_FRACDIV_M	0x16	/* fractional divider MSB address */
#define UART_MTK_FCR_RD		0x17	/* fifo control register */
#define UART_MTK_RX_SEL		0x24	/* uart rx pin sel */
#define UART_MTK_SLEEP_REQ	0x2d	/* uart sleep request register */
#define UART_MTK_SLEEP_ACK	0x2e	/* uart sleep ack register */

#define UART_MTK_CLK_OFF_REQ	(1 << 0)	/* Request UART to sleep*/
#define UART_MTK_CLK_OFF_ACK	(1 << 0)	/* UART sleep ack*/
#define WAIT_MTK_UART_ACK_TIMES	10

#define UART_MTK_NR	CONFIG_SERIAL_8250_NR_UARTS

struct mtk_uart_register {
	unsigned int dll;
	unsigned int dlh;
	unsigned int ier;
	unsigned int lcr;
	unsigned int mcr;
	unsigned int fcr;
	unsigned int lsr;
	unsigned int efr;
	unsigned int highspeed;
	unsigned int sample_count;
	unsigned int sample_point;
	unsigned int fracdiv_l;
	unsigned int fracdiv_m;
	unsigned int escape_en;
	unsigned int guard;
	unsigned int rx_sel;
};

struct mtk8250_data {
	int			line;
	int			clk_count;
	spinlock_t lock;
	struct clk		*uart_clk;
	struct clk		*bus_clk;
	struct mtk_uart_register reg;
};

static void
mtk8250_set_termios(struct uart_port *port, struct ktermios *termios,
			struct ktermios *old)
{
	unsigned long flags;
	unsigned int baud, quot;

	struct uart_8250_port *up =
		container_of(port, struct uart_8250_port, port);

	serial8250_do_set_termios(port, termios, old);

	/*
	 * Mediatek UARTs use an extra highspeed register (UART_MTK_HIGHS)
	 *
	 * We need to recalcualte the quot register, as the claculation depends
	 * on the vaule in the highspeed register.
	 *
	 * Some baudrates are not supported by the chip, so we use the next
	 * lower rate supported and update termios c_flag.
	 *
	 * If highspeed register is set to 3, we need to specify sample count
	 * and sample point to increase accuracy. If not, we reset the
	 * registers to their default values.
	 */
	baud = uart_get_baud_rate(port, termios, old,
				  port->uartclk / 16 / 0xffff,
				  port->uartclk);

	if (baud <= 115200) {
		serial_port_out(port, UART_MTK_HIGHS, 0x0);
		quot = uart_get_divisor(port, baud);
	} else if (baud <= 576000) {
		serial_port_out(port, UART_MTK_HIGHS, 0x2);

		/* Set to next lower baudrate supported */
		if ((baud == 500000) || (baud == 576000))
			baud = 460800;
		quot = DIV_ROUND_UP(port->uartclk, 4 * baud);
	} else {
		serial_port_out(port, UART_MTK_HIGHS, 0x3);
		quot = DIV_ROUND_UP(port->uartclk, 256 * baud);
	}

	/*
	 * Ok, we're now changing the port state.  Do it with
	 * interrupts disabled.
	 */
	spin_lock_irqsave(&port->lock, flags);

	/* set DLAB we have cval saved in up->lcr from the call to the core */
	serial_port_out(port, UART_LCR, up->lcr | UART_LCR_DLAB);
	serial_dl_write(up, quot);

	/* reset DLAB */
	serial_port_out(port, UART_LCR, up->lcr);

	if (baud > 460800) {
		unsigned int tmp;

		tmp = DIV_ROUND_CLOSEST(port->uartclk, quot * baud);
		serial_port_out(port, UART_MTK_SAMPLE_COUNT, tmp - 1);
		serial_port_out(port, UART_MTK_SAMPLE_POINT,
					(tmp - 2) >> 1);
	} else {
		serial_port_out(port, UART_MTK_SAMPLE_COUNT, 0x00);
		serial_port_out(port, UART_MTK_SAMPLE_POINT, 0xff);
	}

	spin_unlock_irqrestore(&port->lock, flags);
	/* Don't rewrite B0 */
	if (tty_termios_baud_rate(termios))
		tty_termios_encode_baud_rate(termios, baud, baud);
}

static int mtk8250_runtime_suspend(struct device *dev)
{
	struct mtk8250_data *data = dev_get_drvdata(dev);

	spin_lock(&data->lock);
	if (data->clk_count == 0) {
		pr_debug("%s(%d)\n", __func__, data->clk_count);
	} else {
		clk_disable_unprepare(data->bus_clk);
		data->clk_count--;
	}
	spin_unlock(&data->lock);

	return 0;
}

static int mtk8250_runtime_resume(struct device *dev)
{
	struct mtk8250_data *data = dev_get_drvdata(dev);

	spin_lock(&data->lock);
	if (data->clk_count > 0) {
		pr_debug("%s(%d)\n", __func__, data->clk_count);
	} else {
		clk_prepare_enable(data->bus_clk);
		data->clk_count++;
	}
	spin_unlock(&data->lock);

	return 0;
}

static void
mtk8250_do_pm(struct uart_port *port, unsigned int state, unsigned int old)
{
	if (state == UART_PM_STATE_ON)
		if (pm_runtime_get_sync(port->dev))
			mtk8250_runtime_resume(port->dev);

	serial8250_do_pm(port, state, old);

	if (state == UART_PM_STATE_OFF)
		if (pm_runtime_put_sync_suspend(port->dev))
			mtk8250_runtime_suspend(port->dev);
}

static int mtk8250_probe_of(struct platform_device *pdev, struct uart_port *p,
			   struct mtk8250_data *data)
{
	data->uart_clk = devm_clk_get(&pdev->dev, "baud");
	if (IS_ERR(data->uart_clk)) {
		/*
		 * For compatibility with older device trees try unnamed
		 * clk when no baud clk can be found.
		 */
		data->uart_clk = devm_clk_get(&pdev->dev, NULL);
		if (IS_ERR(data->uart_clk)) {
			dev_warn(&pdev->dev, "Can't get uart clock\n");
			return PTR_ERR(data->uart_clk);
		}

		return 0;
	}

	data->bus_clk = devm_clk_get(&pdev->dev, "bus");
	if (IS_ERR(data->bus_clk))
		return PTR_ERR(data->bus_clk);

	return 0;
}

static int mtk8250_probe(struct platform_device *pdev)
{
	struct uart_8250_port uart = {};
	struct resource *regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	struct resource *irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	struct mtk8250_data *data;
	int err;

	if (!regs || !irq) {
		dev_err(&pdev->dev, "no registers/irq defined\n");
		return -EINVAL;
	}

	uart.port.membase = devm_ioremap(&pdev->dev, regs->start,
					 resource_size(regs));
	if (!uart.port.membase)
		return -ENOMEM;

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->clk_count = 0;

	if (pdev->dev.of_node) {
		err = mtk8250_probe_of(pdev, &uart.port, data);
		if (err)
			return err;
	} else
		return -ENODEV;

	spin_lock_init(&uart.port.lock);
	spin_lock_init(&data->lock);
	uart.port.mapbase = regs->start;
	uart.port.irq = irq->start;
	uart.port.pm = mtk8250_do_pm;
	uart.port.type = PORT_16550;
	uart.port.flags = UPF_BOOT_AUTOCONF | UPF_FIXED_PORT;
	uart.port.dev = &pdev->dev;
	uart.port.iotype = UPIO_MEM32;
	uart.port.regshift = 2;
	uart.port.private_data = data;
	uart.port.set_termios = mtk8250_set_termios;
	uart.port.uartclk = clk_get_rate(data->uart_clk);

	/* Disable Rate Fix function */
	writel(0x0, uart.port.membase +
			(MTK_UART_RATE_FIX << uart.port.regshift));

	dev_set_drvdata(&pdev->dev, data);
	platform_set_drvdata(pdev, data);

	pm_runtime_enable(&pdev->dev);
	if (!pm_runtime_enabled(&pdev->dev)) {
		err = mtk8250_runtime_resume(&pdev->dev);
		if (err)
			return err;
	}

	data->line = serial8250_register_8250_port(&uart);
	if (data->line < 0)
		return data->line;

	return 0;
}

static int mtk8250_remove(struct platform_device *pdev)
{
	struct mtk8250_data *data = platform_get_drvdata(pdev);

	pm_runtime_get_sync(&pdev->dev);

	serial8250_unregister_port(data->line);

	pm_runtime_disable(&pdev->dev);
	pm_runtime_put_noidle(&pdev->dev);

	if (!pm_runtime_status_suspended(&pdev->dev))
		mtk8250_runtime_suspend(&pdev->dev);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
int request_uart_to_sleep(void)
{
	u32 val1;
	int i = 0;
	int uart_idx = 0;
	struct uart_8250_port *uart;
	struct mtk8250_data *data;

	for (uart_idx = 0; uart_idx < UART_MTK_NR; uart_idx++) {
		uart = serial8250_get_port(uart_idx);

		if (uart->port.type != PORT_16550 || !uart->port.dev)
			continue;

		data = dev_get_drvdata(uart->port.dev);
		if (data->clk_count > 0) {
			/* request UART to sleep */
			val1 = serial_port_in(&uart->port, UART_MTK_SLEEP_REQ);
			serial_port_out(&uart->port, UART_MTK_SLEEP_REQ, val1 | UART_MTK_CLK_OFF_REQ);

			/* wait for UART to ACK */
			while (!(serial_port_in(&uart->port, UART_MTK_SLEEP_ACK) & UART_MTK_CLK_OFF_ACK)) {
				if (i++ >= WAIT_MTK_UART_ACK_TIMES) {
					serial_port_out(&uart->port, UART_MTK_SLEEP_REQ, val1);
					pr_err("CANNOT GET UART[%d] SLEEP ACK\n", uart_idx);
					return -EBUSY;
				}
				udelay(10);
			}
		} else {
			/*pr_err("[UART%d] clock is off[%d]\n", uart->port.line, data->clk_count);*/
		}
	}

	return 0;
}
EXPORT_SYMBOL(request_uart_to_sleep);

int request_uart_to_wakeup(void)
{
	u32 val1;
	int i = 0;
	int uart_idx = 0;
	struct uart_8250_port *uart;
	struct mtk8250_data *data;

	for (uart_idx = 0; uart_idx < UART_MTK_NR; uart_idx++) {
		uart = serial8250_get_port(uart_idx);

		if (uart->port.type != PORT_16550 || !uart->port.dev)
			continue;

		data = dev_get_drvdata(uart->port.dev);
		if (data->clk_count > 0) {
			/* wakeup uart */
			val1 = serial_port_in(&uart->port, UART_MTK_SLEEP_REQ);
			serial_port_out(&uart->port, UART_MTK_SLEEP_REQ, val1 & (~UART_MTK_CLK_OFF_REQ));

			/* wait for UART to ACK */
			while ((serial_port_in(&uart->port, UART_MTK_SLEEP_ACK) & UART_MTK_CLK_OFF_ACK)) {
				if (i++ >= WAIT_MTK_UART_ACK_TIMES) {
					serial_port_out(&uart->port, UART_MTK_SLEEP_REQ, val1);
					pr_err("CANNOT GET UART[%d] WAKE ACK\n", uart_idx);
					return -EBUSY;
				}
				udelay(10);
			}
		} else {
			/*pr_err("[UART%d] clock is wakeup[%d]\n", uart->port.line, data->clk_count);*/
		}
	}

	return 0;
}
EXPORT_SYMBOL(request_uart_to_wakeup);

static void mtk_uart_save_dev(struct device *dev)
{
	unsigned long flags;
	struct mtk8250_data *data = dev_get_drvdata(dev);
	struct mtk_uart_register *reg = &data->reg;
	struct uart_8250_port *uart = serial8250_get_port(data->line);

	/* DLL may be changed by console write. To avoid this, use spinlock */
	spin_lock_irqsave(&uart->port.lock, flags);

	reg->lcr = serial_port_in(&uart->port, UART_LCR);

	serial_port_out(&uart->port, UART_LCR, 0xbf);

	reg->efr = serial_port_in(&uart->port, UART_EFR);

	serial_port_out(&uart->port, UART_LCR, reg->lcr);

	reg->fcr = serial_port_in(&uart->port, UART_MTK_FCR_RD);

	/* baudrate */
	reg->highspeed = serial_port_in(&uart->port, UART_MTK_HIGHS);
	reg->fracdiv_l = serial_port_in(&uart->port, UART_MTK_FRACDIV_L);
	reg->fracdiv_m = serial_port_in(&uart->port, UART_MTK_FRACDIV_M);
	serial_port_out(&uart->port, UART_LCR, reg->lcr | UART_LCR_DLAB);
	reg->dll = serial_port_in(&uart->port, UART_DLL);
	reg->dlh = serial_port_in(&uart->port, UART_MTK_DLH);
	serial_port_out(&uart->port, UART_LCR, reg->lcr);
	reg->sample_count = serial_port_in(&uart->port, UART_MTK_SAMPLE_COUNT);
	reg->sample_point = serial_port_in(&uart->port, UART_MTK_SAMPLE_POINT);
	reg->guard = serial_port_in(&uart->port, UART_MTK_GUARD);

	/* flow control */
	reg->escape_en = serial_port_in(&uart->port, UART_MTK_ESCAPE_EN);
	reg->mcr = serial_port_in(&uart->port, UART_MCR);
	reg->ier = serial_port_in(&uart->port, UART_IER);

	reg->rx_sel = serial_port_in(&uart->port, UART_MTK_RX_SEL);

	spin_unlock_irqrestore(&uart->port.lock, flags);
}

void mtk_uart_restore(void)
{
	int uart_idx = 0;
	unsigned long flags;
	struct uart_8250_port *uart;
	struct mtk8250_data *data;
	struct mtk_uart_register *reg;

	for (uart_idx = 0; uart_idx < UART_MTK_NR; uart_idx++) {
		uart = serial8250_get_port(uart_idx);
		data = dev_get_drvdata(uart->port.dev);
		reg = &data->reg;

		if (!uart_console(&uart->port))
			continue;

		spin_lock_irqsave(&uart->port.lock, flags);
		serial_port_out(&uart->port, UART_LCR, 0xbf);
		serial_port_out(&uart->port, UART_EFR, reg->efr);
		serial_port_out(&uart->port, UART_LCR, reg->lcr);
		serial_port_out(&uart->port, UART_FCR, reg->fcr);

		/* baudrate */
		serial_port_out(&uart->port, UART_MTK_HIGHS, reg->highspeed);
		serial_port_out(&uart->port, UART_MTK_FRACDIV_L, reg->fracdiv_l);
		serial_port_out(&uart->port, UART_MTK_FRACDIV_M, reg->fracdiv_m);
		serial_port_out(&uart->port, UART_LCR, reg->lcr | UART_LCR_DLAB);
		serial_port_out(&uart->port, UART_DLL, reg->dll);
		serial_port_out(&uart->port, UART_MTK_DLH, reg->dlh);
		serial_port_out(&uart->port, UART_LCR, reg->lcr);
		serial_port_out(&uart->port, UART_MTK_SAMPLE_COUNT, reg->sample_count);
		serial_port_out(&uart->port, UART_MTK_SAMPLE_POINT, reg->sample_point);
		serial_port_out(&uart->port, UART_MTK_GUARD, reg->guard);

		/* flow control */
		serial_port_out(&uart->port, UART_MTK_ESCAPE_EN, reg->escape_en);
		serial_port_out(&uart->port, UART_MCR, reg->mcr);
		serial_port_out(&uart->port, UART_IER, reg->ier);

		serial_port_out(&uart->port, UART_MTK_RX_SEL, reg->rx_sel);

		spin_unlock_irqrestore(&uart->port.lock, flags);
	}
}
EXPORT_SYMBOL(mtk_uart_restore);

static int mtk8250_suspend(struct device *dev)
{
	struct mtk8250_data *data = dev_get_drvdata(dev);
	struct uart_8250_port *uart = serial8250_get_port(data->line);

	if (uart_console(&uart->port))
		mtk_uart_save_dev(dev);
	serial8250_suspend_port(data->line);

	return 0;
}

static int mtk8250_resume(struct device *dev)
{
	struct mtk8250_data *data = dev_get_drvdata(dev);

	serial8250_resume_port(data->line);

	return 0;
}
#endif /* CONFIG_PM_SLEEP */

static const struct dev_pm_ops mtk8250_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mtk8250_suspend, mtk8250_resume)
	SET_RUNTIME_PM_OPS(mtk8250_runtime_suspend, mtk8250_runtime_resume,
				NULL)
};

static const struct of_device_id mtk8250_of_match[] = {
	{ .compatible = "mediatek,mt6577-uart" },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, mtk8250_of_match);

static struct platform_driver mtk8250_platform_driver = {
	.driver = {
		.name		= "mt6577-uart",
		.pm		= &mtk8250_pm_ops,
		.of_match_table	= mtk8250_of_match,
	},
	.probe			= mtk8250_probe,
	.remove			= mtk8250_remove,
};
module_platform_driver(mtk8250_platform_driver);

#ifdef CONFIG_SERIAL_8250_CONSOLE
static int __init early_mtk8250_setup(struct earlycon_device *device,
					const char *options)
{
	if (!device->port.membase)
		return -ENODEV;

	device->port.iotype = UPIO_MEM32;

	return early_serial8250_setup(device, NULL);
}

OF_EARLYCON_DECLARE(mtk8250, "mediatek,mt6577-uart", early_mtk8250_setup);
#endif

MODULE_AUTHOR("Matthias Brugger");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Mediatek 8250 serial port driver");
