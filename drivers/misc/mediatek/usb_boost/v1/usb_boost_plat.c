/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
#include <linux/module.h>
#include <linux/pm_qos.h>
#include "mtk_ppm_api.h"
#include "usb_boost.h"
#include <mtk_vcorefs_manager.h>
#ifdef CONFIG_MTK_QOS_SUPPORT
#include <helio-dvfsrc-opp.h>
#endif

/* platform specific parameter here */
#ifdef CONFIG_MACH_MT6799
static int cpu_freq_test_para[] = {1, 5, 500, 0};
static int cpu_core_test_para[] = {1, 5, 500, 0};
static int dram_vcore_test_para[] = {1, 5, 500, 0};

/* -1 denote not used*/
struct act_arg_obj cpu_freq_test_arg = {2000000, -1, -1};
struct act_arg_obj cpu_core_test_arg = {4, -1, -1};
struct act_arg_obj dram_vcore_test_arg = {OPP_UNREQ, -1, -1};

#elif defined(CONFIG_MACH_MT6759)
static int cpu_freq_test_para[] = {1, 5, 500, 0};
static int cpu_core_test_para[] = {1, 5, 500, 0};
static int dram_vcore_test_para[] = {1, 5, 500, 0};

/* -1 denote not used*/
struct act_arg_obj cpu_freq_test_arg = {2000000, -1, -1};
struct act_arg_obj cpu_core_test_arg = {4, -1, -1};
struct act_arg_obj dram_vcore_test_arg = {OPP_UNREQ, -1, -1};

#elif defined(CONFIG_MACH_MT6763)
static int cpu_freq_test_para[] = {1, 5, 500, 0};
static int cpu_core_test_para[] = {1, 5, 500, 0};
static int dram_vcore_test_para[] = {1, 5, 500, 0};

/* -1 denote not used*/
struct act_arg_obj cpu_freq_test_arg = {2500000, -1, -1};
struct act_arg_obj cpu_core_test_arg = {4, -1, -1};
struct act_arg_obj dram_vcore_test_arg = {OPP_0, -1, -1};

#elif defined(CONFIG_MACH_MT6739)
static int cpu_freq_test_para[] = {1, 5, 500, 0};
static int cpu_core_test_para[] = {1, 5, 500, 0};
static int dram_vcore_test_para[] = {1, 5, 500, 0};

/* -1 denote not used*/
struct act_arg_obj cpu_freq_test_arg = {1500000, -1, -1};
struct act_arg_obj cpu_core_test_arg = {4, -1, -1};
struct act_arg_obj dram_vcore_test_arg = {OPP_0, -1, -1};

#elif defined(CONFIG_MACH_MT6771)
static int cpu_freq_test_para[] = {1, 5, 500, 0};
static int cpu_core_test_para[] = {1, 5, 500, 0};
static int dram_vcore_test_para[] = {1, 5, 500, 0};

/* -1 denote not used*/
struct act_arg_obj cpu_freq_test_arg = {2500000, -1, -1};
struct act_arg_obj cpu_core_test_arg = {4, -1, -1};
struct act_arg_obj dram_vcore_test_arg = {OPP_0, -1, -1};

#elif defined(CONFIG_MACH_MT6775)
static int cpu_freq_test_para[] = {1, 5, 500, 0};
static int cpu_core_test_para[] = {1, 5, 500, 0};
static int dram_vcore_test_para[] = {1, 5, 500, 0};

/* -1 denote not used*/
struct act_arg_obj cpu_freq_test_arg = {2500000, -1, -1};
struct act_arg_obj cpu_core_test_arg = {4, -1, -1};
struct act_arg_obj dram_vcore_test_arg = {OPP_0, -1, -1};

#elif defined(CONFIG_ARCH_MT6XXX)
/* add new here */
#endif

static struct pm_qos_request pm_qos_req;
#ifdef CONFIG_MTK_QOS_SUPPORT
static struct pm_qos_request pm_qos_emi_req;
#endif

