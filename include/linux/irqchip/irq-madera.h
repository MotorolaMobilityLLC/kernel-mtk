/*
 * Interrupt support for Cirrus Logic Madera codecs
 *
 * Copyright 2016-2017 Cirrus Logic
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef IRQCHIP_MADERA_H
#define IRQCHIP_MADERA_H

#define MADERA_IRQ_FLL1_LOCK		0
#define MADERA_IRQ_FLL2_LOCK		1
#define MADERA_IRQ_FLL3_LOCK		2
#define MADERA_IRQ_FLLAO_LOCK		3
#define MADERA_IRQ_CLK_SYS_ERR		4
#define MADERA_IRQ_CLK_ASYNC_ERR	5
#define MADERA_IRQ_CLK_DSP_ERR		6
#define MADERA_IRQ_HPDET		7
#define MADERA_IRQ_MICDET1		8
#define MADERA_IRQ_MICDET2		9
#define MADERA_IRQ_JD1_RISE		10
#define MADERA_IRQ_JD1_FALL		11
#define MADERA_IRQ_JD2_RISE		12
#define MADERA_IRQ_JD2_FALL		13
#define MADERA_IRQ_MICD_CLAMP_RISE	14
#define MADERA_IRQ_MICD_CLAMP_FALL	15
#define MADERA_IRQ_DRC2_SIG_DET		16
#define MADERA_IRQ_DRC1_SIG_DET		17
#define MADERA_IRQ_ASRC1_IN1_LOCK	18
#define MADERA_IRQ_ASRC1_IN2_LOCK	19
#define MADERA_IRQ_ASRC2_IN1_LOCK	20
#define MADERA_IRQ_ASRC2_IN2_LOCK	21
#define MADERA_IRQ_DSP_IRQ1		22
#define MADERA_IRQ_DSP_IRQ2		23
#define MADERA_IRQ_DSP_IRQ3		24
#define MADERA_IRQ_DSP_IRQ4		25
#define MADERA_IRQ_DSP_IRQ5		26
#define MADERA_IRQ_DSP_IRQ6		27
#define MADERA_IRQ_DSP_IRQ7		28
#define MADERA_IRQ_DSP_IRQ8		29
#define MADERA_IRQ_DSP_IRQ9		30
#define MADERA_IRQ_DSP_IRQ10		31
#define MADERA_IRQ_DSP_IRQ11		32
#define MADERA_IRQ_DSP_IRQ12		33
#define MADERA_IRQ_DSP_IRQ13		34
#define MADERA_IRQ_DSP_IRQ14		35
#define MADERA_IRQ_DSP_IRQ15		36
#define MADERA_IRQ_DSP_IRQ16		37
#define MADERA_IRQ_HP1L_SC		38
#define MADERA_IRQ_HP1R_SC		39
#define MADERA_IRQ_HP2L_SC		40
#define MADERA_IRQ_HP2R_SC		41
#define MADERA_IRQ_HP3L_SC		42
#define MADERA_IRQ_HP3R_SC		43
#define MADERA_IRQ_SPKOUTL_SC		44
#define MADERA_IRQ_SPKOUTR_SC		45
#define MADERA_IRQ_HP1L_ENABLE_DONE	46
#define MADERA_IRQ_HP1R_ENABLE_DONE	47
#define MADERA_IRQ_HP2L_ENABLE_DONE	48
#define MADERA_IRQ_HP2R_ENABLE_DONE	49
#define MADERA_IRQ_HP3L_ENABLE_DONE	50
#define MADERA_IRQ_HP3R_ENABLE_DONE	51
#define MADERA_IRQ_SPKOUTL_ENABLE_DONE	52
#define MADERA_IRQ_SPKOUTR_ENABLE_DONE	53
#define MADERA_IRQ_SPK_SHUTDOWN		54
#define MADERA_IRQ_SPK_OVERHEAT		55
#define MADERA_IRQ_SPK_OVERHEAT_WARN	56
#define MADERA_IRQ_GPIO1		57
#define MADERA_IRQ_GPIO2		58
#define MADERA_IRQ_GPIO3		59
#define MADERA_IRQ_GPIO4		60
#define MADERA_IRQ_GPIO5		61
#define MADERA_IRQ_GPIO6		62
#define MADERA_IRQ_GPIO7		63
#define MADERA_IRQ_GPIO8		64
#define MADERA_IRQ_DSP1_BUS_ERR		65
#define MADERA_IRQ_DSP2_BUS_ERR		66
#define MADERA_IRQ_DSP3_BUS_ERR		67
#define MADERA_IRQ_DSP4_BUS_ERR		68
#define MADERA_IRQ_DSP5_BUS_ERR		69
#define MADERA_IRQ_DSP6_BUS_ERR		70
#define MADERA_IRQ_DSP7_BUS_ERR		71
#define MADERA_IRQ_BOOT_DONE		72

#define MADERA_NUM_IRQ			73

struct madera;

int madera_request_irq(struct madera *madera, int irq, const char *name,
		       irq_handler_t handler, void *data);
void madera_free_irq(struct madera *madera, int irq, void *data);
int madera_set_irq_wake(struct madera *madera, int irq, int on);

#endif
