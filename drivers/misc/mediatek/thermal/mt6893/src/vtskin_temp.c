// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#include <linux/bits.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/thermal.h>
#include "vtskin_temp.h"
#include "mt-plat/mtk_thermal_monitor.h"
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <mt-plat/aee.h>





static kuid_t uid = KUIDT_INIT(0);
static kgid_t gid = KGIDT_INIT(1000);
static DEFINE_SEMAPHORE(sem_mutex);


static int mtktsvtskin_debug_log = 1;
static unsigned int cl_dev_sysrst_state;

#define VTSKIN_MAX_INDEX  0
#define VTSKIN1_INDEX     1
#define VTSKIN2_INDEX     2
#define VTSKIN3_INDEX     3





#define mtktsvtskin_TEMP_CRIT 100000	/* 100.000 degree Celsius */
static int kernelmode;

//Common to all vtskin

static unsigned int interval = 1;	/* seconds, 0 : no auto polling */



//Vtskin-max

static int g_THERMAL_TRIP[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

//Trip tempratures

static int trip_temp[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
//coolers
static int num_trip ;
static char g_bind0[20] = { 0 };
static char g_bind1[20] = { 0 };
static char g_bind2[20] = { 0 };
static char g_bind3[20] = { 0 };
static char g_bind4[20] = { 0 };
static char g_bind5[20] = { 0 };
static char g_bind6[20] = { 0 };
static char g_bind7[20] = { 0 };
static char g_bind8[20] = { 0 };
static char g_bind9[20] = { 0 };




//Vtskin1


static int g_THERMAL_TRIP1[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

//Trip tempratures

static int trip_temp1[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
//coolers
static int num_trip1 ;
static char g_bind10[20] = { 0 };
static char g_bind11[20] = { 0 };
static char g_bind12[20] = { 0 };
static char g_bind13[20] = { 0 };
static char g_bind14[20] = { 0 };
static char g_bind15[20] = { 0 };
static char g_bind16[20] = { 0 };
static char g_bind17[20] = { 0 };
static char g_bind18[20] = { 0 };
static char g_bind19[20] = { 0 };





//Vtskin2


static int g_THERMAL_TRIP2[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

//Trip tempratures

static int trip_temp2[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
//coolers
static int num_trip2 ;
static char g_bind20[20] = { 0 };
static char g_bind21[20] = { 0 };
static char g_bind22[20] = { 0 };
static char g_bind23[20] = { 0 };
static char g_bind24[20] = { 0 };
static char g_bind25[20] = { 0 };
static char g_bind26[20] = { 0 };
static char g_bind27[20] = { 0 };
static char g_bind28[20] = { 0 };
static char g_bind29[20] = { 0 };




//Vtskin3

static int g_THERMAL_TRIP3[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

//Trip tempratures

static int trip_temp3[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
//coolers
static int num_trip3 ;
static char g_bind30[20] = { 0 };
static char g_bind31[20] = { 0 };
static char g_bind32[20] = { 0 };
static char g_bind33[20] = { 0 };
static char g_bind34[20] = { 0 };
static char g_bind35[20] = { 0 };
static char g_bind36[20] = { 0 };
static char g_bind37[20] = { 0 };
static char g_bind38[20] = { 0 };
static char g_bind39[20] = { 0 };



//Poling factor

static int polling_trip_temp1 = 40000;
static int polling_trip_temp2 = 20000;
static int polling_factor1 = 5000;
static int polling_factor2 = 10000;




#define mtktsvtskin_dprintk(fmt, args...)   \
do {                                    \
	if (mtktsvtskin_debug_log) {                \
		pr_notice("[Thermal/TZ/VTSKIN]" fmt, ##args); \
	}                                   \
} while (0)

#define mtktsvtskin_printk(fmt, args...)   \
pr_notice("[Thermal/TZ/VTSKIN]" fmt, ##args)


//change to enum

static int mtk_thermal_get_vtksin_idx(char *type)
{
	if (strncmp(type, "vtskin-max", 10) == 0)
		return 0;
	else if (strncmp(type, "vtskin1", 7) == 0)
		return 1;
	else if (strncmp(type, "vtskin2", 7) == 0)
		return 2;
	else if (strncmp(type, "vtskin3", 7) == 0)
		return 3;
	else if (strncmp(type, "vtskin4", 7) == 0)
		return 4;
	else if (strncmp(type, "vtskin5", 7) == 0)
		return 5;
	else if (strncmp(type, "vtskin6", 7) == 0)
		return 6;
	return -1;
}


char *mtk_thermal_get_vtksin_tz_name(int id)
{

	if (id == 0)
		return "vtskin-max";
	else if (id == 1)
		return "vtskin1";
	else if (id == 2)
		return "vtskin2";
	else if (id == 3)
		return "vtskin3";
	else if (id == 4)
		return "vtskin4";
	else if (id == 5)
		return "vtskin5";
	else if (id == 6)
		return "vtskin6";

	return NULL;
}


char *mtk_thermal_get_vtksin_tz_proc_name(int id)
{

	if (id == 0)
		return "tzvtskin-max";
	else if (id == 1)
		return "tzvtskin1";
	else if (id == 2)
		return "tzvtskin2";
	else if (id == 3)
		return "tzvtskin3";
	else if (id == 4)
		return "tzvtskin4";
	else if (id == 5)
		return "tzvtskin5";
	else if (id == 6)
		return "tzvtskin6";

	return NULL;
}


enum mt6893_vtskin_sensor_enum {
	MT6893_VTSKIN_MAX,
	MT6893_VTSKIN_1,
	MT6893_VTSKIN_2,
	MT6893_VTSKIN_3,
	MT6893_VTSKIN_4,
	MT6893_VTSKIN_5,
	MT6893_VTSKIN_6,
	MT6893_VTSKIN_NUM,
};
struct vtskin_tz_param mt6893_vtskin_params[] = {
	[MT6893_VTSKIN_MAX] = {
		.ref_num = 0,
		.operation = OP_MAX,
	},
	[MT6893_VTSKIN_1] = {
		.ref_num = 0,
		.operation = OP_COEF,
	},
	[MT6893_VTSKIN_2] = {
		.ref_num = 0,
		.operation = OP_COEF,
	},
	[MT6893_VTSKIN_3] = {
		.ref_num = 0,
		.operation = OP_COEF,
	},
	[MT6893_VTSKIN_4] = {
		.ref_num = 0,
		.operation = OP_COEF,
	},
	[MT6893_VTSKIN_5] = {
		.ref_num = 0,
		.operation = OP_COEF,
	},
	[MT6893_VTSKIN_6] = {
		.ref_num = 0,
		.operation = OP_COEF,
	}
};
static struct vtskin_data mt6893_vtskin_data = {
	.num_sensor = MT6893_VTSKIN_NUM,
	.params = mt6893_vtskin_params,
};



static int tsvtskin_sysrst_get_max_state(
struct thermal_cooling_device *cdev, unsigned long *state)
{
	*state = 1;
	return 0;
}

static int tsvtskin_sysrst_get_cur_state(
struct thermal_cooling_device *cdev, unsigned long *state)
{
	*state = cl_dev_sysrst_state;
	return 0;
}

static int tsvtskin_sysrst_set_cur_state(
struct thermal_cooling_device *cdev, unsigned long state)
{

	cl_dev_sysrst_state = state;
	if (cl_dev_sysrst_state == 1) {
		pr_notice("Power/vtskin: reset, reset, reset!!!");
		pr_notice("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
		pr_notice("*****************************************");
		pr_notice("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");

		/* To trigger data abort to reset the system
		 * for thermal protection.
		 */
		BUG_ON(1);
	}
	return 0;
}

/*
 *static struct thermal_cooling_device_ops
 * mtktsbattery_cooling_dis_charge_ops = {
 *	.get_max_state = dis_charge_get_max_state,
 *	.get_cur_state = dis_charge_get_cur_state,
 *	.set_cur_state = dis_charge_set_cur_state,
 *};
 */
static struct thermal_cooling_device_ops mtktsvtskin_cooling_sysrst_ops = {
	.get_max_state = tsvtskin_sysrst_get_max_state,
	.get_cur_state = tsvtskin_sysrst_get_cur_state,
	.set_cur_state = tsvtskin_sysrst_set_cur_state,
};




int mtktsvtskin_register_cooler(int id)
{
	/* cooling devices */
	 char tz_name[20] = {0};

	 strcpy(tz_name,mtk_thermal_get_vtksin_tz_name(id));
	 mtk_thermal_cooling_device_register(
					strcat(tz_name,"-sysrst"), NULL,
					&mtktsvtskin_cooling_sysrst_ops);
	return 0;
}

static struct vtskin_data *parse_dt(struct platform_device *pdev)
{
	struct device_node *root, *vtskin_map, *vtskin_child_map;
	int count = 0;
	int num_sensor = 0;
	struct vtskin_tz_param *params;
	struct device *dev = &pdev->dev;

	root = pdev->dev.of_node;
	of_property_read_u32(root, "num-sensor", &num_sensor);
	pr_notice("vtskin num_sensor %d\n", num_sensor);
	mt6893_vtskin_data.num_sensor = num_sensor;
	mt6893_vtskin_data.dev = dev;
	mt6893_vtskin_data.params = devm_kzalloc(dev, num_sensor*sizeof(*params), GFP_KERNEL);
	if(!mt6893_vtskin_data.params){
		pr_notice("Error: Failed to allocate memory\n");
		return NULL;
	}
	for_each_child_of_node(root, vtskin_map) {
		int ref_num = 0;
		char *operation = NULL;
		int child_count = 0;

		of_property_read_u32(vtskin_map, "ref-num", &ref_num);
		pr_notice("vtskin ref_num  %d\n", ref_num);
		mt6893_vtskin_data.params[count].ref_num= ref_num;
		of_property_read_string(vtskin_map, "operation", (const char **)&operation);
		pr_notice(" vtskin operation %s\n", operation);

		if (!strcmp(operation, "OP_COEF"))
			mt6893_vtskin_data.params[count].operation = OP_COEF;
		else if(!strcmp(operation, "OP_MAX"))
			mt6893_vtskin_data.params[count].operation = OP_MAX;
		else
			pr_notice("vtskin wrong coefficient %s\n", operation);

		for_each_child_of_node(vtskin_map, vtskin_child_map){
			char *sensor_name = NULL;
			int sensor_coeff = 0;

			of_property_read_string(vtskin_child_map, "sensor-name",
				(const char **)&sensor_name);
			of_property_read_s32(vtskin_child_map, "sensor-coeff", &sensor_coeff);
			pr_notice("vtskin sensor-name %s\n", sensor_name);
			pr_notice("vtskin sensor-coeff  %lld\n", sensor_coeff);
			strcpy(
			mt6893_vtskin_data.params[count].vtskin_ref[child_count].sensor_name,
				sensor_name);
			mt6893_vtskin_data.params[count].vtskin_ref[child_count].sensor_coef =
				sensor_coeff;
			child_count++;
		}
		count++;
	}
	return &mt6893_vtskin_data;
}







static int mtktsvtskin3_bind(struct thermal_zone_device *thermal,
			     struct thermal_cooling_device *cdev)
{
	int table_val = 0;

	if (!strcmp(cdev->type, g_bind30)) {
		table_val = 0;
		pr_notice("g_bind0 [%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind31)) {
		table_val = 1;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind32)) {
		table_val = 2;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind33)) {
		table_val = 3;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind34)) {
		table_val = 4;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind35)) {
		table_val = 5;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind36)) {
		table_val = 6;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind37)) {
		table_val = 7;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind38)) {
		table_val = 8;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind39)) {
		table_val = 9;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else {
		pr_notice("mtktsvtskin_bind called return 0\n");
		return 0;
	}

	if (mtk_thermal_zone_bind_cooling_device(thermal, table_val, cdev)) {
		pr_notice(
			"[%s] error binding cooling dev\n", __func__);
		return -EINVAL;
	}

	pr_notice("[%s] binding OK, %d\n", __func__, table_val);
	return 0;
}

static int mtktsvtskin3_unbind(struct thermal_zone_device *thermal,
			       struct thermal_cooling_device *cdev)
{
	int table_val = 0;

	if (!strcmp(cdev->type, g_bind30)) {
		table_val = 0;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind31)) {
		table_val = 1;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind32)) {
		table_val = 2;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind33)) {
		table_val = 3;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind34)) {
		table_val = 4;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind35)) {
		table_val = 5;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind36)) {
		table_val = 6;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind37)) {
		table_val = 7;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind38)) {
		table_val = 8;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind39)) {
		table_val = 9;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else
		return 0;

	if (thermal_zone_unbind_cooling_device(thermal, table_val, cdev)) {
		mtktsvtskin_dprintk(
				"[%s] error unbinding cooling dev\n", __func__);

		return -EINVAL;
	}

	mtktsvtskin_dprintk("[%s] unbinding OK\n", __func__);
	return 0;
}




