#include "gf_common.h"
#include "gf_spi.h"

#if defined(USE_SPI_BUS)
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#elif defined(USE_PLATFORM_BUS)
#include <linux/platform_device.h>
#endif


/******************** Function Definitions *************************/

int gf_spi_read_bytes(struct gf_dev* gf_dev, unsigned short addr, unsigned short data_len,
                      unsigned char* rx_buf)
{

    struct spi_message msg;
    struct spi_transfer *xfer;
    u32 package_num = (data_len + 1 + 1)>>MTK_SPI_ALIGN_MASK_NUM;
    u32 reminder = (data_len + 1 + 1) & MTK_SPI_ALIGN_MASK;
    u8 *one_more_buff = NULL;
    u8 twice = 0;
    int ret = 0;

    one_more_buff = kzalloc(30720, GFP_KERNEL);
    if(one_more_buff == NULL ) {
        pr_info("No memory for one_more_buff.");
        return -ENOMEM;
    }
    xfer = kzalloc(sizeof(*xfer)*2, GFP_KERNEL);
    if((package_num > 0) && (reminder != 0)) {
        twice = 1;
        printk("stone package_num is %d reminder is %d\n",package_num,reminder);
    } else {
        twice = 0;
    }
    if( xfer == NULL) {
        pr_info("No memory for command.");
        if(one_more_buff != NULL)
            kfree(one_more_buff);
        return -ENOMEM;
    }
    /*set spi mode.*/
    /* switch to DMA mode if transfer length larger than 32 bytes */
    if ((data_len + 1) > 32)
	gf_dev->spi_mcc.com_mod = DMA_TRANSFER;
    else
	gf_dev->spi_mcc.com_mod = FIFO_TRANSFER;

    spi_setup(gf_dev->spi);
    spi_message_init(&msg);
    /*send GF command to device.*/
    rx_buf[0] = GF_W;
    rx_buf[1] = (u8)((addr >> 8)&0xFF);
    rx_buf[2] = (u8)(addr & 0xFF);
    xfer[0].tx_buf = rx_buf;
    xfer[0].len = 3;
    spi_message_add_tail(&xfer[0], &msg);
    spi_sync(gf_dev->spi, &msg);

    spi_message_init(&msg);
    /*if wanted to read data from GF.
     *Should write Read command to device
     *before read any data from device.
     */
    //memset(rx_buf, 0xff, data_len);
    one_more_buff[0] = GF_R;
    xfer[1].tx_buf = &one_more_buff[0];
    xfer[1].rx_buf = &one_more_buff[0];
    //read 1 additional package to ensure no even data read
    if(twice == 1)
        xfer[1].len = ((package_num+1) << MTK_SPI_ALIGN_MASK_NUM);
    else
        xfer[1].len = data_len + 1;
    spi_message_add_tail(&xfer[1], &msg);
    ret = spi_sync(gf_dev->spi, &msg);
    if(ret == 0) {
        memcpy(rx_buf + GF_RDATA_OFFSET,one_more_buff+1,data_len);
        ret = data_len;
    } else {
        pr_info("gf: read failed. ret = %d", ret);
    }
    if(xfer != NULL) {
        kfree(xfer);
        xfer = NULL;
    }
    if(one_more_buff != NULL) {
        kfree(one_more_buff);
        one_more_buff = NULL;
    }
    return ret;
}

int gf_spi_write_bytes(struct gf_dev* gf_dev, unsigned short addr, unsigned short data_len,
                       unsigned char* tx_buf)
{
    struct spi_message msg;
    struct spi_transfer *xfer;
    u32 package_num = (data_len + 2*GF_WDATA_OFFSET)>>MTK_SPI_ALIGN_MASK_NUM;
    u32 reminder = (data_len + 2*GF_WDATA_OFFSET) & MTK_SPI_ALIGN_MASK;
    u8 *reminder_buf = NULL;
    u8 twice = 0;
    int ret = 0;

    /*set spi mode.*/
    if ((data_len + 1) > 32)
        gf_dev->spi_mcc.com_mod = DMA_TRANSFER;
    else
	gf_dev->spi_mcc.com_mod = FIFO_TRANSFER;

    spi_setup(gf_dev->spi);

    if((package_num > 0) && (reminder != 0)) {
        twice = 1;
        /*copy the reminder data to temporarity buffer.*/
        reminder_buf = kzalloc(reminder + GF_WDATA_OFFSET, GFP_KERNEL);
        if(reminder_buf == NULL ) {
            pr_info("gf:No memory for exter data.");
            return -ENOMEM;
        }
        memcpy(reminder_buf + GF_WDATA_OFFSET, tx_buf + 2*GF_WDATA_OFFSET+data_len - reminder, reminder);
        pr_info("gf:w-reminder:0x%x-0x%x,0x%x", reminder_buf[GF_WDATA_OFFSET],reminder_buf[GF_WDATA_OFFSET+1],
                reminder_buf[GF_WDATA_OFFSET + 2]);
        xfer = kzalloc(sizeof(*xfer)*2, GFP_KERNEL);
    } else {
        twice = 0;
        xfer = kzalloc(sizeof(*xfer), GFP_KERNEL);
    }
    if(xfer == NULL) {
        pr_info("gf:No memory for command.");
        if(reminder_buf != NULL)
            kfree(reminder_buf);
        return -ENOMEM;
    }

    /*if the length is not align with 1024. Need 2 transfer at least.*/
    spi_message_init(&msg);
    tx_buf[0] = GF_W;
    tx_buf[1] = (u8)((addr >> 8)&0xFF);
    tx_buf[2] = (u8)(addr & 0xFF);
    xfer[0].tx_buf = tx_buf;
    if(twice == 1) {
        xfer[0].len = package_num << MTK_SPI_ALIGN_MASK_NUM;
        spi_message_add_tail(&xfer[0], &msg);
        addr += (data_len - reminder + GF_WDATA_OFFSET);
        reminder_buf[0] = GF_W;
        reminder_buf[1] = (u8)((addr >> 8)&0xFF);
        reminder_buf[2] = (u8)(addr & 0xFF);
        xfer[1].tx_buf = reminder_buf;
        xfer[1].len = reminder + 2*GF_WDATA_OFFSET;
        spi_message_add_tail(&xfer[1], &msg);
    } else {
        xfer[0].len = data_len + GF_WDATA_OFFSET;
        spi_message_add_tail(&xfer[0], &msg);
    }
    ret = spi_sync(gf_dev->spi, &msg);
    if(ret == 0) {
        if(twice == 1)
            ret = msg.actual_length - 2*GF_WDATA_OFFSET;
        else
            ret = msg.actual_length - GF_WDATA_OFFSET;
    } else 	{
        pr_info("gf:write async failed. ret = %d", ret);
    }

    if(xfer != NULL) {
        kfree(xfer);
        xfer = NULL;
    }
    if(reminder_buf != NULL) {
        kfree(reminder_buf);
        reminder_buf = NULL;
    }

    return ret;
}

