#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/clk.h>
#include <linux/version.h>
#include <linux/wait.h>

#if defined(PLATFORM_MTK) || defined(PLATFORM_MT6739) || defined(PLATFORM_MT676x)
#ifdef CONFIG_MTK_CLKMGR
#include "mach/mt_clkmgr.h"
#endif

#endif

#include "bf_fp_platform.h"

#ifdef USE_SPI1_4GB_TEST
#include <linux/of_reserved_mem.h>
#endif

/* MTK header */
#if defined(PLATFORM_MT6739) || defined(PLATFORM_MT676x)
#include <linux/platform_data/spi-mt65xx.h>
static struct mtk_chip_config spi_init_conf = {
    .cs_pol = 0,
    .rx_mlsb = 1,
    .tx_mlsb = 1,
    .sample_sel = 0,
};

#if defined TEE_TK
enum spi_sample_sel_t {
    POSEDGE,
    NEGEDGE
};
enum spi_cs_pol_t {
    ACTIVE_LOW,
    ACTIVE_HIGH
};

enum spi_cpol_t {
    SPI_CPOL_0,
    SPI_CPOL_1
};

enum spi_cpha_t {
    SPI_CPHA_0,
    SPI_CPHA_1
};

enum spi_mlsb_t {
    SPI_LSB,
    SPI_MSB
};

enum spi_endian_t {
    SPI_LENDIAN,
    SPI_BENDIAN
};

enum spi_transfer_mode_t {
    FIFO_TRANSFER,
    DMA_TRANSFER,
    OTHER1,
    OTHER2,
};

enum spi_pause_mode_t {
    PAUSE_MODE_DISABLE,
    PAUSE_MODE_ENABLE
};
enum spi_finish_intr_t {
    FINISH_INTR_DIS,
    FINISH_INTR_EN,
};

enum spi_deassert_mode_t {
    DEASSERT_DISABLE,
    DEASSERT_ENABLE
};

enum spi_ulthigh_t {
    ULTRA_HIGH_DISABLE,
    ULTRA_HIGH_ENABLE
};

enum spi_tckdly_t {
    TICK_DLY0,
    TICK_DLY1,
    TICK_DLY2,
    TICK_DLY3
};

struct mt_chip_conf {
    u32 setuptime;
    u32 holdtime;
    u32 high_time;
    u32 low_time;
    u32 cs_idletime;
    u32 ulthgh_thrsh;
    enum spi_sample_sel_t sample_sel;
    enum spi_cs_pol_t cs_pol;
    enum spi_cpol_t cpol;
    enum spi_cpha_t cpha;
    enum spi_mlsb_t tx_mlsb;
    enum spi_mlsb_t rx_mlsb;
    enum spi_endian_t tx_endian;
    enum spi_endian_t rx_endian;
    enum spi_transfer_mode_t com_mod;
    enum spi_pause_mode_t pause;
    enum spi_finish_intr_t finish_intr;
    enum spi_deassert_mode_t deassert;
    enum spi_ulthigh_t ulthigh;
    enum spi_tckdly_t tckdly;
};

static struct mt_chip_conf spi_conf_tk = {
    .setuptime = 10,
    .holdtime = 10,
    .high_time = 8,
    .low_time =  8,
    .cs_idletime = 20, //10,
    //.ulthgh_thrsh = 0,
    .cpol = 0,
    .cpha = 0,
    .rx_mlsb = 1,
    .tx_mlsb = 1,
    .tx_endian = 0,
    .rx_endian = 0,
    .com_mod = FIFO_TRANSFER,
    .pause = 1,
    .finish_intr = 1,
    .deassert = 0,
    .ulthigh = 0,
    .tckdly = 0,
};
#endif  //TEE_TK

#elif defined(PLATFORM_MTK)
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 4, 0))
#include <linux/of_gpio.h>
#elif (LINUX_VERSION_CODE > KERNEL_VERSION(4, 0, 0))
#include "mtk_spi.h"
#include "mtk_spi_hal.h"
#include "mtk_gpio.h"
#include "mach/gpio_const.h"
#elif (LINUX_VERSION_CODE > KERNEL_VERSION(3, 18, 0))
#include "mt_spi.h"
#include <mt_chip.h>
#elif (LINUX_VERSION_CODE > KERNEL_VERSION(3, 10, 0))
#include <mach/mt_spi.h>
#endif

