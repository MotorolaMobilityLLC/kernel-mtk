// SPDX-License-Identifier: GPL-2.0+
/**
 * Copyright (C) Shanghai FourSemi Semiconductor Co.,Ltd 2016-2024. All rights reserved.
 * 2024-03-25 File created.
 */

#include <linux/module.h>
#include "frsm-amp-drv.h"

extern struct i2c_driver frsm_i2c_driver;
extern struct platform_driver frsm_amp_driver;

#ifdef FRSM_I2CA_EXPORT_SUPPORT
int frsm_i2ca_get_ndev(void)
{
	struct frsm_amp *frsm_amp = frsm_amp_get_pdev();

	if (frsm_amp == NULL)
		return 0;

	return frsm_amp->spkinfo.ndev;
}
EXPORT_SYMBOL_GPL(frsm_i2ca_get_ndev);

int frsm_i2ca_init_dev(int spkid, bool force)
{
	return frsm_amp_init_dev(spkid, force);
}
EXPORT_SYMBOL_GPL(frsm_i2ca_init_dev);

int frsm_i2ca_set_scene(int spkid, int scene)
{
	return frsm_amp_set_scene(spkid, scene);
}
EXPORT_SYMBOL_GPL(frsm_i2ca_set_scene);

int frsm_i2ca_spk_switch(int spkid, bool on)
{
	int ret;

	if (on) {
		ret  = frsm_amp_spk_switch(spkid, true);
		ret |= frsm_amp_send_event(EVENT_STREAM_ON, NULL, 0);
	} else {
		ret  = frsm_amp_send_event(EVENT_STREAM_OFF, NULL, 0);
		ret |= frsm_amp_spk_switch(spkid, false);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(frsm_i2ca_spk_switch);
#endif

static int __init frsm_drv_init(void)
{
	int ret;

	ret = platform_driver_register(&frsm_amp_driver);
	if (ret)
		pr_err("Failed to add frsm_amp_driver:%d\n", ret);

	return i2c_add_driver(&frsm_i2c_driver);
}

static void __exit frsm_drv_exit(void)
{
	platform_driver_unregister(&frsm_amp_driver);
	i2c_del_driver(&frsm_i2c_driver);
}

module_init(frsm_drv_init);
module_exit(frsm_drv_exit);

MODULE_AUTHOR("FourSemi SW <support@foursemi.com>");
MODULE_DESCRIPTION("ASoC FourSemi Audio Amplifier Driver");
MODULE_VERSION(FRSM_I2C_VERSION);
MODULE_LICENSE("GPL");
