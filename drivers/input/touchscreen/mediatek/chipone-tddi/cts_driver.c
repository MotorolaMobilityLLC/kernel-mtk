#define LOG_TAG         "Driver"

#include "cts_config.h"
#include "cts_platform.h"
#include "cts_core.h"
#include "cts_driver.h"
#include "cts_charger_detect.h"
#include "cts_earjack_detect.h"
#include "cts_sysfs.h"
#include "cts_cdev.h"
#include "cts_strerror.h"

/* BEGIN, Ontim,  zsh, 2022/04/25, St-result :PASS,LCD and TP Device information */
extern unsigned char g_lcm_info_flag;
extern char lcd_info_pr[256];
u16  device_fw_ver = 0;
#include <ontim/ontim_dev_dgb.h>
static char version[40] = "0x00";
static char vendor_name[50] = "dj-icnl9911c";
static char lcdname[50] = "dj-icnl9911c";
DEV_ATTR_DECLARE(touch_screen)
DEV_ATTR_DEFINE("version", version)
DEV_ATTR_DEFINE("vendor", vendor_name)
DEV_ATTR_DEFINE("lcdvendor", lcdname)
DEV_ATTR_DECLARE_END;
ONTIM_DEBUG_DECLARE_AND_INIT(touch_screen,touch_screen,8);
/* END */

bool cts_show_debug_log = false;
module_param_named(debug_log, cts_show_debug_log, bool, 0660);
MODULE_PARM_DESC(debug_log, "Show debug log control");
extern char *mtkfb_find_lcm_driver(void);
int cts_driver_suspend(struct chipone_ts_data *cts_data)
{
    int ret;

    cts_info("Suspend");

    cts_lock_device(&cts_data->cts_dev);
    ret = cts_suspend_device(&cts_data->cts_dev);
    cts_unlock_device(&cts_data->cts_dev);

    if (ret) {
        cts_err("Suspend device failed %d(%s)",
            ret, cts_strerror(ret));
        // TODO:
        //return ret;
    }

    ret = cts_stop_device(&cts_data->cts_dev);
    if (ret) {
        cts_err("Stop device failed %d(%s)",
            ret, cts_strerror(ret));
        return ret;
    }

#ifdef CFG_CTS_GESTURE
    /* Enable IRQ wake if gesture wakeup enabled */
    if (cts_is_gesture_wakeup_enabled(&cts_data->cts_dev)) {
        ret = cts_plat_enable_irq_wake(cts_data->pdata);
        if (ret) {
            cts_err("Enable IRQ wake failed %d(%s)",
            ret, cts_strerror(ret));
            return ret;
        }
        ret = cts_plat_enable_irq(cts_data->pdata);
        if (ret){
            cts_err("Enable IRQ failed %d(%s)",
                ret, cts_strerror(ret));
            return ret;
        }
    }
#endif /* CFG_CTS_GESTURE */

    /** - To avoid waking up while not sleeping,
            delay 20ms to ensure reliability */
    msleep(20);

    return 0;
}

int cts_driver_resume(struct chipone_ts_data *cts_data)
{
    int ret;

    cts_info("Resume");

#ifdef CFG_CTS_GESTURE
    if (cts_is_gesture_wakeup_enabled(&cts_data->cts_dev)) {
        ret = cts_plat_disable_irq_wake(cts_data->pdata);
        if (ret) {
            cts_warn("Disable IRQ wake failed %d(%s)",
                ret, cts_strerror(ret));
            //return ret;
        }
        if ((ret = cts_plat_disable_irq(cts_data->pdata)) < 0) {
            cts_err("Disable IRQ failed %d(%s)",
                ret, cts_strerror(ret));
            //return ret;
        }
    }
#endif /* CFG_CTS_GESTURE */

    cts_lock_device(&cts_data->cts_dev);
    ret = cts_resume_device(&cts_data->cts_dev);
    cts_unlock_device(&cts_data->cts_dev);
    if(ret) {
        cts_warn("Resume device failed %d(%s)",
            ret, cts_strerror(ret));
        return ret;
    }

    ret = cts_start_device(&cts_data->cts_dev);
    if (ret) {
        cts_err("Start device failed %d(%s)",
            ret, cts_strerror(ret));
        return ret;
    }

    return 0;
}