static int mtktsvtskin2_bind(struct thermal_zone_device *thermal,
			     struct thermal_cooling_device *cdev)
{
	int table_val = 0;

	if (!strcmp(cdev->type, g_bind20)) {
		table_val = 0;
		pr_notice("g_bind0 [%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind21)) {
		table_val = 1;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind22)) {
		table_val = 2;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind23)) {
		table_val = 3;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind24)) {
		table_val = 4;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind25)) {
		table_val = 5;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind26)) {
		table_val = 6;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind27)) {
		table_val = 7;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind28)) {
		table_val = 8;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind29)) {
		table_val = 9;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else {
		pr_notice("mtktsvtskin_bind called return 0\n");
		return 0;
	}

	if (mtk_thermal_zone_bind_cooling_device(thermal, table_val, cdev)) {
		pr_notice(
			"[%s] error binding cooling dev\n", __func__);
		return -EINVAL;
	}

	pr_notice("[%s] binding OK, %d\n", __func__, table_val);
	return 0;
}

static int mtktsvtskin2_unbind(struct thermal_zone_device *thermal,
			       struct thermal_cooling_device *cdev)
{
	int table_val = 0;

	if (!strcmp(cdev->type, g_bind20)) {
		table_val = 0;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind21)) {
		table_val = 1;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind22)) {
		table_val = 2;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind23)) {
		table_val = 3;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind24)) {
		table_val = 4;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind25)) {
		table_val = 5;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind26)) {
		table_val = 6;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind27)) {
		table_val = 7;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind28)) {
		table_val = 8;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind29)) {
		table_val = 9;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else
		return 0;

	if (thermal_zone_unbind_cooling_device(thermal, table_val, cdev)) {
		mtktsvtskin_dprintk(
				"[%s] error unbinding cooling dev\n", __func__);

		return -EINVAL;
	}

	mtktsvtskin_dprintk("[%s] unbinding OK\n", __func__);
	return 0;
}




static int mtktsvtskin1_bind(struct thermal_zone_device *thermal,
			     struct thermal_cooling_device *cdev)
{
	int table_val = 0;

	if (!strcmp(cdev->type, g_bind10)) {
		table_val = 0;
		pr_notice("g_bind0 [%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind11)) {
		table_val = 1;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind12)) {
		table_val = 2;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind13)) {
		table_val = 3;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind14)) {
		table_val = 4;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind15)) {
		table_val = 5;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind16)) {
		table_val = 6;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind17)) {
		table_val = 7;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind18)) {
		table_val = 8;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind19)) {
		table_val = 9;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else {
		pr_notice(dev, "mtktsvtskin_bind called return 0\n");
		return 0;
	}

	if (mtk_thermal_zone_bind_cooling_device(thermal, table_val, cdev)) {
		pr_notice(
			"[%s] error binding cooling dev\n", __func__);
		return -EINVAL;
	}

	pr_notice("[%s] binding OK, %d\n", __func__, table_val);
	return 0;
}

static int mtktsvtskin1_unbind(struct thermal_zone_device *thermal,
			       struct thermal_cooling_device *cdev)
{
	int table_val = 0;

	if (!strcmp(cdev->type, g_bind10)) {
		table_val = 0;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind11)) {
		table_val = 1;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind12)) {
		table_val = 2;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind13)) {
		table_val = 3;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind14)) {
		table_val = 4;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind15)) {
		table_val = 5;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind16)) {
		table_val = 6;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind17)) {
		table_val = 7;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind18)) {
		table_val = 8;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind19)) {
		table_val = 9;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else
		return 0;

	if (thermal_zone_unbind_cooling_device(thermal, table_val, cdev)) {
		mtktsvtskin_dprintk(
				"[%s] error unbinding cooling dev\n", __func__);

		return -EINVAL;
	}

	mtktsvtskin_dprintk("[%s] unbinding OK\n", __func__);
	return 0;
}


static int mtktsvtskin_bind(struct thermal_zone_device *thermal,
			     struct thermal_cooling_device *cdev)
{
	int table_val = 0;


	if (!strcmp(cdev->type, g_bind0)) {
		table_val = 0;
		pr_notice("g_bind0 [%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind1)) {
		table_val = 1;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind2)) {
		table_val = 2;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind3)) {
		table_val = 3;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind4)) {
		table_val = 4;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind5)) {
		table_val = 5;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind6)) {
		table_val = 6;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind7)) {
		table_val = 7;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind8)) {
		table_val = 8;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind9)) {
		table_val = 9;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else {
		return 0;
	}

	if (mtk_thermal_zone_bind_cooling_device(thermal, table_val, cdev)) {
		pr_notice(
			"[%s] error binding cooling dev\n", __func__);
		return -EINVAL;
	}

	pr_notice("[%s] binding OK, %d\n", __func__, table_val);
	return 0;
}

static int mtktsvtskin_unbind(struct thermal_zone_device *thermal,
			       struct thermal_cooling_device *cdev)
{
	int table_val = 0;

	int id = mtk_thermal_get_vtksin_idx(thermal->type);
		if(id == 0)
			return 0;

	if (!strcmp(cdev->type, g_bind0)) {
		table_val = 0;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind1)) {
		table_val = 1;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind2)) {
		table_val = 2;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind3)) {
		table_val = 3;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind4)) {
		table_val = 4;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind5)) {
		table_val = 5;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind6)) {
		table_val = 6;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind7)) {
		table_val = 7;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind8)) {
		table_val = 8;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else if (!strcmp(cdev->type, g_bind9)) {
		table_val = 9;
		mtktsvtskin_dprintk("[%s] %s\n", __func__, cdev->type);
	} else
		return 0;

	if (thermal_zone_unbind_cooling_device(thermal, table_val, cdev)) {
		mtktsvtskin_dprintk(
				"[%s] error unbinding cooling dev\n", __func__);

		return -EINVAL;
	}

	mtktsvtskin_dprintk("[%s] unbinding OK\n", __func__);
	return 0;
}