static struct mt_chip_conf spi_init_conf = {
    .setuptime = 10,
    .holdtime = 10,
    .high_time = 8,
    .low_time =  8,
    .cs_idletime = 20, //10,
    //.ulthgh_thrsh = 0,
    .cpol = 0,
    .cpha = 0,
    .rx_mlsb = 1,
    .tx_mlsb = 1,
    .tx_endian = 0,
    .rx_endian = 0,
    .com_mod = FIFO_TRANSFER,
    .pause = 1,
    .finish_intr = 1,
    .deassert = 0,
    .ulthigh = 0,
    .tckdly = 0,
};
#endif

extern struct bf_device *g_bf_dev;
extern wait_queue_head_t waiting_spi_prepare;
extern atomic_t suspended;
u32 g_chip_type = 0;
static DEFINE_MUTEX(spi_lock);

#ifdef USE_SPI1_4GB_TEST
#define OMIT_PIXCEL     (1)
static dma_addr_t SpiDmaBufTx_pa;
static dma_addr_t SpiDmaBufRx_pa;
static char *spi_tx_local_buf;
static char *spi_rx_local_buf;

static int reserve_memory_spi_fn(struct reserved_mem *rmem)
{
    BF_LOG(" name: %s, base: 0x%llx, size: 0x%llx\n", rmem->name,
           (unsigned long long)rmem->base, (unsigned long long)rmem->size);
    BUG_ON(rmem->size < 0x8000);
    SpiDmaBufTx_pa = rmem->base;
    SpiDmaBufRx_pa = rmem->base + 0x4000;
    return 0;
}

RESERVEDMEM_OF_DECLARE(reserve_memory_test, "mediatek,spi-reserve-memory", reserve_memory_spi_fn);

static int spi_setup_xfer(struct spi_transfer *xfer)
{
    /* map physical addr to virtual addr */
    if (NULL == spi_tx_local_buf) {
        spi_tx_local_buf = (char *)ioremap_nocache(SpiDmaBufTx_pa, 0x4000);
        if (!spi_tx_local_buf) {
            BF_LOG("SPI Failed to dma_alloc_coherent()\n");
            return -ENOMEM;
        }
    }
    if (NULL == spi_rx_local_buf) {
        spi_rx_local_buf = (char *)ioremap_nocache(SpiDmaBufRx_pa, 0x4000);
        if (!spi_rx_local_buf) {
            BF_LOG("SPI Failed to dma_alloc_coherent()\n");
            return -ENOMEM;
        }
    }
    return 0;
}
#endif

int spi_set_dma_en(int mode)
{
#if defined(PLATFORM_MTK)
    struct mt_chip_conf* spi_par;
    spi_par = &spi_init_conf;
    if (!spi_par) {
        return -1;
    }
    if (1 == mode) {
        if (spi_par->com_mod == DMA_TRANSFER) {
            return 0;
        }
        spi_par->com_mod = DMA_TRANSFER;
    } else {
        if (spi_par->com_mod == FIFO_TRANSFER) {
            return 0;
        }
        spi_par->com_mod = FIFO_TRANSFER;
    }
    spi_setup(g_bf_dev->spi);
#endif
    return 0;
}

/*----------------------------------------------------------------------------*/
int spi_send_cmd(struct bf_device *bf_dev, u8 *tx, u8 *rx, u16 spilen)
{
#ifdef TEE_TK
    extern int tee_spi_transfer(void *conf, uint32_t conf_size, void *inbuf, void *outbuf, uint32_t size);
#if defined(PLATFORM_MT6739) || defined(PLATFORM_MT676x)
    int ret = tee_spi_transfer(&spi_conf_tk, sizeof(spi_conf_tk), tx, rx, spilen);
#else
    int ret = tee_spi_transfer(&spi_init_conf, sizeof(spi_init_conf), tx, rx, spilen);
#endif

#else
    int ret = 0;
    struct spi_message m;
    struct spi_transfer t = {
        .tx_buf = tx,
        .rx_buf = rx,
        .len = spilen,
#ifdef USE_SPI1_4GB_TEST
        .tx_dma = SpiDmaBufTx_pa,
        .rx_dma = SpiDmaBufRx_pa,
#else
        .tx_dma = 0,
        .rx_dma = 0,
#endif
    };
#ifndef KERNEL_4_9
    mutex_lock(&spi_lock);
#endif
    spi_message_init(&m);
    spi_message_add_tail(&t, &m);
    ret = spi_sync(bf_dev->spi, &m);

#ifndef KERNEL_4_9
    mutex_unlock(&spi_lock);
#endif
#endif
    return ret;
}

