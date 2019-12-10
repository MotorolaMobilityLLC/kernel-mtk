/******************************************************************************
  Copyright (c), 1991-2007, My Company.
  
  文件: chip-fw9362.c
  描述: 
  函数: 
  版本:
         1. Author : 
            Date   : 2019-9-11
            Changes: First create file
 ******************************************************************************/

#include <linux/delay.h>

#include "ff_log.h"
#include "ff_err.h"
#include "ff_spi.h"
#include "ff_chip.h"

#undef LOG_TAG
#define LOG_TAG "focaltech:ft9362"

#define FW9362_CMD_SFR_WRITE  0x09
#define FW9362_CMD_SFR_READ   0x08
#define FW9362_CMD_SRAM_WRITE 0x05
#define FW9362_CMD_SRAM_READ  0x04
#define FW9362_CMD_FIFO_READ  0x06

#define WA_SPI_MISO_RTIGER    0xC6

typedef enum
{
    e_WorkMode_Idle = 0x00,
    e_WorkMode_Sleep,
    e_WorkMode_Fdt,
    e_WorkMode_Img,
    e_WorkMode_Nav,
    e_WorkMode_SysRst,
    e_WorkMode_AfeRst,
    e_WorkMode_FdtRst,
    e_WorkMode_FifoRst,
    e_WorkMode_OscOn,
    e_WorkMode_OscOff,
    e_WorkMode_SpiWakeUp,
    e_WorkMode_Max,
    e_Dummy
}E_WORKMODE_FW;

#define  WorkMode_Idle_Cmd     {0xC0, 0x3F, 0x00}
#define  WorkMode_Sleep_Cmd    {0xC1, 0x3E, 0x00}
#define  WorkMode_Fdt_Cmd      {0xC2, 0x3D, 0x00}
#define  WorkMode_Img_Cmd      {0xC4, 0x3B, 0x00}
#define  WorkMode_Nav_Cmd      {0xC8, 0x37, 0x00}
#define  WorkMode_SysRst_Cmd   {0xD8, 0x27, 0x00}
#define  WorkMode_AfeRst_Cmd   {0xD1, 0x2E, 0x00}
#define  WorkMode_FdtRst_Cmd   {0xD2, 0x2D, 0x00}
#define  WorkMode_FifoRst_Cmd  {0xD4, 0x2B, 0x00}
#define  WorkMode_OscOn_Cmd    {0x5A, 0xA5, 0x00}
#define  WorkMode_OscOff_Cmd   {0xA5, 0x5A, 0x00}
#define  WorkMode_SpiWakeUp    {0x70, 0x00, 0x00}

uint8_t const fw9362_WorkMode_Cmd[12][3] = {WorkMode_Idle_Cmd,\
                                   WorkMode_Sleep_Cmd,\
                                   WorkMode_Fdt_Cmd,\
                                   WorkMode_Img_Cmd,\
                                   WorkMode_Nav_Cmd,\
                                   WorkMode_SysRst_Cmd,\
                                   WorkMode_AfeRst_Cmd,\
                                   WorkMode_FdtRst_Cmd,\
                                   WorkMode_FifoRst_Cmd,\
                                   WorkMode_OscOn_Cmd,\
                                   WorkMode_OscOff_Cmd,\
                                   WorkMode_SpiWakeUp};
                                   
extern int ff_ctl_reset_device(void);

int fw9362_wm_switch(E_WORKMODE_FW workmode)
{
    int err = FF_SUCCESS;
    uint8_t ucBuffCmd[64];
    
    FF_LOGV("'%s' enter.", __func__);

    ucBuffCmd[0] = fw9362_WorkMode_Cmd[workmode][0];
    ucBuffCmd[1] = fw9362_WorkMode_Cmd[workmode][1];
    ucBuffCmd[2] = fw9362_WorkMode_Cmd[workmode][2];

    if (e_WorkMode_Max > workmode) {
        if (e_WorkMode_SpiWakeUp == workmode) {
            err = ff_spi_write_buf(ucBuffCmd, 1);
        } else {
            err = ff_spi_write_buf(ucBuffCmd, 3);
        }
    } else {
        FF_LOGI("'%s' Cmd Error.", __func__);
        return FF_ERR_INTERNAL;
    }

    FF_LOGV("'%s' leave.", __func__);
    return err;
}