static int freq_hold(struct act_arg_obj *arg)
{
	USB_BOOST_DBG("\n");
	mt_ppm_sysboost_freq(BOOST_BY_USB, arg->arg1);
	return 0;
}
static int freq_release(struct act_arg_obj *arg)
{
	USB_BOOST_DBG("\n");
	mt_ppm_sysboost_freq(BOOST_BY_USB, 0);
	return 0;
}

static int core_hold(struct act_arg_obj *arg)
{
	USB_BOOST_DBG("\n");

	/*This API is deprecated*/
	mt_ppm_sysboost_core(BOOST_BY_USB, arg->arg1);

	/*Disable MCDI to save around 100us "Power ON CPU -> CPU context restore"*/
	pm_qos_update_request(&pm_qos_req, 50);
	return 0;
}

static int core_release(struct act_arg_obj *arg)
{
	USB_BOOST_DBG("\n");

	/*This API is deprecated*/
	mt_ppm_sysboost_core(BOOST_BY_USB, 0);

	/*Enable MCDI*/
	pm_qos_update_request(&pm_qos_req, PM_QOS_DEFAULT_VALUE);
	return 0;
}

static int vcorefs_hold(struct act_arg_obj *arg)
{
#ifdef CONFIG_MTK_QOS_SUPPORT
	pm_qos_update_request(&pm_qos_emi_req, DDR_OPP_0);
	return 0;
#else
	int vcore_ret;

	vcore_ret = vcorefs_request_dvfs_opp(KIR_USB, arg->arg1);
	if (vcore_ret)
		USB_BOOST_DBG("hold VCORE fail (%d)\n", vcore_ret);
	else
		USB_BOOST_DBG("hold VCORE ok\n");

	return 0;
#endif
}

static int vcorefs_release(struct act_arg_obj *arg)
{
#ifdef CONFIG_MTK_QOS_SUPPORT
	pm_qos_update_request(&pm_qos_emi_req, DDR_OPP_UNREQ);
	return 0;
#else
	int vcore_ret;

	vcore_ret = vcorefs_request_dvfs_opp(KIR_USB, OPP_UNREQ);
	if (vcore_ret)
		USB_BOOST_DBG("release VCORE fail(%d)\n", vcore_ret);
	else
		USB_BOOST_DBG("release VCORE ok\n");

	return 0;
#endif
}

static int __init init(void)
{
	/* mandatory, related resource inited*/
	usb_boost_init();

	/* optional, change setting for alorithm other than default*/
	usb_boost_set_para_and_arg(TYPE_CPU_FREQ, cpu_freq_test_para,
			ARRAY_SIZE(cpu_freq_test_para), &cpu_freq_test_arg);
	usb_boost_set_para_and_arg(TYPE_CPU_CORE, cpu_core_test_para,
			ARRAY_SIZE(cpu_core_test_para), &cpu_core_test_arg);
	usb_boost_set_para_and_arg(TYPE_DRAM_VCORE, dram_vcore_test_para,
			ARRAY_SIZE(dram_vcore_test_para), &dram_vcore_test_arg);

	/* mandatory, hook callback depends on platform */
	register_usb_boost_act(TYPE_CPU_FREQ, ACT_HOLD, freq_hold);
	register_usb_boost_act(TYPE_CPU_FREQ, ACT_RELEASE, freq_release);
	register_usb_boost_act(TYPE_CPU_CORE, ACT_HOLD, core_hold);
	register_usb_boost_act(TYPE_CPU_CORE, ACT_RELEASE, core_release);
	register_usb_boost_act(TYPE_DRAM_VCORE, ACT_HOLD, vcorefs_hold);
	register_usb_boost_act(TYPE_DRAM_VCORE, ACT_RELEASE, vcorefs_release);


	pm_qos_add_request(&pm_qos_req, PM_QOS_CPU_DMA_LATENCY, PM_QOS_DEFAULT_VALUE);
#ifdef CONFIG_MTK_QOS_SUPPORT
	pm_qos_add_request(&pm_qos_emi_req, PM_QOS_EMI_OPP, PM_QOS_EMI_OPP_DEFAULT_VALUE);
#endif

	return 0;
}
module_init(init);