#ifdef USE_SPI1_4GB_TEST
int spi_dma_exchange(struct bf_device *bl229x, bl_read_write_reg_command_t read_write_cmd, int dma_size)
{
    memcpy(spi_tx_local_buf, read_write_cmd.data_tx, sizeof(read_write_cmd.data_tx));
    spi_send_cmd(bl229x, spi_tx_local_buf, spi_rx_local_buf, dma_size);
    memcpy(bl229x->image_buf, spi_rx_local_buf + OMIT_PIXCEL, dma_size - OMIT_PIXCEL);
    return 0;
}
#endif

/*----------------------------------------------------------------------------*/
u8 bf_spi_write_reg(u8 reg, u8 value)
{
    u8 nAddr;
    u8 data_tx[4] = {0};
    u8 data_rx[4] = {0};

    nAddr = reg << 1;
    nAddr |= 0x80;

    data_tx[0] = nAddr;
    data_tx[1] = value;

    spi_send_cmd(g_bf_dev, data_tx, data_rx, 2);
    return data_rx[1];
}

//-------------------------------------------------------------------------------------------------
u8 bf_spi_read_reg(u8 reg)
{
    u8 nAddr;
    u8 data_tx[4] = {0};
    u8 data_rx[4] = {0};

    nAddr = reg << 1;
    nAddr &= 0x7F;

    data_tx[0] = nAddr;
    data_tx[1] = 0xff;

    spi_send_cmd(g_bf_dev, data_tx, data_rx, 2);
    return data_rx[1];
}


/*----------------------------------------------------------------------------*/
u8 bf_spi_write_reg_bit(u8 nRegID, u8 bit, u8 value)
{
    u8 tempvalue = 0;

    tempvalue = bf_spi_read_reg(nRegID);
    tempvalue &= ~(1 << bit);
    tempvalue |= (value << bit);
    bf_spi_write_reg(nRegID, tempvalue);

    return 0;
}

int bf_read_chipid(void)
{
    u8 val_low = 0, val_high = 0, version = 0;
    int chip_id = 0;
    u8	reg_value = 0;

    if(g_chip_type == BF3290) {
        bf_spi_write_reg (0x13, 0x00);
        reg_value = bf_spi_read_reg (0x3a);
        bf_spi_write_reg (0x3a, reg_value | 0x80);

        val_low = bf_spi_read_reg (0x10); //id reg low
        BF_LOG ("val_low=0x%x \n", val_low);

        val_high = bf_spi_read_reg (0x11); //id reg high
        BF_LOG ("val_high=0x%x \n", val_high);

        version = bf_spi_read_reg (0x12); //ic type
        BF_LOG ("version=0x%x \n", version);
        chip_id = (val_high << 8) | (val_low & 0xff);
        BF_LOG ("chip_id=%x \n", chip_id);
        bf_spi_write_reg (0x3a, reg_value);
    } else if(g_chip_type == BF3182 || g_chip_type == BF3390) {
        //enable 0x10 bit5
        reg_value = bf_spi_read_reg(0x10);
        reg_value &= ~(1 << 5);
        bf_spi_write_reg(0x10, reg_value);

        val_high = bf_spi_read_reg(0x37);
        val_low = bf_spi_read_reg(0x36);
        chip_id = (val_high << 8) | val_low;
        version = bf_spi_read_reg(0x38);

        //disabl 0x10 bit5
        reg_value |= (1 << 5);
        bf_spi_write_reg(0x10, reg_value);
    } else {
        val_high = bf_spi_read_reg(0x37);
        val_low = bf_spi_read_reg(0x36);
        chip_id = (val_high << 8) | val_low;
        version = bf_spi_read_reg(0x38);

        if(chip_id != BF3582P && chip_id != BF3582S && chip_id != 0x5883 && chip_id != 0x5B83 && chip_id != 0x5683 && chip_id != 0x5a83) {
            //enable 0x10 bit5
            reg_value = bf_spi_read_reg(0x10);
            reg_value &= ~(1 << 5);
            bf_spi_write_reg(0x10, reg_value);

            val_high = bf_spi_read_reg(0x37);
            val_low = bf_spi_read_reg(0x36);
            chip_id = (val_high << 8) | val_low;
            version = bf_spi_read_reg(0x38);

            //disabl 0x10 bit5
            reg_value |= (1 << 5);
            bf_spi_write_reg(0x10, reg_value);
            if(chip_id != BF3182 && chip_id != BF3390) {
                bf_spi_write_reg (0x13, 0x00);
                reg_value = bf_spi_read_reg (0x3a);
                bf_spi_write_reg (0x3a, reg_value | 0x80);

                val_low = bf_spi_read_reg (0x10); //id reg low
                BF_LOG ("val_low=0x%x \n", val_low);

                val_high = bf_spi_read_reg (0x11); //id reg high
                BF_LOG ("val_high=0x%x \n", val_high);

                version = bf_spi_read_reg (0x12); //ic type
                BF_LOG ("version=0x%x \n", version);
                chip_id = (val_high << 8) | (val_low & 0xff);
                BF_LOG ("chip_id=%x \n", chip_id);
                bf_spi_write_reg (0x3a, reg_value);
            }
        }
    }

    BF_LOG(" chip_id=0x%x,version=0x%x\n", chip_id, version);
    return chip_id;
}

