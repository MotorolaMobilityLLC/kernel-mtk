#ifndef _PMIC_REGULATOR_H_
#define _PMIC_REGULATOR_H_

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/regulator/of_regulator.h>
#include <linux/of_device.h>
#include <linux/of_fdt.h>
#endif
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/consumer.h>

#include "include/pmic.h"
#include "include/pmic_irq.h"

#define GETSIZE(array) (sizeof(array)/sizeof(array[0]))

extern struct mtk_regulator mtk_ldos[];
extern struct of_regulator_match pmic_regulator_matches[];

extern int mtk_ldos_size;
extern int pmic_regulator_matches_size;


#endif				/* _PMIC_REGULATOR_H_ */
