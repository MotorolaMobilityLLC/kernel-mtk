#ifndef CTS_DRIVER_H
#define CTS_DRIVER_H

#ifdef CONFIG_CTS_SYSFS
#include <linux/sysfs.h>
extern const struct attribute_group *cts_driver_config_groups[];
#endif /* CONFIG_CTS_SYSFS */

#ifdef CONFIG_CTS_OF
#include <linux/of.h>
extern const struct of_device_id cts_driver_of_match_table[];
#endif /* CONFIG_CTS_OF */

#ifdef CONFIG_CTS_I2C_HOST
#include <linux/i2c.h>
extern struct i2c_driver cts_i2c_driver;
#endif /* CONFIG_CTS_I2C_HOST */

#ifdef CONFIG_CTS_SPI_HOST
#include <linux/spi/spi.h>
extern struct spi_driver cts_spi_driver;
#endif /* CONFIG_CTS_I2C_HOST */

#include "cts_core.h"

extern int cts_driver_init(void);
extern void cts_driver_exit(void);
extern int cts_driver_probe(struct device *device, enum cts_bus_type bus_type);
extern int cts_driver_remove(struct device *device);
extern int cts_driver_suspend(struct chipone_ts_data *cts_data);
extern int cts_driver_resume(struct chipone_ts_data *cts_data);

#endif /* CTS_DRIVER_H */