//Common to all Vtskin

static int mtktsvtskin_get_temp(struct thermal_zone_device *thermal, int *temp)
{

	struct vtskin_data *skin_data = &mt6893_vtskin_data;
	struct vtskin_tz_param *skin_param = skin_data->params;
	struct thermal_zone_device *tzd;
	long long vtskin = 0, coef;
	int tz_temp, i, ret;
	char *sensor_name;
	int id = mtk_thermal_get_vtksin_idx(thermal->type);

	if (skin_param[id].ref_num == 0) {
		*temp = THERMAL_TEMP_INVALID;
		return 0;
	}
	for (i = 0; i < skin_param[id].ref_num; i++) {
		sensor_name = skin_param[id].vtskin_ref[i].sensor_name;
		if (!sensor_name) {
			pr_notice("get sensor name fail %d\n", i);
			*temp = THERMAL_TEMP_INVALID;
			return -EINVAL;
		}
		pr_notice("vtskin_get_temp sensor_name   %s\n", sensor_name);
		tzd = thermal_zone_get_zone_by_name(sensor_name);
		if (IS_ERR_OR_NULL(tzd) || !tzd->ops->get_temp) {
			pr_notice("get %s temp fail\n", sensor_name);
			*temp = THERMAL_TEMP_INVALID;
			return -EINVAL;
		}
		ret = tzd->ops->get_temp(tzd, &tz_temp);
		if (ret < 0) {
			pr_notice("%s get_temp fail %d\n", sensor_name, ret);
			*temp = THERMAL_TEMP_INVALID;
			return -EINVAL;
		}
		if (skin_param[id].operation == OP_MAX) {
			if (i == 0)
				*temp = THERMAL_TEMP_INVALID;
			if (tz_temp > *temp)
				*temp = tz_temp;

			pr_notice("get tz_temp %d\n", *temp);
		} else if (skin_param[id].operation == OP_COEF) {
			coef = skin_param[id].vtskin_ref[i].sensor_coef;
			pr_notice("sensor_coef:  %d\n", coef);
			vtskin += tz_temp * coef;
			if (i == skin_param[id].ref_num - 1){
				pr_notice(" divide by vtskin / 10");
				*temp = (int)(vtskin / 10);
			}
		}
	}



	if ((int)*temp >= polling_trip_temp1)
		thermal->polling_delay = interval * 1000;
	else if ((int)*temp < polling_trip_temp2)
		thermal->polling_delay = interval * polling_factor2;
	else
		thermal->polling_delay = interval * polling_factor1;
	return 0;
}






//Common to all Vtskin

static int mtktsvtskin_get_mode(
struct thermal_zone_device *thermal, enum thermal_device_mode *mode)
{
	*mode = (kernelmode) ? THERMAL_DEVICE_ENABLED : THERMAL_DEVICE_DISABLED;
	return 0;
}

static int mtktsvtskin_set_mode(
struct thermal_zone_device *thermal, enum thermal_device_mode mode)
{
	kernelmode = mode;
	return 0;
}


static int mtktsvtskin_get_crit_temp(struct thermal_zone_device *thermal,
				      int *temperature)
{
	*temperature = mtktsvtskin_TEMP_CRIT;
	return 0;
}


 //vtskin-max


static int mtktsvtskin_get_trip_type(
struct thermal_zone_device *thermal, int trip, enum thermal_trip_type *type)
{
	*type = g_THERMAL_TRIP[trip];
	return 0;
}

static int mtktsvtskinget_trip_temp(
struct thermal_zone_device *thermal, int trip, int *temp)
{
	*temp = trip_temp[trip];
	return 0;
}

//vtskin1

static int mtktsvtskin1_get_trip_type(
struct thermal_zone_device *thermal, int trip, enum thermal_trip_type *type)
{
	*type = g_THERMAL_TRIP1[trip];
	return 0;
}

static int mtktsvtskin1_get_trip_temp(
struct thermal_zone_device *thermal, int trip, int *temp)
{
	*temp = trip_temp1[trip];
	return 0;
}



//vtskin2

static int mtktsvtskin2_get_trip_type(
struct thermal_zone_device *thermal, int trip, enum thermal_trip_type *type)
{
	*type = g_THERMAL_TRIP2[trip];
	return 0;
}

static int mtktsvtskin2_get_trip_temp(
struct thermal_zone_device *thermal, int trip, int *temp)
{
	*temp = trip_temp2[trip];
	return 0;
}



//vtskin3

static int mtktsvtskin3_get_trip_type(
struct thermal_zone_device *thermal, int trip, enum thermal_trip_type *type)
{
	*type = g_THERMAL_TRIP3[trip];
	return 0;
}

static int mtktsvtskin3_get_trip_temp(
struct thermal_zone_device *thermal, int trip, int *temp)
{
	*temp = trip_temp3[trip];
	return 0;
}


static struct thermal_zone_device_ops mtktsvtskin3_dev_ops = {
	.bind = mtktsvtskin3_bind,
	.unbind = mtktsvtskin3_unbind,
	.get_temp = mtktsvtskin_get_temp,
	.get_mode = mtktsvtskin_get_mode,
	.set_mode = mtktsvtskin_set_mode,
	.get_trip_type = mtktsvtskin3_get_trip_type,
	.get_trip_temp = mtktsvtskin3_get_trip_temp,
	.get_crit_temp = mtktsvtskin_get_crit_temp,
};



static struct thermal_zone_device_ops mtktsvtskin2_dev_ops = {
	.bind = mtktsvtskin2_bind,
	.unbind = mtktsvtskin2_unbind,
	.get_temp = mtktsvtskin_get_temp,
	.get_mode = mtktsvtskin_get_mode,
	.set_mode = mtktsvtskin_set_mode,
	.get_trip_type = mtktsvtskin2_get_trip_type,
	.get_trip_temp = mtktsvtskin2_get_trip_temp,
	.get_crit_temp = mtktsvtskin_get_crit_temp,
};


static struct thermal_zone_device_ops mtktsvtskin1_dev_ops = {
	.bind = mtktsvtskin1_bind,
	.unbind = mtktsvtskin1_unbind,
	.get_temp = mtktsvtskin_get_temp,
	.get_mode = mtktsvtskin_get_mode,
	.set_mode = mtktsvtskin_set_mode,
	.get_trip_type = mtktsvtskin1_get_trip_type,
	.get_trip_temp = mtktsvtskin1_get_trip_temp,
	.get_crit_temp = mtktsvtskin_get_crit_temp,
};


static struct thermal_zone_device_ops mtktsvtskin_max_dev_ops = {
	.bind = mtktsvtskin_bind,
	.unbind = mtktsvtskin_unbind,
	.get_temp = mtktsvtskin_get_temp,
	.get_mode = mtktsvtskin_get_mode,
	.set_mode = mtktsvtskin_set_mode,
	.get_trip_type = mtktsvtskin_get_trip_type,
	.get_trip_temp = mtktsvtskinget_trip_temp,
	.get_crit_temp = mtktsvtskin_get_crit_temp,
};



void mtktsvtskin_max_register_thermal(void)
{


	int i = VTSKIN_MAX_INDEX, ret;
	struct vtskin_data *skin_data = &mt6893_vtskin_data;
	struct thermal_zone_device *tzdev;
	struct device *dev = mt6893_vtskin_data.dev;




	tzdev= mtk_thermal_zone_device_register(mtk_thermal_get_vtksin_tz_name(i), num_trip,
							NULL, &mtktsvtskin_max_dev_ops,
							0, 0, 0, interval * 1000);

	if (IS_ERR(tzdev))
		pr_notice("Thermal zone register vtskin max fail");

	ret = snprintf(skin_data->params[i].tz_name, THERMAL_NAME_LENGTH, tzdev->type);
	if (ret < 0)
		pr_notice("copy tz_name fail %s\n", tzdev->type);



}


void mtktsvtskin1_register_thermal(void)
{


	int i = VTSKIN1_INDEX, ret;
	struct vtskin_data *skin_data = &mt6893_vtskin_data;
	struct thermal_zone_device *tzdev;
	struct device *dev = mt6893_vtskin_data.dev;




	tzdev= mtk_thermal_zone_device_register(mtk_thermal_get_vtksin_tz_name(i), num_trip1,
							NULL, &mtktsvtskin1_dev_ops,
							0, 0, 0, interval * 1000);

	if (IS_ERR(tzdev))
		pr_notice("Thermal zone register fail");

	ret = snprintf(skin_data->params[i].tz_name, THERMAL_NAME_LENGTH, tzdev->type);
	if (ret < 0)
		pr_notice("copy tz_name fail %s\n", tzdev->type);



}


void mtktsvtskin2_register_thermal(void)
{


	int i = VTSKIN2_INDEX , ret;
	struct vtskin_data *skin_data = &mt6893_vtskin_data;
	struct thermal_zone_device *tzdev;
	struct device *dev = mt6893_vtskin_data.dev;




	tzdev= mtk_thermal_zone_device_register(mtk_thermal_get_vtksin_tz_name(i), num_trip2,
							NULL, &mtktsvtskin2_dev_ops,
							0, 0, 0, interval * 1000);

	if (IS_ERR(tzdev))
		pr_notice("Thermal zone register fail");

	ret = snprintf(skin_data->params[i].tz_name, THERMAL_NAME_LENGTH, tzdev->type);
	if (ret < 0)
		pr_notice("copy tz_name fail %s\n", tzdev->type);



}