//add by zsh,2022/04/25
//start
static void cts_tp_fw(struct cts_device *cts_dev)
{
	u8 cts_fw = 0;
	cts_fw_reg_readw_retry(cts_dev,
                    CTS_DEVICE_FW_REG_VERSION, &device_fw_ver, 5, 0);
	cts_fw = be16_to_cpup(&device_fw_ver);
	cts_info("ver:0x%x", cts_fw);
	snprintf(version, sizeof(version),"FW_02_0x%x#VID_0x98", cts_fw);
	cts_info("version:%s", version);
}
//end

int cts_driver_probe(struct device *device, enum cts_bus_type bus_type)
{
    struct chipone_ts_data *cts_data = NULL;
    int ret = 0;

    /* BEGIN, Ontim,  zsh, 2022/04/25, St-result :PASS,LCD and TP Device information */
    if(CHECK_THIS_DEV_DEBUG_AREADY_EXIT()==0)
    {
        return -EIO;
    }
    /* END */

    cts_data = kzalloc(sizeof(struct chipone_ts_data), GFP_KERNEL);
    if (cts_data == NULL) {
        cts_err("Alloc chipone_ts_data failed");
        return -ENOMEM;
    }

    cts_data->pdata = kzalloc(sizeof(struct cts_platform_data), GFP_KERNEL);
    if (cts_data->pdata == NULL) {
        cts_err("Alloc cts_platform_data failed");
        ret = -ENOMEM;
        goto err_free_cts_data;
    }

    chipone_ts_data = cts_data;

    cts_data->cts_dev.bus_type = bus_type;
    dev_set_drvdata(device, cts_data);
    cts_data->device = device;

    ret = cts_init_platform_data(cts_data->pdata, device, bus_type);
    if (ret) {
        cts_err("Init platform data failed %d(%s)",
            ret, cts_strerror(ret));
        goto err_free_pdata;
    }

    cts_data->cts_dev.pdata = cts_data->pdata;
    cts_data->pdata->cts_dev = &cts_data->cts_dev;

    cts_data->workqueue =
        create_singlethread_workqueue(CFG_CTS_DEVICE_NAME "-workqueue");
    if (cts_data->workqueue == NULL) {
        cts_err("Create workqueue failed");
        ret = -ENOMEM;
        goto err_free_pdata;
    }

#ifdef CONFIG_CTS_ESD_PROTECTION
    cts_data->esd_workqueue =
        create_singlethread_workqueue(CFG_CTS_DEVICE_NAME "-esd_workqueue");
    if (cts_data->esd_workqueue == NULL) {
        cts_err("Create esd workqueue failed");
        ret = -ENOMEM;
        goto err_destroy_workqueue;
    }
#endif
    ret = cts_plat_request_resource(cts_data->pdata);
    if (ret < 0) {
        cts_err("Request resource failed %d(%s)",
            ret, cts_strerror(ret));
        goto err_destroy_esd_workqueue;
    }

    ret = cts_plat_reset_device(cts_data->pdata);
    if (ret < 0) {
        cts_err("Reset device failed %d(%s)",
            ret, cts_strerror(ret));
        goto err_free_resource;
    }

    ret = cts_probe_device(&cts_data->cts_dev);
    if (ret) {
        cts_err("Probe device failed %d(%s)",
            ret, cts_strerror(ret));
        goto err_free_resource;
    }

    ret = cts_plat_init_touch_device(cts_data->pdata);
    if (ret < 0) {
        cts_err("Init touch device failed %d(%s)",
            ret, cts_strerror(ret));
        goto err_free_resource;
    }

    ret = cts_plat_init_vkey_device(cts_data->pdata);
    if (ret < 0) {
        cts_err("Init vkey device failed %d(%s)",
            ret, cts_strerror(ret));
        goto err_deinit_touch_device;
    }

    ret = cts_plat_init_gesture(cts_data->pdata);
    if (ret < 0) {
        cts_err("Init gesture failed %d(%s)",
            ret, cts_strerror(ret));
        goto err_deinit_vkey_device;
    }

    cts_init_esd_protection(cts_data);

    ret = cts_tool_init(cts_data);
    if (ret < 0) {
        cts_warn("Init tool node failed %d(%s)",
            ret, cts_strerror(ret));
    }

    ret = cts_sysfs_add_device(device);
    if (ret < 0) {
        cts_warn("Add sysfs entry for device failed %d(%s)",
            ret, cts_strerror(ret));
    }

    ret = cts_init_cdev(cts_data);
    if (ret < 0) {
        cts_warn("Init cdev failed %d(%s)",
            ret, cts_strerror(ret));
    }

    ret = cts_plat_request_irq(cts_data->pdata);
    if (ret < 0) {
        cts_err("Request IRQ failed %d(%s)",
            ret, cts_strerror(ret));
        goto err_deinit_sysfs;
    }

    ret = cts_init_charger_detect(cts_data);
    if (ret) {
        cts_err("Init charger detect failed %d(%s)",
            ret, cts_strerror(ret));
        // Ignore this error
    }

    ret = cts_init_earjack_detect(cts_data);
    if (ret) {
        cts_err("Init earjack detect failed %d(%s)",
            ret, cts_strerror(ret));
        // Ignore this error
    }

    ret = cts_start_device(&cts_data->cts_dev);
    if (ret) {
        cts_err("Start device failed %d(%s)",
            ret, cts_strerror(ret));
        goto err_deinit_earjack_detect;
    }

#ifdef CONFIG_MTK_PLATFORM
    tpd_load_status = 1;
#endif /* CONFIG_MTK_PLATFORM */

//add by zsh 2022/04/25
//start
    if(strcmp(mtkfb_find_lcm_driver(), "ontim_icnl9911c_hdplus_dsi_vdo_dj") == 0) {
        snprintf(lcdname, sizeof(lcdname),"%s","dj-icnl9911c");
        snprintf(vendor_name, sizeof(vendor_name),"%s","dj-icnl9911c");
    }
    else {
        snprintf(lcdname, sizeof(lcdname),"%s","boe-icnl9911c");
        snprintf(vendor_name, sizeof(vendor_name),"%s","boe-icnl9911c");

    }

#if 0
    if (LCM_INFO_EASYQUICK_608 == g_lcm_info_flag) {
        snprintf(lcdname, sizeof(lcdname),"%s ", "dj-icnl9911c" );
        snprintf(vendor_name, sizeof(vendor_name),"%s ", "dj-icnl9911c" );
    } else if (LCM_INFO_HLT_GLASS == g_lcm_info_flag) {
        snprintf(lcdname, sizeof(lcdname),"%s ", "dj-icnl9911c" );
        snprintf(vendor_name, sizeof(vendor_name),"%s ", "dj-icnl9911c" );
    }
#endif
    cts_tp_fw(&cts_data->cts_dev);
    REGISTER_AND_INIT_ONTIM_DEBUG_FOR_THIS_DEV();
//end

    return 0;

err_deinit_earjack_detect:
    cts_deinit_earjack_detect(cts_data);
    cts_deinit_charger_detect(cts_data);
    cts_plat_free_irq(cts_data->pdata);
err_deinit_sysfs:
    cts_sysfs_remove_device(device);
#ifdef CONFIG_CTS_LEGACY_TOOL
    cts_tool_deinit(cts_data);
#endif /* CONFIG_CTS_LEGACY_TOOL */

#ifdef CONFIG_CTS_ESD_PROTECTION
    cts_deinit_esd_protection(cts_data);
#endif /* CONFIG_CTS_ESD_PROTECTION */

#ifdef CFG_CTS_GESTURE
    cts_plat_deinit_gesture(cts_data->pdata);
#endif /* CFG_CTS_GESTURE */

err_deinit_vkey_device:
#ifdef CONFIG_CTS_VIRTUALKEY
    cts_plat_deinit_vkey_device(cts_data->pdata);
#endif /* CONFIG_CTS_VIRTUALKEY */

err_deinit_touch_device:
    cts_plat_deinit_touch_device(cts_data->pdata);

err_free_resource:
    cts_plat_free_resource(cts_data->pdata);
err_destroy_esd_workqueue:
#ifdef CONFIG_CTS_ESD_PROTECTION
    destroy_workqueue(cts_data->esd_workqueue);
err_destroy_workqueue:
#endif
    destroy_workqueue(cts_data->workqueue);

err_free_pdata:
    kfree(cts_data->pdata);
err_free_cts_data:
    kfree(cts_data);

    cts_err("Probe failed %d(%s)", ret, cts_strerror(ret));

    return ret;
}