void bf_chip_info(void)
{
    BF_LOG("data:2017-12-18\n");
    g_chip_type = bf_read_chipid();
    BF_LOG("BTL: chipid:%x\r\n", g_chip_type);
}

#ifdef CONFIG_MTK_CLK
#if defined (PLATFORM_MT6739)
struct mtk_spi {
    void __iomem *base;
    void __iomem *peri_regs;
    u32 state;
    int pad_num;
    u32 *pad_sel;
    struct clk *parent_clk, *sel_clk, *spi_clk;
    struct spi_transfer *cur_transfer;
    u32 xfer_len;
    struct scatterlist *tx_sgl, *rx_sgl;
    u32 tx_sgl_len, rx_sgl_len;
    const struct mtk_spi_compatible *dev_comp;
    u32 dram_8gb_offset;
};
#endif
void bf_spi_clk_enable(struct bf_device *bf_dev, u8 onoff)
{
    static int count = 0;

#if defined(PLATFORM_MT6739)
    struct mtk_spi *mdata = NULL;
    int ret = -1;
    mdata = spi_master_get_devdata(bf_dev->spi->master);

    if (onoff && (count == 0)) {
        ret = clk_prepare_enable(mdata->spi_clk);
        if (ret < 0) {
            BF_LOG("failed to enable spi_clk (%d)\n", ret);
        } else
            count = 1;
    } else if ((count > 0) && (onoff == 0)) {
        clk_disable_unprepare(mdata->spi_clk);
        count = 0;
    }
#elif defined(USE_ENABLE_CLOCK)
    if (onoff)
        enable_clock(MT_CG_SPI_SW_CG, "spi");
    else
        disable_clock(MT_CG_SPI_SW_CG, "spi");
#else
    if (onoff && (count == 0)) {
        mt_spi_enable_master_clk(bf_dev->spi);
        count = 1;
    } else if ((count > 0) && (onoff == 0)) {
        mt_spi_disable_master_clk(bf_dev->spi);
        count = 0;
    }
#endif
}
#endif

#if !defined(PLATFORM_SPRD) && !defined(PLATFORM_QCOM) && !defined(PLATFORM_RK)
#if defined(BF_PINCTL)
static int32_t bf_platform_pinctrl_spi_init(struct bf_device *bf_dev)
{
    int32_t error = 0;


    error = pinctrl_select_state(bf_dev->pinctrl_gpios, bf_dev->pins_spi_default);
    if (error) {
        dev_err(&bf_dev->pdev->dev, "failed to activate pins_spi_default state\n");
    }

    return error;
}

#else
static int32_t bf_platform_gpio_spi_init(struct bf_device *bf_dev)
{
    //please setup and check in dws
    return 0;
}
#endif
#endif

static int bf_spi_suspend (struct device *dev)
{
    BF_LOG("bf_spi_suspend+++\n");
    atomic_set(&suspended, 1);
    BF_LOG("bf_spi_suspend----\n");
    return 0;
}

static int bf_spi_resume (struct device *dev)
{
    BF_LOG("bf_spi_resume+++\n");
    atomic_set(&suspended, 0);
    wake_up_interruptible(&waiting_spi_prepare);
    BF_LOG("bf_spi_resume------\n");
    return 0;
}

static int bf_spi_remove(struct spi_device *spi)
{
    return 0;
}

int32_t bf_platform_uninit(struct bf_device *bf_dev)
{
#ifdef CONFIG_MTK_CLK
    bf_spi_clk_enable(bf_dev, 0);
#endif

    bf_remove(g_bf_dev->pdev);
    return 0;
}