void mtktsvtskin3_register_thermal(void)
{


	int i = VTSKIN3_INDEX , ret;
	struct vtskin_data *skin_data = &mt6893_vtskin_data;
	struct thermal_zone_device *tzdev;
	struct device *dev = mt6893_vtskin_data.dev;

	tzdev= mtk_thermal_zone_device_register(mtk_thermal_get_vtksin_tz_name(i), num_trip3,
							NULL, &mtktsvtskin3_dev_ops,
							0, 0, 0, interval * 1000);

	if (IS_ERR(tzdev))
		pr_notice("Thermal zone register fail");

	ret = snprintf(skin_data->params[i].tz_name, THERMAL_NAME_LENGTH, tzdev->type);
	if (ret < 0)
		pr_notice("copy tz_name fail %s\n", tzdev->type);



}




static ssize_t mtktsvtskin3_write(
struct file *file, const char __user *buffer, size_t count, loff_t *data)
/* static ssize_t mtktsbattery_write(
 * struct file *file, const char *buffer, int count, void *data)
 */
{
	int len = 0, i;
	struct mtktsvtskin_data {
		int trip[10];
		int t_type[10];
	char bind0[20], bind1[20], bind2[20], bind3[20], bind4[20];
	char bind5[20], bind6[20], bind7[20], bind8[20], bind9[20];
		int time_msec;
	char desc[512];
	};


	struct mtktsvtskin_data *ptr_mtktsvtskin_data = kmalloc(
				sizeof(*ptr_mtktsvtskin_data), GFP_KERNEL);


	pr_notice("num_trip3 mtktsvtskin_write  [%d]\n",count);

	if (ptr_mtktsvtskin_data == NULL)
		return -ENOMEM;

	len = (count < (sizeof(ptr_mtktsvtskin_data->desc) - 1)) ?
			count : (sizeof(ptr_mtktsvtskin_data->desc) - 1);

	if (copy_from_user(ptr_mtktsvtskin_data->desc, buffer, len)) {
		kfree(ptr_mtktsvtskin_data);
		return 0;
	}

	ptr_mtktsvtskin_data->desc[len] = '\0';

	if (sscanf
	    (ptr_mtktsvtskin_data->desc,
	     "%d %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d",
		&num_trip3,
		&ptr_mtktsvtskin_data->trip[0],
		&ptr_mtktsvtskin_data->t_type[0],
		ptr_mtktsvtskin_data->bind0,
		&ptr_mtktsvtskin_data->trip[1],
		&ptr_mtktsvtskin_data->t_type[1],
		ptr_mtktsvtskin_data->bind1,
		&ptr_mtktsvtskin_data->trip[2],
		&ptr_mtktsvtskin_data->t_type[2],
		ptr_mtktsvtskin_data->bind2,
		&ptr_mtktsvtskin_data->trip[3],
		&ptr_mtktsvtskin_data->t_type[3],
		ptr_mtktsvtskin_data->bind3,
		&ptr_mtktsvtskin_data->trip[4],
		&ptr_mtktsvtskin_data->t_type[4],
		ptr_mtktsvtskin_data->bind4,
		&ptr_mtktsvtskin_data->trip[5],
		&ptr_mtktsvtskin_data->t_type[5],
		ptr_mtktsvtskin_data->bind5,
		&ptr_mtktsvtskin_data->trip[6],
		&ptr_mtktsvtskin_data->t_type[6],
		ptr_mtktsvtskin_data->bind6,
		&ptr_mtktsvtskin_data->trip[7],
		&ptr_mtktsvtskin_data->t_type[7],
		ptr_mtktsvtskin_data->bind7,
		&ptr_mtktsvtskin_data->trip[8],
		&ptr_mtktsvtskin_data->t_type[8],
		ptr_mtktsvtskin_data->bind8,
		&ptr_mtktsvtskin_data->trip[9],
		&ptr_mtktsvtskin_data->t_type[9],
		ptr_mtktsvtskin_data->bind9,
		&ptr_mtktsvtskin_data->time_msec) == 32) {

		down(&sem_mutex);
		mtktsvtskin_dprintk(
				"[%s] mtktsbattery_unregister_thermal\n",
				__func__);

		//mtktsbattery_unregister_thermal();
		pr_notice("num_trip3 mtktsvtskin_write [%d]\n",num_trip3);

		if (num_trip3 < 0 || num_trip3 > 10) {
			#ifdef CONFIG_MTK_AEE_FEATURE
			aee_kernel_warning_api(__FILE__, __LINE__,
					DB_OPT_DEFAULT, "mtktsbattery_write",
					"Bad argument");
			#endif
			mtktsvtskin_dprintk(
					"[%s] bad argument\n", __func__);

			kfree(ptr_mtktsvtskin_data);
			up(&sem_mutex);
			return -EINVAL;
		}

		for (i = 0; i < num_trip3; i++)
			g_THERMAL_TRIP3[i] = ptr_mtktsvtskin_data->t_type[i];

		g_bind30[0] = g_bind31[0] = g_bind32[0] = g_bind33[0]
			= g_bind34[0] = g_bind35[0] = g_bind36[0] = g_bind37[0]
			= g_bind38[0] = g_bind39[0] = '\0';

		for (i = 0; i < 20; i++) {
			g_bind30[i] = ptr_mtktsvtskin_data->bind0[i];
			g_bind31[i] = ptr_mtktsvtskin_data->bind1[i];
			g_bind32[i] = ptr_mtktsvtskin_data->bind2[i];
			g_bind33[i] = ptr_mtktsvtskin_data->bind3[i];
			g_bind34[i] = ptr_mtktsvtskin_data->bind4[i];
			g_bind35[i] = ptr_mtktsvtskin_data->bind5[i];
			g_bind36[i] = ptr_mtktsvtskin_data->bind6[i];
			g_bind37[i] = ptr_mtktsvtskin_data->bind7[i];
			g_bind38[i] = ptr_mtktsvtskin_data->bind8[i];
			g_bind39[i] = ptr_mtktsvtskin_data->bind9[i];
		}

		mtktsvtskin_dprintk(
			"[%s] g_THERMAL_TRIP_0=%d,g_THERMAL_TRIP_1=%d,",
			__func__,
			g_THERMAL_TRIP3[0], g_THERMAL_TRIP3[1]);

		mtktsvtskin_dprintk(
			"g_THERMAL_TRIP_2=%d,g_THERMAL_TRIP_3=%d,g_THERMAL_TRIP_4=%d,",
			g_THERMAL_TRIP3[2], g_THERMAL_TRIP3[3],
			g_THERMAL_TRIP3[4]);

		mtktsvtskin_dprintk(
			"g_THERMAL_TRIP_5=%d,g_THERMAL_TRIP_6=%d,g_THERMAL_TRIP_7=%d,",
			g_THERMAL_TRIP3[5], g_THERMAL_TRIP3[6],
			g_THERMAL_TRIP3[7]);

		mtktsvtskin_dprintk(
			"g_THERMAL_TRIP_8=%d,g_THERMAL_TRIP_9=%d,\n",
			g_THERMAL_TRIP3[8], g_THERMAL_TRIP3[9]);

		mtktsvtskin_dprintk(
			"[%s] cooldev0=%s,cooldev1=%s,cooldev2=%s,cooldev3=%s,",
			__func__,
			g_bind30, g_bind31, g_bind32, g_bind33);

		mtktsvtskin_dprintk(
			"cooldev4=%s,cooldev5=%s,cooldev6=%s,cooldev7=%s,cooldev8=%s,cooldev9=%s\n",
			g_bind34, g_bind35, g_bind36, g_bind37, g_bind38, g_bind39);

		for (i = 0; i < num_trip3; i++)
			trip_temp3[i] = ptr_mtktsvtskin_data->trip[i];

		interval = ptr_mtktsvtskin_data->time_msec / 1000;

		mtktsvtskin_dprintk(
			"[%s] trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,",
			__func__,
			trip_temp3[0], trip_temp3[1], trip_temp3[2]);

		mtktsvtskin_dprintk(
			"trip_3_temp=%d,trip_4_temp=%d,trip_5_temp=%d,trip_6_temp=%d,trip_7_temp=%d,",
			trip_temp3[3], trip_temp3[4], trip_temp3[5],
			trip_temp3[6], trip_temp3[7]);

		mtktsvtskin_dprintk(
			"trip_8_temp=%d,trip_9_temp=%d,time_ms=%d\n",
			trip_temp3[8], trip_temp3[9], interval * 1000);

		mtktsvtskin_dprintk(
			"[%s] mtktsbattery_register_thermal\n", __func__);

		mtktsvtskin3_register_thermal();

		up(&sem_mutex);

		kfree(ptr_mtktsvtskin_data);
		/* battery_write_flag=1; */
		return count;
	}

	mtktsvtskin_dprintk("[%s] bad argument\n", __func__);
    #ifdef CONFIG_MTK_AEE_FEATURE
	aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT,
					"mtktsbattery_write", "Bad argument");
    #endif
	kfree(ptr_mtktsvtskin_data);
	return -EINVAL;
}