int cts_driver_remove(struct device *device)
{
    struct chipone_ts_data *cts_data;
    int ret = 0;

    cts_info("Remove");

    cts_data = (struct chipone_ts_data *)dev_get_drvdata(device);
    if (cts_data) {
        ret = cts_stop_device(&cts_data->cts_dev);
        if (ret) {
            cts_warn("Stop device failed %d(%s)",
                ret, cts_strerror(ret));
        }

        cts_deinit_charger_detect(cts_data);
        cts_deinit_earjack_detect(cts_data);

        cts_plat_free_irq(cts_data->pdata);

        cts_tool_deinit(cts_data);

        cts_deinit_cdev(cts_data);

        cts_sysfs_remove_device(device);

        cts_deinit_esd_protection(cts_data);

        cts_plat_deinit_touch_device(cts_data->pdata);

        cts_plat_deinit_vkey_device(cts_data->pdata);

        cts_plat_deinit_gesture(cts_data->pdata);

        cts_plat_free_resource(cts_data->pdata);

#ifdef CONFIG_CTS_ESD_PROTECTION
        if (cts_data->esd_workqueue) {
            destroy_workqueue(cts_data->esd_workqueue);
        }
#endif

        if (cts_data->workqueue) {
            destroy_workqueue(cts_data->workqueue);
        }

        if (cts_data->pdata) {
            kfree(cts_data->pdata);
        }
        kfree(cts_data);
    }else {
        cts_warn("Remove while chipone_ts_data = NULL");
        return -EINVAL;
    }

    return ret;
}