void fw9362_sram_write(uint16_t addr, uint16_t data)
{
    int tx_len,dlen;
    static uint8_t tx_buffer[MAX_XFER_BUF_SIZE] = {0, };
    ff_sram_buf_t *tx_buf = TYPE_OF(ff_sram_buf_t, tx_buffer);
    
    //FF_LOGV("'%s' enter.", __func__);

    /* Sign the package.  */
    tx_buf->cmd[0] = (uint8_t)( FW9362_CMD_SRAM_WRITE);
    tx_buf->cmd[1] = (uint8_t)(~FW9362_CMD_SRAM_WRITE);

    /* Write it repeatedly. */
    dlen = 2;

    /* Packing. */
    tx_buf->addr = u16_swap_endian(addr | 0x8000); //×îžßbitÎª1
    tx_buf->dlen = u16_swap_endian((dlen/2)?(dlen/2-1):(dlen/2));
    tx_len = sizeof(ff_sram_buf_t)/2*2 + dlen;
    tx_buf->data[0] = data >> 8; 
    tx_buf->data[1] = data & 0xff;

    /* Low-level transfer. */
    ff_spi_write_buf(tx_buf, tx_len);

    //FF_LOGV("'%s' leave.", __func__);
}

uint16_t fw9362_read_sram(uint16_t addr)
{
    int tx_len;
    int dlen; 
    uint8_t ucbuff[8];
    uint16_t ustemp;
    ff_sram_buf_t tx_buffer, *tx_buf = &tx_buffer;
    uint8_t *p_data = TYPE_OF(uint8_t, ucbuff);
    // FF_LOGV("'%s' enter.", __func__);

    /* Sign the package.  */
    tx_buf->cmd[0] = (uint8_t)( FW9362_CMD_SRAM_READ);
    tx_buf->cmd[1] = (uint8_t)(~FW9362_CMD_SRAM_READ);

    /* Read it repeatedly. */
    dlen = 2;

        /* Packing. */
    tx_buf->addr = u16_swap_endian(addr | 0x8000);
    tx_buf->dlen = u16_swap_endian((dlen/2)?(dlen/2-1):(dlen/2));
    tx_len = sizeof(ff_sram_buf_t)/2*2;

        /* Low-level transfer. */
    ff_spi_write_then_read_buf(tx_buf, tx_len, p_data, dlen);

    ustemp = p_data[0];
    ustemp = (ustemp << 8) + p_data[1];

    return ustemp;
}

uint16_t fw9362_chipid_get(void)
{
    uint16_t usAddr,usData;

    usAddr = 0x3500/2 + 0x0B;
    usData = fw9362_read_sram(usAddr);

    return usData;
}

void fw9362_int_mask_set(uint16_t usdata)
{
    uint16_t usAddr;

    usAddr = 0x3500/2 + 0x03;
    fw9362_sram_write(usAddr, usdata);
}

void fw9362_intflag_clear(uint16_t usdata)
{
    uint16_t usAddr;

    usAddr = 0x3500/2 + 0x04;
    fw9362_sram_write(usAddr, usdata);
} 

void fw9362_sfr_write(uint8_t addr, uint8_t data)
{
    ff_sfr_buf_t tx_buf = {{0x00},0,0,0 };
    tx_buf.cmd[0]  = (uint8_t)( FW9362_CMD_SFR_WRITE);
    tx_buf.cmd[1]  = (uint8_t)(~FW9362_CMD_SFR_WRITE);
    tx_buf.addr    = addr;
    tx_buf.tx_byte = data;
    ff_spi_write_buf(&tx_buf, 4);
}

uint16_t fw9362_probe_id(void)
{
    int err = FF_SUCCESS;
    uint16_t device_id = 0;
    FF_LOGV("'%s' enter. TA", __func__);

    err = ff_ctl_reset_device();
	FF_LOGI("ff_ctl_reset_device= : %d ", err);
	err = fw9362_wm_switch(e_WorkMode_OscOn);
	FF_LOGI("e_WorkMode_OscOn= : %d ", err);
	err = fw9362_wm_switch(e_WorkMode_Idle);
	FF_LOGI("e_WorkMode_Idle= : %d ", err);
	mdelay(5);
	fw9362_sfr_write(WA_SPI_MISO_RTIGER, 0x01); //spi mode 00
	device_id = fw9362_chipid_get();
	FF_LOGI("chip id is = : 0x%04x ", device_id);
	if(device_id != 0x9362) device_id = 0;
	return device_id;
}