static int mtktsvtskin3_read(struct seq_file *m, void *v)
/* static int mtktsbattery_read(
 * char *buf, char **start, off_t off, int count, int *eof, void *data)
 */
{

	seq_printf(m,
		"[mtktsvtskin_read3] trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,trip_3_temp=%d,\n",
		trip_temp3[0], trip_temp3[1], trip_temp3[2], trip_temp3[3]);

	seq_printf(m,
		"trip_4_temp=%d,trip_5_temp=%d,trip_6_temp=%d,trip_7_temp=%d,trip_8_temp=%d,trip_9_temp=%d,\n",
		trip_temp3[4], trip_temp3[5], trip_temp3[6],
		trip_temp3[7], trip_temp3[8], trip_temp3[9]);

	seq_printf(m,
		"g_THERMAL_TRIP_0=%d,g_THERMAL_TRIP_1=%d,g_THERMAL_TRIP_2=%d,g_THERMAL_TRIP_3=%d,",
		g_THERMAL_TRIP3[0], g_THERMAL_TRIP3[1],
		g_THERMAL_TRIP3[2], g_THERMAL_TRIP3[3]);

	seq_printf(m,
		"g_THERMAL_TRIP_4=%d,g_THERMAL_TRIP_5=%d,g_THERMAL_TRIP_6=%d,g_THERMAL_TRIP_7=%d,",
		g_THERMAL_TRIP3[4], g_THERMAL_TRIP3[5],
		g_THERMAL_TRIP3[6], g_THERMAL_TRIP3[7]);

	seq_printf(m,
		"g_THERMAL_TRIP_8=%d,g_THERMAL_TRIP_9=%d,\n",
		g_THERMAL_TRIP3[8], g_THERMAL_TRIP3[9]);


	seq_printf(m,
		"cooldev0=%s,cooldev1=%s,cooldev2=%s,cooldev3=%s,cooldev4=%s,\n",
		g_bind30, g_bind31, g_bind32, g_bind33, g_bind34);

	seq_printf(m,
		"cooldev5=%s,cooldev6=%s,cooldev7=%s,cooldev8=%s,cooldev9=%s,time_ms=%d\n",
		g_bind35, g_bind36, g_bind37, g_bind38, g_bind39, interval * 1000);

	return 0;
}



static ssize_t mtktsvtskin2_write(
struct file *file, const char __user *buffer, size_t count, loff_t *data)
/* static ssize_t mtktsbattery_write(
 * struct file *file, const char *buffer, int count, void *data)
 */
{
	int len = 0, i;
	struct mtktsvtskin_data {
		int trip[10];
		int t_type[10];
	char bind0[20], bind1[20], bind2[20], bind3[20], bind4[20];
	char bind5[20], bind6[20], bind7[20], bind8[20], bind9[20];
		int time_msec;
	char desc[512];
	};


	struct mtktsvtskin_data *ptr_mtktsvtskin_data = kmalloc(
				sizeof(*ptr_mtktsvtskin_data), GFP_KERNEL);


	pr_notice("num_trip2 mtktsvtskin_write  [%d]\n",count);

	if (ptr_mtktsvtskin_data == NULL)
		return -ENOMEM;

	len = (count < (sizeof(ptr_mtktsvtskin_data->desc) - 1)) ?
			count : (sizeof(ptr_mtktsvtskin_data->desc) - 1);

	if (copy_from_user(ptr_mtktsvtskin_data->desc, buffer, len)) {
		kfree(ptr_mtktsvtskin_data);
		return 0;
	}

	ptr_mtktsvtskin_data->desc[len] = '\0';

	if (sscanf
	    (ptr_mtktsvtskin_data->desc,
	     "%d %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d",
		&num_trip2,
		&ptr_mtktsvtskin_data->trip[0],
		&ptr_mtktsvtskin_data->t_type[0],
		ptr_mtktsvtskin_data->bind0,
		&ptr_mtktsvtskin_data->trip[1],
		&ptr_mtktsvtskin_data->t_type[1],
		ptr_mtktsvtskin_data->bind1,
		&ptr_mtktsvtskin_data->trip[2],
		&ptr_mtktsvtskin_data->t_type[2],
		ptr_mtktsvtskin_data->bind2,
		&ptr_mtktsvtskin_data->trip[3],
		&ptr_mtktsvtskin_data->t_type[3],
		ptr_mtktsvtskin_data->bind3,
		&ptr_mtktsvtskin_data->trip[4],
		&ptr_mtktsvtskin_data->t_type[4],
		ptr_mtktsvtskin_data->bind4,
		&ptr_mtktsvtskin_data->trip[5],
		&ptr_mtktsvtskin_data->t_type[5],
		ptr_mtktsvtskin_data->bind5,
		&ptr_mtktsvtskin_data->trip[6],
		&ptr_mtktsvtskin_data->t_type[6],
		ptr_mtktsvtskin_data->bind6,
		&ptr_mtktsvtskin_data->trip[7],
		&ptr_mtktsvtskin_data->t_type[7],
		ptr_mtktsvtskin_data->bind7,
		&ptr_mtktsvtskin_data->trip[8],
		&ptr_mtktsvtskin_data->t_type[8],
		ptr_mtktsvtskin_data->bind8,
		&ptr_mtktsvtskin_data->trip[9],
		&ptr_mtktsvtskin_data->t_type[9],
		ptr_mtktsvtskin_data->bind9,
		&ptr_mtktsvtskin_data->time_msec) == 32) {

		down(&sem_mutex);
		mtktsvtskin_dprintk(
				"[%s] mtktsbattery_unregister_thermal\n",
				__func__);

		//mtktsbattery_unregister_thermal();
		pr_notice("num_trip2 mtktsvtskin_write [%d]\n",num_trip2);

		if (num_trip2 < 0 || num_trip2 > 10) {
			#ifdef CONFIG_MTK_AEE_FEATURE
			aee_kernel_warning_api(__FILE__, __LINE__,
					DB_OPT_DEFAULT, "mtktsbattery_write",
					"Bad argument");
			#endif
			mtktsvtskin_dprintk(
					"[%s] bad argument\n", __func__);

			kfree(ptr_mtktsvtskin_data);
			up(&sem_mutex);
			return -EINVAL;
		}

		for (i = 0; i < num_trip2; i++)
			g_THERMAL_TRIP2[i] = ptr_mtktsvtskin_data->t_type[i];

		g_bind20[0] = g_bind21[0] = g_bind22[0] = g_bind23[0]
			= g_bind24[0] = g_bind25[0] = g_bind26[0] = g_bind27[0]
			= g_bind28[0] = g_bind29[0] = '\0';

		for (i = 0; i < 20; i++) {
			g_bind20[i] = ptr_mtktsvtskin_data->bind0[i];
			g_bind21[i] = ptr_mtktsvtskin_data->bind1[i];
			g_bind22[i] = ptr_mtktsvtskin_data->bind2[i];
			g_bind23[i] = ptr_mtktsvtskin_data->bind3[i];
			g_bind24[i] = ptr_mtktsvtskin_data->bind4[i];
			g_bind25[i] = ptr_mtktsvtskin_data->bind5[i];
			g_bind26[i] = ptr_mtktsvtskin_data->bind6[i];
			g_bind27[i] = ptr_mtktsvtskin_data->bind7[i];
			g_bind28[i] = ptr_mtktsvtskin_data->bind8[i];
			g_bind29[i] = ptr_mtktsvtskin_data->bind9[i];
		}

		mtktsvtskin_dprintk(
			"[%s] g_THERMAL_TRIP_0=%d,g_THERMAL_TRIP_1=%d,",
			__func__,
			g_THERMAL_TRIP2[0], g_THERMAL_TRIP2[1]);

		mtktsvtskin_dprintk(
			"g_THERMAL_TRIP_2=%d,g_THERMAL_TRIP_3=%d,g_THERMAL_TRIP_4=%d,",
			g_THERMAL_TRIP2[2], g_THERMAL_TRIP2[3],
			g_THERMAL_TRIP2[4]);

		mtktsvtskin_dprintk(
			"g_THERMAL_TRIP_5=%d,g_THERMAL_TRIP_6=%d,g_THERMAL_TRIP_7=%d,",
			g_THERMAL_TRIP2[5], g_THERMAL_TRIP2[6],
			g_THERMAL_TRIP2[7]);

		mtktsvtskin_dprintk(
			"g_THERMAL_TRIP_8=%d,g_THERMAL_TRIP_9=%d,\n",
			g_THERMAL_TRIP2[8], g_THERMAL_TRIP2[9]);

		mtktsvtskin_dprintk(
			"[%s] cooldev0=%s,cooldev1=%s,cooldev2=%s,cooldev3=%s,",
			__func__,
			g_bind20, g_bind21, g_bind22, g_bind23);

		mtktsvtskin_dprintk(
			"cooldev4=%s,cooldev5=%s,cooldev6=%s,cooldev7=%s,cooldev8=%s,cooldev9=%s\n",
			g_bind24, g_bind25, g_bind26, g_bind27, g_bind28, g_bind29);

		for (i = 0; i < num_trip2; i++)
			trip_temp2[i] = ptr_mtktsvtskin_data->trip[i];

		interval = ptr_mtktsvtskin_data->time_msec / 1000;

		mtktsvtskin_dprintk(
			"[%s] trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,",
			__func__,
			trip_temp2[0], trip_temp2[1], trip_temp2[2]);

		mtktsvtskin_dprintk(
			"trip_3_temp=%d,trip_4_temp=%d,trip_5_temp=%d,trip_6_temp=%d,trip_7_temp=%d,",
			trip_temp2[3], trip_temp2[4], trip_temp2[5],
			trip_temp2[6], trip_temp2[7]);

		mtktsvtskin_dprintk(
			"trip_8_temp=%d,trip_9_temp=%d,time_ms=%d\n",
			trip_temp2[8], trip_temp2[9], interval * 1000);

		mtktsvtskin_dprintk(
			"[%s] mtktsbattery_register_thermal\n", __func__);

		mtktsvtskin2_register_thermal();

		up(&sem_mutex);

		kfree(ptr_mtktsvtskin_data);
		/* battery_write_flag=1; */
		return count;
	}

	mtktsvtskin_dprintk("[%s] bad argument\n", __func__);
    #ifdef CONFIG_MTK_AEE_FEATURE
	aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT,
					"mtktsbattery_write", "Bad argument");
    #endif
	kfree(ptr_mtktsvtskin_data);
	return -EINVAL;
}