#ifdef CONFIG_CTS_SYSFS
static ssize_t driver_config_show(struct device_driver *driver, char *buf)
{
#define SEPARATION_LINE \
    "-----------------------------------------------\n"

    int count = 0;

    count += scnprintf(buf + count, PAGE_SIZE - count,
        SEPARATION_LINE
        "%-32s: "CFG_CTS_DRIVER_VERSION"\n"
        "%-32s: "CFG_CTS_DRIVER_NAME"\n"
        "%-32s: "CFG_CTS_DEVICE_NAME"\n",
        "Driver Version", "Driver Name", "Device Name");

    count += scnprintf(buf + count, PAGE_SIZE - count,
        "%-32s: %c\n", "CONFIG_CTS_OF",
#ifdef CONFIG_CTS_OF
         'Y'
#else
         'N'
#endif
    );
#ifdef CONFIG_CTS_OF
    count += scnprintf(buf + count, PAGE_SIZE - count,
        "  %-30s: "CFG_CTS_OF_DEVICE_ID_NAME"\n",
        "CFG_CTS_OF_DEVICE_ID_NAME");
#endif /* CONFIG_CTS_OF */
    count += scnprintf(buf + count, PAGE_SIZE - count,
        SEPARATION_LINE
        "%-32s: %c\n", "CONFIG_CTS_LEGACY_TOOL",
#ifdef CONFIG_CTS_LEGACY_TOOL
         'Y'
#else
         'N'
#endif
    );
    count += scnprintf(buf + count, PAGE_SIZE - count,
        "%-32s: %c\n", "CONFIG_CTS_SYSFS",
#ifdef CONFIG_CTS_SYSFS
         'Y'
#else
         'N'
#endif
    );
    count += scnprintf(buf + count, PAGE_SIZE - count,
        "%-32s: %c\n", "CFG_CTS_HANDLE_IRQ_USE_KTHREAD",
#ifdef CFG_CTS_HANDLE_IRQ_USE_KTHREAD
         'Y'
#else
         'N'
#endif
    );
    count += scnprintf(buf + count, PAGE_SIZE - count,
        "%-32s: %c\n", "CFG_CTS_MAKEUP_EVENT_UP",
#ifdef CFG_CTS_MAKEUP_EVENT_UP
         'Y'
#else
         'N'
#endif
    );

    /* Reset pin, i2c/spi bus */
    count += scnprintf(buf + count, PAGE_SIZE - count,
        SEPARATION_LINE
        "%-32s: %c\n", "CFG_CTS_HAS_RESET_PIN",
#ifdef CFG_CTS_HAS_RESET_PIN
        'Y'
#else
        'N'
#endif
    );

#ifdef CONFIG_CTS_I2C_HOST
    count += scnprintf(buf + count, PAGE_SIZE - count,
        "%-32s: Y\n"
        "  %-30s: %u\n",
        "CONFIG_CTS_I2C_HOST",
        "CFG_CTS_MAX_I2C_XFER_SIZE", CFG_CTS_MAX_I2C_XFER_SIZE);
#endif /* CONFIG_CTS_I2C_HOST */

#ifdef CONFIG_CTS_SPI_HOST
    count += scnprintf(buf + count, PAGE_SIZE - count,
        "%-32s: Y\n"
        "  %-30s: %u\n"
        "  %-30s: %uKbps\n",
        "CONFIG_CTS_SPI_HOST",
        "CFG_CTS_MAX_SPI_XFER_SIZE", CFG_CTS_MAX_SPI_XFER_SIZE,
        "CFG_CTS_SPI_SPEED_KHZ", CFG_CTS_SPI_SPEED_KHZ);
#endif /* CONFIG_CTS_I2C_HOST */

    /* Firmware */
    count += scnprintf(buf + count, PAGE_SIZE - count,
        SEPARATION_LINE
        "%-32s: %c\n", "CFG_CTS_DRIVER_BUILTIN_FIRMWARE",
#ifdef CFG_CTS_DRIVER_BUILTIN_FIRMWARE
        'Y'
#else
        'N'
#endif
    );
    count += scnprintf(buf + count, PAGE_SIZE - count,
        "%-32s: %c\n", "CFG_CTS_FIRMWARE_IN_FS",
#ifdef CFG_CTS_FIRMWARE_IN_FS
        'Y'
#else
        'N'
#endif
    );
#ifdef CFG_CTS_FIRMWARE_IN_FS
    count += scnprintf(buf + count, PAGE_SIZE - count,
        "%-32s: "CFG_CTS_FIRMWARE_FILEPATH"\n",
        "CFG_CTS_FIRMWARE_FILEPATH");
#endif /* CFG_CTS_FIRMWARE_IN_FS */

    /* Input device & features */
    count += scnprintf(buf + count, PAGE_SIZE - count,
        SEPARATION_LINE
        "%-32s: %c\n", "CONFIG_CTS_SLOTPROTOCOL",
#ifdef CONFIG_CTS_SLOTPROTOCOL
         'Y'
#else
         'N'
#endif
    );
    count += scnprintf(buf + count, PAGE_SIZE - count,
        "%-32s: %d\n", "CFG_CTS_MAX_TOUCH_NUM",
        CFG_CTS_MAX_TOUCH_NUM);

    count += scnprintf(buf + count, PAGE_SIZE - count,
        "%-32s: %c\n"
        "%-32s: %c\n"
        "%-32s: %c\n",
        "CFG_CTS_SWAP_XY",
#ifdef CFG_CTS_SWAP_XY
        'Y',
#else
        'N',
#endif
        "CFG_CTS_WRAP_X",
#ifdef CFG_CTS_WRAP_X
        'Y',
#else
        'N',
#endif
        "CFG_CTS_WRAP_Y",
#ifdef CFG_CTS_WRAP_Y
        'Y'
#else
        'N'
#endif
    );
    count += scnprintf(buf + count, PAGE_SIZE - count,
        "%-32s: %c\n", "CONFIG_CTS_GLOVE",
#ifdef CONFIG_CTS_GLOVE
       'Y'
#else
       'N'
#endif
    );
    count += scnprintf(buf + count, PAGE_SIZE - count,
        "%-32s: %c\n", "CFG_CTS_GESTURE",
#ifdef CFG_CTS_GESTURE
       'Y'
#else
       'N'
#endif
    );

    /* Charger detect */
    count += scnprintf(buf + count, PAGE_SIZE - count,
        SEPARATION_LINE
        "%-32s: %c\n", "CONFIG_CTS_CHARGER_DETECT",
#ifdef CONFIG_CTS_CHARGER_DETECT
       'Y'
#else
       'N'
#endif
    );

    /* Earjack detect */
    count += scnprintf(buf + count, PAGE_SIZE - count,
        SEPARATION_LINE
        "%-32s: %c\n", "CONFIG_CTS_EARJACK_DETECT",
#ifdef CONFIG_CTS_EARJACK_DETECT
       'Y'
#else
       'N'
#endif
    );

    /* ESD protection */
    count += scnprintf(buf + count, PAGE_SIZE - count,
        SEPARATION_LINE
        "%-32s: %c\n", "CONFIG_CTS_ESD_PROTECTION",
#ifdef CONFIG_CTS_ESD_PROTECTION
        'Y'
#else
        'N'
#endif
        );
#ifdef CONFIG_CTS_ESD_PROTECTION
    count += scnprintf(buf + count, PAGE_SIZE - count,
        "  %-30s: %uHz\n"
        "  %-30s: %u\n",
        "CFG_CTS_ESD_PROTECTION_CHECK_PERIOD",
        "CFG_CTS_ESD_FAILED_CONFIRM_CNT",
        CFG_CTS_ESD_PROTECTION_CHECK_PERIOD,
        CFG_CTS_ESD_FAILED_CONFIRM_CNT);
#endif /* CONFIG_CTS_ESD_PROTECTION */

    count += scnprintf(buf + count, PAGE_SIZE - count, SEPARATION_LINE);

    return count;
#undef SEPARATION_LINE
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0)
static DRIVER_ATTR(driver_config, S_IRUGO, driver_config_show, NULL);
#else
static DRIVER_ATTR_RO(driver_config);
#endif

