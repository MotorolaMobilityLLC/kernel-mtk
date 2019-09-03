/*
 * MFD internals for Cirrus Logic Madera codecs
 *
 * Copyright 2015-2017 Cirrus Logic
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef MADERA_CORE_H
#define MADERA_CORE_H

#include <linux/interrupt.h>
#include <linux/notifier.h>
#include <linux/regmap.h>
#include <linux/gpio/consumer.h>
#include <linux/irqchip/irq-madera.h>
#include <linux/mfd/madera/pdata.h>
#include <linux/regulator/consumer.h>
#include <sound/madera-pdata.h>

enum madera_type {
	CS47L35 = 1,
	CS47L85 = 2,
	CS47L90 = 3,
	CS47L91 = 4,
	CS47L92 = 5,
	CS47L93 = 6,
	WM1840 = 7,
	CS47L15 = 8,
};

#define MADERA_MAX_CORE_SUPPLIES	2
#define MADERA_MAX_GPIOS		40

#define CS47L15_NUM_GPIOS		15
#define CS47L35_NUM_GPIOS		16
#define CS47L85_NUM_GPIOS		40
#define CS47L90_NUM_GPIOS		38
#define CS47L92_NUM_GPIOS		16


/* Notifier events */
#define MADERA_NOTIFY_VOICE_TRIGGER	0x1
#define MADERA_NOTIFY_HPDET		0x2
#define MADERA_NOTIFY_MICDET		0x4