static int mtktsvtskin2_read(struct seq_file *m, void *v)
/* static int mtktsbattery_read(
 * char *buf, char **start, off_t off, int count, int *eof, void *data)
 */
{

	seq_printf(m,
		"[mtktsvtskin_read2] trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,trip_3_temp=%d,\n",
		trip_temp2[0], trip_temp2[1], trip_temp2[2], trip_temp2[3]);

	seq_printf(m,
		"trip_4_temp=%d,trip_5_temp=%d,trip_6_temp=%d,trip_7_temp=%d,trip_8_temp=%d,trip_9_temp=%d,\n",
		trip_temp2[4], trip_temp2[5], trip_temp2[6],
		trip_temp2[7], trip_temp2[8], trip_temp2[9]);

	seq_printf(m,
		"g_THERMAL_TRIP_0=%d,g_THERMAL_TRIP_1=%d,g_THERMAL_TRIP_2=%d,g_THERMAL_TRIP_3=%d,",
		g_THERMAL_TRIP2[0], g_THERMAL_TRIP2[1],
		g_THERMAL_TRIP2[2], g_THERMAL_TRIP2[3]);

	seq_printf(m,
		"g_THERMAL_TRIP_4=%d,g_THERMAL_TRIP_5=%d,g_THERMAL_TRIP_6=%d,g_THERMAL_TRIP_7=%d,",
		g_THERMAL_TRIP2[4], g_THERMAL_TRIP2[5],
		g_THERMAL_TRIP2[6], g_THERMAL_TRIP2[7]);

	seq_printf(m,
		"g_THERMAL_TRIP_8=%d,g_THERMAL_TRIP_9=%d,\n",
		g_THERMAL_TRIP2[8], g_THERMAL_TRIP2[9]);


	seq_printf(m,
		"cooldev0=%s,cooldev1=%s,cooldev2=%s,cooldev3=%s,cooldev4=%s,\n",
		g_bind20, g_bind21, g_bind22, g_bind23, g_bind24);

	seq_printf(m,
		"cooldev5=%s,cooldev6=%s,cooldev7=%s,cooldev8=%s,cooldev9=%s,time_ms=%d\n",
		g_bind25, g_bind26, g_bind27, g_bind28, g_bind29, interval * 1000);

	return 0;
}


static ssize_t mtktsvtskin1_write(
struct file *file, const char __user *buffer, size_t count, loff_t *data)
/* static ssize_t mtktsbattery_write(
 * struct file *file, const char *buffer, int count, void *data)
 */
{
	int len = 0, i;
	struct mtktsvtskin_data {
		int trip[10];
		int t_type[10];
	char bind0[20], bind1[20], bind2[20], bind3[20], bind4[20];
	char bind5[20], bind6[20], bind7[20], bind8[20], bind9[20];
		int time_msec;
	char desc[512];
	};





	struct mtktsvtskin_data *ptr_mtktsvtskin_data = kmalloc(
				sizeof(*ptr_mtktsvtskin_data), GFP_KERNEL);


	pr_notice("num_trip1 mtktsvtskin_write[%d]\n",count);

	if (ptr_mtktsvtskin_data == NULL)
		return -ENOMEM;

	len = (count < (sizeof(ptr_mtktsvtskin_data->desc) - 1)) ?
			count : (sizeof(ptr_mtktsvtskin_data->desc) - 1);

	if (copy_from_user(ptr_mtktsvtskin_data->desc, buffer, len)) {
		kfree(ptr_mtktsvtskin_data);
		return 0;
	}

	ptr_mtktsvtskin_data->desc[len] = '\0';

	if (sscanf
	    (ptr_mtktsvtskin_data->desc,
	     "%d %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d",
		&num_trip1,
		&ptr_mtktsvtskin_data->trip[0],
		&ptr_mtktsvtskin_data->t_type[0],
		ptr_mtktsvtskin_data->bind0,
		&ptr_mtktsvtskin_data->trip[1],
		&ptr_mtktsvtskin_data->t_type[1],
		ptr_mtktsvtskin_data->bind1,
		&ptr_mtktsvtskin_data->trip[2],
		&ptr_mtktsvtskin_data->t_type[2],
		ptr_mtktsvtskin_data->bind2,
		&ptr_mtktsvtskin_data->trip[3],
		&ptr_mtktsvtskin_data->t_type[3],
		ptr_mtktsvtskin_data->bind3,
		&ptr_mtktsvtskin_data->trip[4],
		&ptr_mtktsvtskin_data->t_type[4],
		ptr_mtktsvtskin_data->bind4,
		&ptr_mtktsvtskin_data->trip[5],
		&ptr_mtktsvtskin_data->t_type[5],
		ptr_mtktsvtskin_data->bind5,
		&ptr_mtktsvtskin_data->trip[6],
		&ptr_mtktsvtskin_data->t_type[6],
		ptr_mtktsvtskin_data->bind6,
		&ptr_mtktsvtskin_data->trip[7],
		&ptr_mtktsvtskin_data->t_type[7],
		ptr_mtktsvtskin_data->bind7,
		&ptr_mtktsvtskin_data->trip[8],
		&ptr_mtktsvtskin_data->t_type[8],
		ptr_mtktsvtskin_data->bind8,
		&ptr_mtktsvtskin_data->trip[9],
		&ptr_mtktsvtskin_data->t_type[9],
		ptr_mtktsvtskin_data->bind9,
		&ptr_mtktsvtskin_data->time_msec) == 32) {

		down(&sem_mutex);
		mtktsvtskin_dprintk(
				"[%s] mtktsbattery_unregister_thermal\n",__func__);

		//mtktsbattery_unregister_thermal();
		pr_notice("num_trip1 mtktsvtskin_write [%d]\n",num_trip1);

		if (num_trip1 < 0 || num_trip1 > 10) {
			#ifdef CONFIG_MTK_AEE_FEATURE
			aee_kernel_warning_api(__FILE__, __LINE__,
					DB_OPT_DEFAULT, "mtktsbattery_write",
					"Bad argument");
			#endif
			mtktsvtskin_dprintk(
					"[%s] bad argument\n", __func__);

			kfree(ptr_mtktsvtskin_data);
			up(&sem_mutex);
			return -EINVAL;
		}

		for (i = 0; i < num_trip1; i++)
			g_THERMAL_TRIP1[i] = ptr_mtktsvtskin_data->t_type[i];

		g_bind10[0] = g_bind11[0] = g_bind12[0] = g_bind13[0]
			= g_bind14[0] = g_bind15[0] = g_bind16[0] = g_bind17[0]
			= g_bind18[0] = g_bind19[0] = '\0';

		for (i = 0; i < 20; i++) {
			g_bind10[i] = ptr_mtktsvtskin_data->bind0[i];
			g_bind11[i] = ptr_mtktsvtskin_data->bind1[i];
			g_bind12[i] = ptr_mtktsvtskin_data->bind2[i];
			g_bind13[i] = ptr_mtktsvtskin_data->bind3[i];
			g_bind14[i] = ptr_mtktsvtskin_data->bind4[i];
			g_bind15[i] = ptr_mtktsvtskin_data->bind5[i];
			g_bind16[i] = ptr_mtktsvtskin_data->bind6[i];
			g_bind17[i] = ptr_mtktsvtskin_data->bind7[i];
			g_bind18[i] = ptr_mtktsvtskin_data->bind8[i];
			g_bind19[i] = ptr_mtktsvtskin_data->bind9[i];
		}

		mtktsvtskin_dprintk(
			"[%s] g_THERMAL_TRIP_0=%d,g_THERMAL_TRIP_1=%d,",
			__func__,
			g_THERMAL_TRIP1[0], g_THERMAL_TRIP1[1]);

		mtktsvtskin_dprintk(
			"g_THERMAL_TRIP_2=%d,g_THERMAL_TRIP_3=%d,g_THERMAL_TRIP_4=%d,",
			g_THERMAL_TRIP1[2], g_THERMAL_TRIP1[3],
			g_THERMAL_TRIP1[4]);

		mtktsvtskin_dprintk(
			"g_THERMAL_TRIP_5=%d,g_THERMAL_TRIP_6=%d,g_THERMAL_TRIP_7=%d,",
			g_THERMAL_TRIP1[5], g_THERMAL_TRIP1[6],
			g_THERMAL_TRIP1[7]);

		mtktsvtskin_dprintk(
			"g_THERMAL_TRIP_8=%d,g_THERMAL_TRIP_9=%d,\n",
			g_THERMAL_TRIP1[8], g_THERMAL_TRIP1[9]);

		mtktsvtskin_dprintk(
			"[%s] cooldev0=%s,cooldev1=%s,cooldev2=%s,cooldev3=%s,",
			__func__,
			g_bind10, g_bind11, g_bind12, g_bind13);

		mtktsvtskin_dprintk(
			"cooldev4=%s,cooldev5=%s,cooldev6=%s,cooldev7=%s,cooldev8=%s,cooldev9=%s\n",
			g_bind14, g_bind15, g_bind16, g_bind17, g_bind18, g_bind19);

		for (i = 0; i < num_trip1; i++)
			trip_temp1[i] = ptr_mtktsvtskin_data->trip[i];

		interval = ptr_mtktsvtskin_data->time_msec / 1000;

		mtktsvtskin_dprintk(
			"[%s] trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,",
			__func__,
			trip_temp1[0], trip_temp1[1], trip_temp1[2]);

		mtktsvtskin_dprintk(
			"trip_3_temp=%d,trip_4_temp=%d,trip_5_temp=%d,trip_6_temp=%d,trip_7_temp=%d,",
			trip_temp1[3], trip_temp1[4], trip_temp1[5],
			trip_temp1[6], trip_temp1[7]);

		mtktsvtskin_dprintk(
			"trip_8_temp=%d,trip_9_temp=%d,time_ms=%d\n",
			trip_temp1[8], trip_temp1[9], interval * 1000);

		mtktsvtskin_dprintk(
			"[%s] mtktsbattery_register_thermal\n", __func__);

		//mtktsbattery_register_thermal();

		mtktsvtskin1_register_thermal();
		up(&sem_mutex);

		kfree(ptr_mtktsvtskin_data);
		/* battery_write_flag=1; */
		return count;
	}

	mtktsvtskin_dprintk("[%s] bad argument\n", __func__);
    #ifdef CONFIG_MTK_AEE_FEATURE
	aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT,
					"mtktsbattery_write", "Bad argument");
    #endif
	kfree(ptr_mtktsvtskin_data);
	return -EINVAL;
}