int gf_spi_read_word(struct gf_dev* gf_dev, unsigned short addr, unsigned short* value)
{
    int status = 0;
    u8* buf = NULL;
    mutex_lock(&gf_dev->buf_lock);
    status = gf_spi_read_bytes(gf_dev, addr, 2, gf_dev->gBuffer);
    buf = gf_dev->gBuffer + GF_RDATA_OFFSET;
    *value = ((unsigned short)(buf[0]<<8)) | buf[1];
    mutex_unlock(&gf_dev->buf_lock);
    return status;
}

int gf_spi_write_word(struct gf_dev* gf_dev, unsigned short addr, unsigned short value)
{
    int status = 0;
    mutex_lock(&gf_dev->buf_lock);
    gf_dev->gBuffer[GF_WDATA_OFFSET] = 0x00;
    gf_dev->gBuffer[GF_WDATA_OFFSET+1] = 0x01;
    gf_dev->gBuffer[GF_WDATA_OFFSET+2] = (u8)(value>>8);
    gf_dev->gBuffer[GF_WDATA_OFFSET+3] = (u8)(value & 0x00ff);
    status = gf_spi_write_bytes(gf_dev, addr, 4, gf_dev->gBuffer);
    mutex_unlock(&gf_dev->buf_lock);

    return status;
}

void endian_exchange(int len, unsigned char* buf)
{
    int i;
    u8 buf_tmp;
    for(i=0; i< len/2; i++)
    {
        buf_tmp = buf[2*i+1];
        buf[2*i+1] = buf[2*i] ;
        buf[2*i] = buf_tmp;
    }
}

int gf_spi_read_data(struct gf_dev* gf_dev, unsigned short addr, int len, unsigned char* value)
{
    int status;

    mutex_lock(&gf_dev->buf_lock);
    status = gf_spi_read_bytes(gf_dev, addr, len, gf_dev->gBuffer);
    memcpy(value, gf_dev->gBuffer+GF_RDATA_OFFSET, len);
    mutex_unlock(&gf_dev->buf_lock);

    return status;
}

int gf_spi_read_data_bigendian(struct gf_dev* gf_dev, unsigned short addr,int len, unsigned char* value)
{
    int status;

    mutex_lock(&gf_dev->buf_lock);
    status = gf_spi_read_bytes(gf_dev, addr, len, gf_dev->gBuffer);
    memcpy(value, gf_dev->gBuffer+GF_RDATA_OFFSET, len);
    mutex_unlock(&gf_dev->buf_lock);

    endian_exchange(len,value);
    return status;
}

int gf_spi_write_data(struct gf_dev* gf_dev, unsigned short addr, int len, unsigned char* value)
{
    int status =0;
    unsigned short addr_len = 0;
    unsigned char* buf = NULL;

    if (len > 1024 * 10) {
        pr_err("%s length is large.\n",__func__);
        return -1;
    }

    addr_len = len / 2;

    buf = kzalloc(len + 2, GFP_KERNEL);
    if (buf == NULL) {
        pr_err("%s, No memory for gBuffer.\n",__func__);
        return -ENOMEM;
    }

    buf[0] = (unsigned char) ((addr_len & 0xFF00) >> 8);
    buf[1] = (unsigned char) (addr_len & 0x00FF);
    memcpy(buf+2, value, len);
    endian_exchange(len,buf+2);
    mutex_lock(&gf_dev->buf_lock);
    memcpy(gf_dev->gBuffer+GF_WDATA_OFFSET, buf, len+2);
    kfree(buf);

    status = gf_spi_write_bytes(gf_dev, addr, len+2, gf_dev->gBuffer);
    mutex_unlock(&gf_dev->buf_lock);

    return status;
}

int gf_spi_send_cmd(struct gf_dev* gf_dev, unsigned char* cmd, int len)
{
    struct spi_message msg;
    struct spi_transfer *xfer;
    int ret;

    gf_dev->spi_mcc.com_mod = FIFO_TRANSFER;
    spi_setup(gf_dev->spi);
    spi_message_init(&msg);
    xfer = kzalloc(sizeof(*xfer), GFP_KERNEL);
    if( xfer == NULL) {
        pr_err("%s, No memory for command.\n",__func__);
        return -ENOMEM;
    }
    xfer->tx_buf = cmd;
    xfer->len = len;
    spi_message_add_tail(xfer, &msg);
    ret = spi_sync(gf_dev->spi, &msg);

    kfree(xfer);
    if(xfer != NULL)
        xfer = NULL;

    return ret;
}