/* GPIO Function Definitions */
#define MADERA_GP_FN_ALTERNATE		0x00
#define MADERA_GP_FN_GPIO		0x01
#define MADERA_GP_FN_DSP_GPIO		0x02
#define MADERA_GP_FN_IRQ1		0x03
#define MADERA_GP_FN_IRQ2		0x04
#define MADERA_GP_FN_FLL1_CLOCK		0x10
#define MADERA_GP_FN_FLL2_CLOCK		0x11
#define MADERA_GP_FN_FLL3_CLOCK		0x12
#define MADERA_GP_FN_FLLAO_CLOCK	0x13
#define MADERA_GP_FN_FLL1_LOCK		0x18
#define MADERA_GP_FN_FLL2_LOCK		0x19
#define MADERA_GP_FN_FLL3_LOCK		0x1A
#define MADERA_GP_FN_FLLAO_LOCK		0x1B
#define MADERA_GP_FN_OPCLK_OUT		0x40
#define MADERA_GP_FN_OPCLK_ASYNC_OUT	0x41
#define MADERA_GP_FN_PWM1		0x48
#define MADERA_GP_FN_PWM2		0x49
#define MADERA_GP_FN_SPDIF_OUT		0x4C
#define MADERA_GP_FN_HEADPHONE_DET	0x50
#define MADERA_GP_FN_MIC_DET		0x58
#define MADERA_GP_FN_DRC1_SIGNAL_DETECT	0x80
#define MADERA_GP_FN_DRC2_SIGNAL_DETECT	0x81
#define MADERA_GP_FN_ASRC1_IN1_LOCK	0x88
#define MADERA_GP_FN_ASRC1_IN2_LOCK	0x89
#define MADERA_GP_FN_ASRC2_IN1_LOCK	0x8A
#define MADERA_GP_FN_ASRC2_IN2_LOCK	0x8B
#define MADERA_GP_FN_DSP_IRQ1		0xA0
#define MADERA_GP_FN_DSP_IRQ2		0xA1
#define MADERA_GP_FN_DSP_IRQ3		0xA2
#define MADERA_GP_FN_DSP_IRQ4		0xA3
#define MADERA_GP_FN_DSP_IRQ5		0xA4
#define MADERA_GP_FN_DSP_IRQ6		0xA5
#define MADERA_GP_FN_DSP_IRQ7		0xA6
#define MADERA_GP_FN_DSP_IRQ8		0xA7
#define MADERA_GP_FN_DSP_IRQ9		0xA8
#define MADERA_GP_FN_DSP_IRQ10		0xA9
#define MADERA_GP_FN_DSP_IRQ11		0xAA
#define MADERA_GP_FN_DSP_IRQ12		0xAB
#define MADERA_GP_FN_DSP_IRQ13		0xAC
#define MADERA_GP_FN_DSP_IRQ14		0xAD
#define MADERA_GP_FN_DSP_IRQ15		0xAE
#define MADERA_GP_FN_DSP_IRQ16		0xAF
#define MADERA_GP_FN_HPOUT1L_SC		0xB0
#define MADERA_GP_FN_HPOUT1R_SC		0xB1
#define MADERA_GP_FN_HPOUT2L_SC		0xB2
#define MADERA_GP_FN_HPOUT2R_SC		0xB3
#define MADERA_GP_FN_HPOUT3L_SC		0xB4
#define MADERA_GP_FN_HPOUT4R_SC		0xB5
#define MADERA_GP_FN_SPKOUTL_SC		0xB6
#define MADERA_GP_FN_SPKOUTR_SC		0xB7
#define MADERA_GP_FN_HPOUT1L_ENA	0xC0
#define MADERA_GP_FN_HPOUT1R_ENA	0xC1
#define MADERA_GP_FN_HPOUT2L_ENA	0xC2
#define MADERA_GP_FN_HPOUT2R_ENA	0xC3
#define MADERA_GP_FN_HPOUT3L_ENA	0xC4
#define MADERA_GP_FN_HPOUT4R_ENA	0xC5
#define MADERA_GP_FN_SPKOUTL_ENA	0xC6
#define MADERA_GP_FN_SPKOUTR_ENA	0xC7
#define MADERA_GP_FN_HPOUT1L_DIS	0xD0
#define MADERA_GP_FN_HPOUT1R_DIS	0xD1
#define MADERA_GP_FN_HPOUT2L_DIS	0xD2
#define MADERA_GP_FN_HPOUT2R_DIS	0xD3
#define MADERA_GP_FN_HPOUT3L_DIS	0xD4
#define MADERA_GP_FN_HPOUT4R_DIS	0xD5
#define MADERA_GP_FN_SPKOUTL_DIS	0xD6
#define MADERA_GP_FN_SPKOUTR_DIS	0xD7
#define MADERA_GP_FN_SPK_SHUTDOWN	0xE0
#define MADERA_GP_FN_SPK_OVH_SHUTDOWN	0xE1
#define MADERA_GP_FN_SPK_OVH_WARN	0xE2
#define MADERA_GP_FN_TIMER1_STATUS	0x140
#define MADERA_GP_FN_TIMER2_STATUS	0x141
#define MADERA_GP_FN_TIMER3_STATUS	0x142
#define MADERA_GP_FN_TIMER4_STATUS	0x143
#define MADERA_GP_FN_TIMER5_STATUS	0x144
#define MADERA_GP_FN_TIMER6_STATUS	0x145
#define MADERA_GP_FN_TIMER7_STATUS	0x146
#define MADERA_GP_FN_TIMER8_STATUS	0x147
#define MADERA_GP_FN_EVENTLOG1_FIFO_STS	0x150
#define MADERA_GP_FN_EVENTLOG2_FIFO_STS	0x151
#define MADERA_GP_FN_EVENTLOG3_FIFO_STS	0x152
#define MADERA_GP_FN_EVENTLOG4_FIFO_STS	0x153
#define MADERA_GP_FN_EVENTLOG5_FIFO_STS	0x154
#define MADERA_GP_FN_EVENTLOG6_FIFO_STS	0x155
#define MADERA_GP_FN_EVENTLOG7_FIFO_STS	0x156
#define MADERA_GP_FN_EVENTLOG8_FIFO_STS	0x157

struct snd_soc_dapm_context;

/*
 * struct madera
 *
 * This should not be used by anything except child drivers of the Madera MFD
 */
struct madera {
	struct regmap *regmap;
	struct regmap *regmap_32bit;

	struct device *dev;

	enum madera_type type;
	unsigned int rev;

	struct gpio_desc *reset_gpio;

	int num_core_supplies;
	struct regulator_bulk_data core_supplies[MADERA_MAX_CORE_SUPPLIES];
	struct regulator *dcvdd;
	bool internal_dcvdd;

	struct madera_pdata pdata;

	struct device *irq_dev;
	int irq;

	unsigned int out_clamp[MADERA_MAX_OUTPUT];
	unsigned int out_shorted[MADERA_MAX_OUTPUT];
	unsigned int hp_ena;
	unsigned int hp_impedance_x100[MADERA_MAX_ACCESSORY];

	struct snd_soc_dapm_context *dapm;

	struct mutex reg_setting_lock;

	struct blocking_notifier_head notifier;
};

unsigned int madera_get_num_micbias(struct madera *madera);
unsigned int madera_get_num_childbias(struct madera *madera,
				      unsigned int micbias);

const char *madera_name_from_type(enum madera_type type);

static inline int madera_call_notifiers(struct madera *madera,
					unsigned long event,
					void *data)
{
	return blocking_notifier_call_chain(&madera->notifier, event, data);
}
#endif
