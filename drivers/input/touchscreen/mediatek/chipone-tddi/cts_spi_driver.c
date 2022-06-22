#define LOG_TAG         "SPIDrv"

#include <linux/spi/spi.h>
#include <linux/of.h>
#include "cts_config.h"
#include "cts_platform.h"
#include "cts_core.h"
#include "cts_driver.h"
#include "cts_strerror.h"

#ifdef CONFIG_CTS_SPI_HOST
static int cts_spi_driver_probe(struct spi_device *spi_dev)
{
    if (spi_dev == NULL) {
        cts_info("Spi driver probe with spi_dev = NULL");
        return -EINVAL;
    }

    cts_info("Probe spi device '%s': "
             "mode='%u' speed=%u bits_per_word=%u "
             "chip_select=%u, cs_gpio=%d irq=%d",
        spi_dev->modalias,
        spi_dev->mode, spi_dev->max_speed_hz, spi_dev->bits_per_word,
        spi_dev->chip_select, spi_dev->cs_gpio, spi_dev->irq);

    return cts_driver_probe(&spi_dev->dev, CTS_SPI_BUS);
}

static int cts_spi_driver_remove(struct spi_device *spi_dev)
{
    if (spi_dev == NULL) {
        cts_info("Spi driver remove with spi_dev = NULL");
        return -EINVAL;
    }

    return cts_driver_remove(&spi_dev->dev);
}

static const struct spi_device_id cts_spi_device_id_table[] = {
    {CFG_CTS_DEVICE_NAME, 0},
    {}
};

struct spi_driver cts_spi_driver = {
    .probe = cts_spi_driver_probe,
    .remove = cts_spi_driver_remove,
    .driver = {
        .name = CFG_CTS_DRIVER_NAME,
        .owner = THIS_MODULE,
#ifdef CONFIG_CTS_OF
        .of_match_table = of_match_ptr(cts_driver_of_match_table),
#endif /* CONFIG_CTS_OF */
#ifdef CONFIG_CTS_SYSFS
        .groups = cts_driver_config_groups,
#endif /* CONFIG_CTS_SYSFS */
    },
    .id_table = cts_spi_device_id_table,
};
#endif /* CONFIG_CTS_SPI_HOST */