static int bf_spi_probe (struct spi_device *spi)
{
    int status = -EINVAL;

#if defined(COMPATIBLE)
    status = bf_init_dts_and_irq(g_bf_dev);
    if (status) {
        BF_LOG("bf_init_dts_and_irq failed");
        bf_remove(g_bf_dev->pdev);
        return status;
    }

#if !defined(PLATFORM_SPRD) && !defined(PLATFORM_QCOM) && !defined(PLATFORM_RK)
#if defined(BF_PINCTL)
    status = bf_platform_pinctrl_spi_init(g_bf_dev);
    if(status) {
        BF_LOG("bf_platform_pinctrl_init fail");
        bf_remove(g_bf_dev->pdev);
        return status;
    }
#else /*else BF_PINCTRL*/
    status = bf_platform_gpio_spi_init(g_bf_dev);
    if(status) {
        BF_LOG("bf_platform_gpio_init fail");
        bf_remove(g_bf_dev->pdev);
        return status;
    }
#endif	//BF_PINCTL
#endif
#endif

    /* Initialize the driver data */
    BF_LOG( "bf config spi ");
    g_bf_dev->spi = spi;
    /* setup SPI parameters */
    g_bf_dev->spi->mode = SPI_MODE_0;
    g_bf_dev->spi->bits_per_word = 8;
    g_bf_dev->spi->max_speed_hz = 8 * 1000 * 1000;

#if defined(PLATFORM_MTK) || defined(PLATFORM_MT6739) || defined(PLATFORM_MT676x)
    g_bf_dev->spi->controller_data = (void*)&spi_init_conf;
#endif

#if defined(BF_REE) || defined(COMPATIBLE)
    spi_setup(g_bf_dev->spi);
#endif

    spi_set_drvdata(spi, g_bf_dev);
#ifdef USE_SPI1_4GB_TEST
    spi_setup_xfer(NULL);
#endif

#ifdef CONFIG_MTK_CLK
    bf_spi_clk_enable(g_bf_dev, 1);
#endif

#if defined (COMPATIBLE)
    bf_chip_info();
    if(g_chip_type != 0x5183 && g_chip_type != 0x5283 && g_chip_type != 0x5383 && g_chip_type != 0x5483 && g_chip_type != 0x5783 && g_chip_type != 0x5B83 && g_chip_type != 0x5883 && g_chip_type != 0x5683 && g_chip_type != 0x5a83) {
        BF_LOG("ChipID:0x%x error and exit", g_chip_type);
        bf_platform_uninit(g_bf_dev);
        return -EINVAL;
    }
#endif

#if defined(TEE_BEANPOD) && defined(COMPATIBLE)
    //set_fp_vendor(FP_VENDOR_BETTERLIFE);
#endif
    BF_LOG ("--- spi probe ok --");
    return 0;
}

static const struct dev_pm_ops bf_pm = {
    .suspend = bf_spi_suspend,
    .resume =  bf_spi_resume
};

#ifdef CONFIG_OF
static struct of_device_id bf_of_spi_table[] = {
    {.compatible = "betterlife,spi",},
    {},
};
//MODULE_DEVICE_TABLE(of, bf_of_spi_table);
#endif
#ifdef REGISTER_DEVICE
static struct spi_board_info spi_board_blfp[] __initdata = {
    [0] = {
        .modalias = BF_DEV_NAME,
        .bus_num = 0,
        .chip_select = 0,
        .mode = SPI_MODE_0,
        .max_speed_hz = 6 * 1000 * 1000,
    },
};
#endif
//static struct spi_device_id bf_spi_device_id = {};
static struct spi_driver bf_spi_driver = {
    .driver = {
        .name = BF_DEV_NAME,
        .bus	= &spi_bus_type,
        .owner = THIS_MODULE,
#ifdef CONFIG_OF
        .of_match_table = bf_of_spi_table,
#endif
        .pm = &bf_pm,
    },
    .probe = bf_spi_probe,
    .remove = bf_spi_remove,
};

int32_t bf_spi_init(struct bf_device *bf_dev)
{
    int32_t error = 0;

#ifdef REGISTER_DEVICE
    error = spi_register_board_info(spi_board_blfp, ARRAY_SIZE(spi_board_blfp));
    if(error) {
        BF_LOG("spi_register_board_info fail");
        goto out;
    }
#endif
    spi_register_driver(&bf_spi_driver);
out:
    return error;
}

void bf_spi_unregister()
{
    BF_LOG("%s ++", __func__);
    spi_unregister_driver(&bf_spi_driver);
    BF_LOG("%s --", __func__);
}