static struct attribute *cts_driver_config_attrs[] = {
    &driver_attr_driver_config.attr,
    NULL
};

static const struct attribute_group cts_driver_config_group = {
    .name = "config",
    .attrs = cts_driver_config_attrs,
};

const struct attribute_group *cts_driver_config_groups[] = {
    &cts_driver_config_group,
    NULL,
};
#endif /* CONFIG_CTS_SYSFS */

#ifdef CONFIG_CTS_OF
const struct of_device_id cts_driver_of_match_table[] = {
    {.compatible = CFG_CTS_OF_DEVICE_ID_NAME,},
    { },
};
MODULE_DEVICE_TABLE(of, cts_driver_of_match_table);
#endif /* CONFIG_CTS_OF */

int cts_driver_init(void)
{
    int ret = 0;

    cts_info("Chipone touch driver init, version: "CFG_CTS_DRIVER_VERSION);

#ifdef CONFIG_CTS_I2C_HOST
    cts_info(" - Register i2c driver");
    ret = i2c_add_driver(&cts_i2c_driver);
    if (ret) {
        cts_info("Register i2c driver failed %d(%s)",
            ret, cts_strerror(ret));
    }
#endif /* CONFIG_CTS_I2C_HOST */

#ifdef CONFIG_CTS_SPI_HOST
    cts_info(" - Register spi driver");
    ret = spi_register_driver(&cts_spi_driver);
    if (ret) {
        cts_info("Register spi driver failed %d(%s)",
            ret, cts_strerror(ret));
    }
#endif /* CONFIG_CTS_SPI_HOST */

    return 0;
}

void cts_driver_exit(void)
{
    cts_info("Exit");

#ifdef CONFIG_CTS_I2C_HOST
    cts_info(" - Delete i2c driver");
    i2c_del_driver(&cts_i2c_driver);
#endif /* CONFIG_CTS_I2C_HOST */

#ifdef CONFIG_CTS_SPI_HOST
    cts_info(" - Delete spi driver");
    spi_unregister_driver(&cts_spi_driver);
#endif /* CONFIG_CTS_SPI_HOST */
}

