/*
 * Early serial console for mtk uart chip devices
 *
 * (c) Copyright 2004 Hewlett-Packard Development Company, L.P.
 *	Bjorn Helgaas <bjorn.helgaas@hp.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Based on the 8250.c serial driver, Copyright (C) 2001 Russell King,
 * and on early_printk.c by Andi Kleen.
 *
 * This is for use before the serial driver has initialized, in
 * particular, before the UARTs have been discovered and named.
 * Instead of specifying the console device as, e.g., "ttyS0",
 * we locate the device directly by its MMIO or I/O port address.
 *
 * The user can specify the device directly, e.g.,
 *	earlycon=uartmtk,io,0x3f8,9600n8
 *	earlycon=uartmtk,mmio,0xff5e0000,115200n8
 *	earlycon=uartmtk,mmio32,0xff5e0000,115200n8
 * or
 *	console=uartmtk,io,0x3f8,9600n8
 *	console=uartmtk,mmio,0xff5e0000,115200n8
 *	console=uartmtk,mmio32,0xff5e0000,115200n8
 */

#include <linux/tty.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/serial_core.h>
#include <linux/serial_reg.h>
#include <linux/serial.h>
#include <linux/io.h>
#include <asm/serial.h>



static struct earlycon_device *early_device;


unsigned int __weak __init serial_mtk_earlycon_early_in(struct uart_port *port, int offset)
{
	switch (port->iotype) {
	case UPIO_MEM:
		return readb(port->membase + offset);
	case UPIO_MEM32:
		return readl(port->membase + offset);
	case UPIO_PORT:
		return inb(port->iobase + offset);
	default:
		return 0;
	}
}

void __weak __init serial_mtk_earlycon_early_out(struct uart_port *port, int offset, int value)
{
	switch (port->iotype) {
	case UPIO_MEM:
		writeb(value, port->membase + offset);
		break;
	case UPIO_MEM32:
		writel(value, port->membase + (offset));
		break;
	case UPIO_PORT:
		outb(value, port->iobase + offset);
		break;
	}
}
#define REG_MT6797_UART_BASE			0x00
#define REG_MT6797_UART_STATUS		0x14


static void __init wait_for_xmitr(struct uart_port *port)
{
	unsigned int status;

	for (;;) {
		status = serial_mtk_earlycon_early_in(port, REG_MT6797_UART_STATUS);
		if ((status & 0x20) == 0x20)
			return;
		cpu_relax();
	}
}


static void __init serial_putc(struct uart_port *port, int c)
{
	wait_for_xmitr(port);
	serial_mtk_earlycon_early_out(port, REG_MT6797_UART_BASE, c);
}

static void __init early_serial_mtk_earlycon_write(struct console *console,
					const char *s, unsigned int count)
{
	struct uart_port *port = &early_device->port;

	uart_console_write(port, s, count, serial_putc);

}

static unsigned int __init probe_baud(struct uart_port *port)
{
	return 921600;
}

static void __init init_port(struct earlycon_device *device)
{
 #if 0
	struct uart_port *port = &device->port;
	unsigned int divisor;
	unsigned char c;

	serial_mtk_earlycon_early_out(port, UART_LCR, 0x3);	/* 8n1 */
	serial_mtk_earlycon_early_out(port, UART_IER, 0);	/* no interrupt */
	serial_mtk_earlycon_early_out(port, UART_FCR, 0);	/* no fifo */
	serial_mtk_earlycon_early_out(port, UART_MCR, 0x3);	/* DTR + RTS */

	divisor = DIV_ROUND_CLOSEST(port->uartclk, 16 * device->baud);
	c = serial_mtk_earlycon_early_in(port, UART_LCR);
	serial_mtk_earlycon_early_out(port, UART_LCR, c | UART_LCR_DLAB);
	serial_mtk_earlycon_early_out(port, UART_DLL, divisor & 0xff);
	serial_mtk_earlycon_early_out(port, UART_DLM, (divisor >> 8) & 0xff);
	serial_mtk_earlycon_early_out(port, UART_LCR, c & ~UART_LCR_DLAB);
#endif
}

static int __init early_serial_mtk_earlycon_setup(struct earlycon_device *device,
					 const char *options)
{
	if (!(device->port.membase || device->port.iobase))
		return 0;

#if 1
	if (!device->baud)
		device->baud = probe_baud(&device->port);

	init_port(device);
#endif

	early_device = device;
	device->con->write = early_serial_mtk_earlycon_write;
	return 0;
}
EARLYCON_DECLARE(uartmtk, early_serial_mtk_earlycon_setup);

int __init setup_early_serial_mtk_earlycon_console(char *cmdline)
{
	char match[] = "uartmtk";

	if (cmdline && cmdline[4] == ',')
		match[4] = '\0';

	return setup_earlycon(cmdline);
}