static int mtktsvtskin1_read(struct seq_file *m, void *v)
/* static int mtktsbattery_read(
 * char *buf, char **start, off_t off, int count, int *eof, void *data)
 */
{

	seq_printf(m,
		"[mtktsvtskin_read] trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,trip_3_temp=%d,\n",
		trip_temp1[0], trip_temp1[1], trip_temp1[2], trip_temp1[3]);

	seq_printf(m,
		"trip_4_temp=%d,trip_5_temp=%d,trip_6_temp=%d,trip_7_temp=%d,trip_8_temp=%d,trip_9_temp=%d,\n",
		trip_temp1[4], trip_temp1[5], trip_temp1[6],
		trip_temp1[7], trip_temp1[8], trip_temp1[9]);

	seq_printf(m,
		"g_THERMAL_TRIP_0=%d,g_THERMAL_TRIP_1=%d,g_THERMAL_TRIP_2=%d,g_THERMAL_TRIP_3=%d,",
		g_THERMAL_TRIP1[0], g_THERMAL_TRIP1[1],
		g_THERMAL_TRIP1[2], g_THERMAL_TRIP1[3]);

	seq_printf(m,
		"g_THERMAL_TRIP_4=%d,g_THERMAL_TRIP_5=%d,g_THERMAL_TRIP_6=%d,g_THERMAL_TRIP_7=%d,",
		g_THERMAL_TRIP1[4], g_THERMAL_TRIP1[5],
		g_THERMAL_TRIP1[6], g_THERMAL_TRIP1[7]);

	seq_printf(m,
		"g_THERMAL_TRIP_8=%d,g_THERMAL_TRIP_9=%d,\n",
		g_THERMAL_TRIP1[8], g_THERMAL_TRIP1[9]);


	seq_printf(m,
		"cooldev0=%s,cooldev1=%s,cooldev2=%s,cooldev3=%s,cooldev4=%s,\n",
		g_bind10, g_bind11, g_bind12, g_bind13, g_bind14);

	seq_printf(m,
		"cooldev5=%s,cooldev6=%s,cooldev7=%s,cooldev8=%s,cooldev9=%s,time_ms=%d\n",
		g_bind15, g_bind16, g_bind17, g_bind18, g_bind19, interval * 1000);

	return 0;
}


static ssize_t mtktsvtskin0_write(
struct file *file, const char __user *buffer, size_t count, loff_t *data)
/* static ssize_t mtktsbattery_write(
 * struct file *file, const char *buffer, int count, void *data)
 */
{
	int len = 0, i;
	struct mtktsvtskin_data {
		int trip[10];
		int t_type[10];
	char bind0[20], bind1[20], bind2[20], bind3[20], bind4[20];
	char bind5[20], bind6[20], bind7[20], bind8[20], bind9[20];
		int time_msec;
	char desc[512];
	};


	struct mtktsvtskin_data *ptr_mtktsvtskin_data = kmalloc(
				sizeof(*ptr_mtktsvtskin_data), GFP_KERNEL);

	pr_notice("num_trip mtktsvtskin_write  [%d]\n",count);

	if (ptr_mtktsvtskin_data == NULL)
		return -ENOMEM;

	len = (count < (sizeof(ptr_mtktsvtskin_data->desc) - 1)) ?
			count : (sizeof(ptr_mtktsvtskin_data->desc) - 1);

	if (copy_from_user(ptr_mtktsvtskin_data->desc, buffer, len)) {
		kfree(ptr_mtktsvtskin_data);
		return 0;
	}

	ptr_mtktsvtskin_data->desc[len] = '\0';

	if (sscanf
	    (ptr_mtktsvtskin_data->desc,
	     "%d %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d %d %19s %d",
		&num_trip,
		&ptr_mtktsvtskin_data->trip[0],
		&ptr_mtktsvtskin_data->t_type[0],
		ptr_mtktsvtskin_data->bind0,
		&ptr_mtktsvtskin_data->trip[1],
		&ptr_mtktsvtskin_data->t_type[1],
		ptr_mtktsvtskin_data->bind1,
		&ptr_mtktsvtskin_data->trip[2],
		&ptr_mtktsvtskin_data->t_type[2],
		ptr_mtktsvtskin_data->bind2,
		&ptr_mtktsvtskin_data->trip[3],
		&ptr_mtktsvtskin_data->t_type[3],
		ptr_mtktsvtskin_data->bind3,
		&ptr_mtktsvtskin_data->trip[4],
		&ptr_mtktsvtskin_data->t_type[4],
		ptr_mtktsvtskin_data->bind4,
		&ptr_mtktsvtskin_data->trip[5],
		&ptr_mtktsvtskin_data->t_type[5],
		ptr_mtktsvtskin_data->bind5,
		&ptr_mtktsvtskin_data->trip[6],
		&ptr_mtktsvtskin_data->t_type[6],
		ptr_mtktsvtskin_data->bind6,
		&ptr_mtktsvtskin_data->trip[7],
		&ptr_mtktsvtskin_data->t_type[7],
		ptr_mtktsvtskin_data->bind7,
		&ptr_mtktsvtskin_data->trip[8],
		&ptr_mtktsvtskin_data->t_type[8],
		ptr_mtktsvtskin_data->bind8,
		&ptr_mtktsvtskin_data->trip[9],
		&ptr_mtktsvtskin_data->t_type[9],
		ptr_mtktsvtskin_data->bind9,
		&ptr_mtktsvtskin_data->time_msec) == 32) {

		down(&sem_mutex);
		mtktsvtskin_dprintk(
				"[%s] mtktsbattery_unregister_thermal\n",__func__);

		//mtktsbattery_unregister_thermal();
		pr_notice("num_trip mtktsvtskin_write[%d]\n", num_trip);

		if (num_trip < 0 || num_trip > 10) {
			#ifdef CONFIG_MTK_AEE_FEATURE
			aee_kernel_warning_api(__FILE__, __LINE__,
					DB_OPT_DEFAULT, "mtktsbattery_write",
					"Bad argument");
			#endif
			mtktsvtskin_dprintk(
					"[%s] bad argument\n", __func__);

			kfree(ptr_mtktsvtskin_data);
			up(&sem_mutex);
			return -EINVAL;
		}

		for (i = 0; i < num_trip; i++)
			g_THERMAL_TRIP[i] = ptr_mtktsvtskin_data->t_type[i];

		g_bind0[0] = g_bind1[0] = g_bind2[0] = g_bind3[0]
			= g_bind4[0] = g_bind5[0] = g_bind6[0] = g_bind7[0]
			= g_bind8[0] = g_bind9[0] = '\0';

		for (i = 0; i < 20; i++) {
			g_bind0[i] = ptr_mtktsvtskin_data->bind0[i];
			g_bind1[i] = ptr_mtktsvtskin_data->bind1[i];
			g_bind2[i] = ptr_mtktsvtskin_data->bind2[i];
			g_bind3[i] = ptr_mtktsvtskin_data->bind3[i];
			g_bind4[i] = ptr_mtktsvtskin_data->bind4[i];
			g_bind5[i] = ptr_mtktsvtskin_data->bind5[i];
			g_bind6[i] = ptr_mtktsvtskin_data->bind6[i];
			g_bind7[i] = ptr_mtktsvtskin_data->bind7[i];
			g_bind8[i] = ptr_mtktsvtskin_data->bind8[i];
			g_bind9[i] = ptr_mtktsvtskin_data->bind9[i];
		}

		mtktsvtskin_dprintk(
			"[%s] g_THERMAL_TRIP_0=%d,g_THERMAL_TRIP_1=%d,",
			__func__,
			g_THERMAL_TRIP[0], g_THERMAL_TRIP[1]);

		mtktsvtskin_dprintk(
			"g_THERMAL_TRIP_2=%d,g_THERMAL_TRIP_3=%d,g_THERMAL_TRIP_4=%d,",
			g_THERMAL_TRIP[2], g_THERMAL_TRIP[3],
			g_THERMAL_TRIP[4]);

		mtktsvtskin_dprintk(
			"g_THERMAL_TRIP_5=%d,g_THERMAL_TRIP_6=%d,g_THERMAL_TRIP_7=%d,",
			g_THERMAL_TRIP[5], g_THERMAL_TRIP[6],
			g_THERMAL_TRIP[7]);

		mtktsvtskin_dprintk(
			"g_THERMAL_TRIP_8=%d,g_THERMAL_TRIP_9=%d,\n",
			g_THERMAL_TRIP[8], g_THERMAL_TRIP[9]);

		mtktsvtskin_dprintk(
			"[%s] cooldev0=%s,cooldev1=%s,cooldev2=%s,cooldev3=%s,",
			__func__,
			g_bind0, g_bind1, g_bind2, g_bind3);

		mtktsvtskin_dprintk(
			"cooldev4=%s,cooldev5=%s,cooldev6=%s,cooldev7=%s,cooldev8=%s,cooldev9=%s\n",
			g_bind4, g_bind5, g_bind6, g_bind7, g_bind8, g_bind9);

		for (i = 0; i < num_trip; i++)
			trip_temp[i] = ptr_mtktsvtskin_data->trip[i];

		interval = ptr_mtktsvtskin_data->time_msec / 1000;

		mtktsvtskin_dprintk(
			"[%s] trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,",
			__func__,
			trip_temp[0], trip_temp[1], trip_temp[2]);

		mtktsvtskin_dprintk(
			"trip_3_temp=%d,trip_4_temp=%d,trip_5_temp=%d,trip_6_temp=%d,trip_7_temp=%d,",
			trip_temp[3], trip_temp[4], trip_temp[5],
			trip_temp[6], trip_temp[7]);

		mtktsvtskin_dprintk(
			"trip_8_temp=%d,trip_9_temp=%d,time_ms=%d\n",
			trip_temp[8], trip_temp[9], interval * 1000);

		mtktsvtskin_dprintk(
			"[%s] mtktsbattery_register_thermal\n", __func__);

		mtktsvtskin_max_register_thermal();
		up(&sem_mutex);

		kfree(ptr_mtktsvtskin_data);
		/* battery_write_flag=1; */
		return count;
	}

	mtktsvtskin_dprintk("[%s] bad argument\n", __func__);
    #ifdef CONFIG_MTK_AEE_FEATURE
	aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT,
					"mtktsbattery_write", "Bad argument");
    #endif
	kfree(ptr_mtktsvtskin_data);
	return -EINVAL;
}

static int mtktsvtskin0_read(struct seq_file *m, void *v)
/* static int mtktsbattery_read(
 * char *buf, char **start, off_t off, int count, int *eof, void *data)
 */
{

	seq_printf(m,
		"[mtktsvtskin_read] trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,trip_3_temp=%d,\n",
		trip_temp[0], trip_temp[1], trip_temp[2], trip_temp[3]);

	seq_printf(m,
		"trip_4_temp=%d,trip_5_temp=%d,trip_6_temp=%d,trip_7_temp=%d,trip_8_temp=%d,trip_9_temp=%d,\n",
		trip_temp[4], trip_temp[5], trip_temp[6],
		trip_temp[7], trip_temp[8], trip_temp[9]);

	seq_printf(m,
		"g_THERMAL_TRIP_0=%d,g_THERMAL_TRIP_1=%d,g_THERMAL_TRIP_2=%d,g_THERMAL_TRIP_3=%d,",
		g_THERMAL_TRIP[0], g_THERMAL_TRIP[1],
		g_THERMAL_TRIP[2], g_THERMAL_TRIP[3]);

	seq_printf(m,
		"g_THERMAL_TRIP_4=%d,g_THERMAL_TRIP_5=%d,g_THERMAL_TRIP_6=%d,g_THERMAL_TRIP_7=%d,",
		g_THERMAL_TRIP[4], g_THERMAL_TRIP[5],
		g_THERMAL_TRIP[6], g_THERMAL_TRIP[7]);

	seq_printf(m,
		"g_THERMAL_TRIP_8=%d,g_THERMAL_TRIP_9=%d,\n",
		g_THERMAL_TRIP[8], g_THERMAL_TRIP[9]);


	seq_printf(m,
		"cooldev0=%s,cooldev1=%s,cooldev2=%s,cooldev3=%s,cooldev4=%s,\n",
		g_bind0, g_bind1, g_bind2, g_bind3, g_bind4);

	seq_printf(m,
		"cooldev5=%s,cooldev6=%s,cooldev7=%s,cooldev8=%s,cooldev9=%s,time_ms=%d\n",
		g_bind5, g_bind6, g_bind7, g_bind8, g_bind9, interval * 1000);

	return 0;
}


static int mtkts_vtskin_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtktsvtskin0_read, NULL);
}


static int mtkts_vtskin1_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtktsvtskin1_read, NULL);
}

static int mtkts_vtskin2_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtktsvtskin2_read, NULL);
}


static int mtkts_vtskin3_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtktsvtskin3_read, NULL);
}







static const struct file_operations mtkts_vtskin_max_fops = {
	.owner = THIS_MODULE,
	.open = mtkts_vtskin_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = mtktsvtskin0_write,
	.release = single_release,
};


static const struct file_operations mtkts_vtskin1_fops = {
	.owner = THIS_MODULE,
	.open = mtkts_vtskin1_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = mtktsvtskin1_write,
	.release = single_release,
};

static const struct file_operations mtkts_vtskin2_fops = {
	.owner = THIS_MODULE,
	.open = mtkts_vtskin2_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = mtktsvtskin2_write,
	.release = single_release,
};

static const struct file_operations mtkts_vtskin3_fops = {
	.owner = THIS_MODULE,
	.open = mtkts_vtskin3_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = mtktsvtskin3_write,
	.release = single_release,
};


static int vtskin_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	//struct vtskin_temp_tz *skin_tz;
	struct vtskin_data *skin_data;
	//struct thermal_zone_device *tzdev;
	struct proc_dir_entry *mtktsvtskin_dir = NULL;
	struct proc_dir_entry *entry = NULL;
	int i; //ret;

	if (!pdev->dev.of_node) {
		pr_notice("Only DT based supported\n");
		return -ENODEV;
	}
	pr_notice(" vtskin probe called");

	skin_data = parse_dt(pdev);
	if (!skin_data)
		pr_notice("Error: Failed to get lvts platform data\n");

	skin_data->dev = dev;
	platform_set_drvdata(pdev, skin_data);

	for (i = 0; i < skin_data->num_sensor; i++)
		mtktsvtskin_register_cooler(i);

	mtktsvtskin_dir = mtk_thermal_get_proc_drv_therm_dir_entry();
	if (!mtktsvtskin_dir) {
		pr_notice("%s mkdir /proc/driver/thermal failed\n",__func__);
		return -1;
	}

	entry = proc_create(mtk_thermal_get_vtksin_tz_proc_name(VTSKIN_MAX_INDEX),
		0664, mtktsvtskin_dir, &mtkts_vtskin_max_fops);
	if (entry)
		proc_set_user(entry, uid, gid);
	}

	entry = proc_create(mtk_thermal_get_vtksin_tz_proc_name(VTSKIN1_INDEX),
			0664, mtktsvtskin_dir, &mtkts_vtskin1_fops);
	if (entry)
		proc_set_user(entry, uid, gid);

	entry = proc_create(mtk_thermal_get_vtksin_tz_proc_name(VTSKIN2_INDEX),
			0664, mtktsvtskin_dir, &mtkts_vtskin2_fops);
	if (entry)
		proc_set_user(entry, uid, gid);

	entry = proc_create(mtk_thermal_get_vtksin_tz_proc_name(VTSKIN3_INDEX),
			0664, mtktsvtskin_dir, &mtkts_vtskin3_fops);
	if (entry)
		proc_set_user(entry, uid, gid);




	pr_notice("vtkin probe  completed");
	return 0;
}

static const struct of_device_id vtskin_of_match[] = {
	{
		.compatible = "mediatek,mt6893-virtual-tskin",
	},
	{},
};
MODULE_DEVICE_TABLE(of, vtskin_of_match);
static struct platform_driver vtskin_driver = {
	.probe = vtskin_probe,
	.driver = {
		.name = "mtk-virtual-tskin",
		.of_match_table = vtskin_of_match,
	},
};
module_platform_driver(vtskin_driver);
MODULE_AUTHOR("Utkarsh Tripathi <utkarsh.tripathi@mediatek.com>");
MODULE_DESCRIPTION("Mediatek on virtual tskin driver");
MODULE_LICENSE("GPL v2");
