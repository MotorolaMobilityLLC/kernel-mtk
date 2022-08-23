/*
 * Copyright (c) 2022 MediaTek Inc.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#define LOG_TAG "LCM"

#include <debug.h>
#include <gpio_api.h>
#include <lib/kcmdline.h>
#include <mt_i2c.h>
#include <platform_i2c.h>

#include "lcm_drv.h"
#include "lcm_util.h"

#define LCM_LOGE(string, args...)  dprintf(CRITICAL, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(INFO, "[LK/"LOG_TAG"]"string, ##args)

#define GPIO_LED_BIAS_EN GPIO87
#define GPIO_LCD_RESET GPIO85
#define GPIO_LCM_ID0  GPIO109
#define GPIO_LCM_ID1  GPIO98
#define GPIO_LCD_BIAS_ENP GPIO82
#define GPIO_LCD_BIAS_ENN GPIO81
static const unsigned int BL_MIN_LEVEL = 20;
static LCM_UTIL_FUNCS lcm_util;
#define BIAS_I2C_ADDRESS (0x11)

#define SET_RESET_PIN(v, fdt)    (lcm_util.set_reset_pin(v, fdt))
#define MDELAY(n)       (lcm_util.mdelay(n))
#define UDELAY(n)       (lcm_util.udelay(n))


/* --------------------------------------------------------------------------- */
/* Local Functions */
/* --------------------------------------------------------------------------- */

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
        lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
        lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
        lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) \
        lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
        lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

/* --------------------------------------------------------------------------- */
/* Local Constants */
/* --------------------------------------------------------------------------- */
#define LCM_DSI_CMD_MODE                                    0
#define FRAME_WIDTH                                     (1200)
#define FRAME_HEIGHT                                    (2000)

#define REGFLAG_DELAY       0xFFFC
#define REGFLAG_UDELAY  0xFFFB
#define REGFLAG_END_OF_TABLE    0xFFFD
#define REGFLAG_RESET_LOW   0xFFFE
#define REGFLAG_RESET_HIGH  0xFFFF

static int i2c_write_bytes(int bus_id, uint8_t dev_addr, uint8_t reg_addr, uint8_t value)
{
    s32 ret = 0;
    u8 write_data[2] = { 0 };
    u8 len;
    int i2c_id = bus_id & 0xff;
    int i2c_addr = dev_addr & 0xff;

    /* pkt data */
    write_data[0] = reg_addr;
    write_data[1] = value;
    len = 2;

    ret = mtk_i2c_write((u8)i2c_id, (u8)i2c_addr, write_data, len, NULL);
    if (ret < 0)
        LCM_LOGE("[LCM][ERROR] %s: %d\n", __func__, ret);

    return ret;
}

static int i2c_read_bytes(int bus_id, uint8_t dev_addr, uint8_t reg_addr, size_t count)
{
    s32 ret = 0;
    u8 write_data[2] = { 0 };

    write_data[0] = reg_addr;

    if (bus_id < 0) {
        LCM_LOGE("bus is invalid(negative).\n");
        return -1;
    }

    if (count == 0) {
        LCM_LOGE("count is invalid(zero).\n");
        return -1;
    }

    ret = mtk_i2c_read(bus_id, dev_addr, (u8 *)write_data, count, NULL);
    if (ret < 0)
        LCM_LOGE("[LCM][ERROR] %s: %d\n", __func__, ret);

    return write_data[0];
}

static int i2c_read_write(int bus_id, uint8_t dev_addr, uint8_t reg_addr, size_t count)
{
    s32 ret = 0;
    u8 write_data[2] = { 0 };

    write_data[0] = reg_addr;

    if (bus_id < 0) {
        LCM_LOGE("bus is invalid(negative).\n");
        return -1;
    }

    if (count == 0) {
        LCM_LOGE("count is invalid(zero).\n");
        return -1;
    }

    ret = mtk_i2c_write_read(5, BIAS_I2C_ADDRESS, write_data, 1, 1, NULL);
    if (ret < 0)
        LCM_LOGE("[LCM][ERROR] %s: %d\n", __func__, ret);

    return write_data[0];
}

static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
    mt_set_gpio_mode(GPIO, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO, (output > 0) ? GPIO_OUT_ONE : GPIO_OUT_ZERO);
}

struct LCM_setting_table {
    unsigned int cmd;
    unsigned char count;
    unsigned char para_list[200];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
    {0x28, 0, {} },
    {REGFLAG_DELAY, 20, {} },
    {0x10, 0, {} },
    {REGFLAG_DELAY, 120, {} },
};

static struct LCM_setting_table init_setting_cmd[] = {
    {0xB9, 5, {0x83, 0x10, 0x21, 0x55, 0x00} },
    {0xB1, 26, {0x2C, 0xB3, 0xB3, 0x2F, 0xEF, 0x43, 0xE1,
        0x50, 0x36, 0x36, 0x36, 0x36, 0x1A, 0x8B, 0x11,
        0x65, 0x00, 0x88, 0xFA, 0xFF, 0xFF, 0x8F, 0xFF,
        0x08, 0x9A, 0x33} },
    {0xB2, 15, {0x00, 0x47, 0xB0, 0xD0, 0x00, 0x12, 0xC4,
        0x3C, 0x36, 0x32, 0x30, 0x10, 0x00, 0x88, 0xF5} },
    {0xB4, 12, {0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x49,
        0x3F, 0x7C, 0x70, 0x01, 0x49} },
    {0xE9, 1, {0xCD} },
    {0xBA, 1, {0x84} },
    {0xE9, 1, {0x3F} },
    {0xBC, 2, {0x1B, 0x04} },
    {0xBE, 1, {0x20} },
    {0xC0, 10, {0x3A, 0x3A, 0x22, 0x11, 0x22, 0xA0, 0x61,
        0x08, 0xF5, 0x03} },
    {0xE9, 1, {0xCC} },
    {0xC7, 1, {0x80} },
    {0xE9, 1, {0x3F} },
    {0xE9, 1, {0xC6} },
    {0xC8, 1, {0x97} },
    {0xE9, 1, {0x3F} },
    {0xCB, 6, {0x08, 0x13, 0x07, 0x00, 0x07, 0x0B} },
    {0xCC, 3, {0x02, 0x03, 0x4C} },
    {0xE9, 1, {0xC4} },
    {0xD0, 1, {0x03} },
    {0xE9, 1, {0x3F} },
    {0xD1, 7, {0x37, 0x06, 0x00, 0x02, 0x04, 0x0C, 0xFF} },
    {0xD3, 34, {0x06, 0x00, 0x00, 0x00, 0x40, 0x04, 0x08,
        0x04, 0x08, 0x37, 0x47, 0x64, 0x4B, 0x11, 0x11,
        0x03, 0x03, 0x32, 0x10, 0x0C, 0x00, 0x0C, 0x32,
        0x10, 0x08, 0x00, 0x08, 0x32, 0x17, 0xE7, 0x07,
        0xE7, 0x00, 0x00} },
    {0xD5, 44, {0x24, 0x24, 0x24, 0x24, 0x07, 0x06, 0x07,
        0x06, 0x05, 0x04, 0x05, 0x04, 0x03, 0x02, 0x03,
        0x02, 0x01, 0x00, 0x01, 0x00, 0x21, 0x20, 0x21,
        0x20, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x1F, 0x1F, 0x1F, 0x1F, 0x1E, 0x1E, 0x1E,
        0x1E, 0x18, 0x18, 0x18, 0x18} },
    {0xD8, 12, {0xAA, 0xAA, 0xAA, 0xAA, 0xFA, 0xA0, 0xAA,
        0xAA, 0xAA, 0xAA, 0xFA, 0xA0} },
    {0xE0, 46, {0x00, 0x04, 0x0D, 0x15, 0x1D, 0x32, 0x4F,
        0x58, 0x62, 0x5F, 0x7D, 0x85, 0x8A, 0x9A, 0x9A,
        0x9E, 0xA5, 0xB6, 0xB2, 0x56, 0x5E, 0x67, 0x73,
        0x00, 0x04, 0x0D, 0x15, 0x1D, 0x32, 0x4F, 0x58,
        0x62, 0x5F, 0x7D, 0x85, 0x8A, 0x9A, 0x9A, 0x9E,
        0xA5, 0xB6, 0xB2, 0x56, 0x5E, 0x67, 0x73} },
    {0xE7, 23, {0x28, 0x08, 0x08, 0x3B, 0x3F, 0x49, 0x03,
        0xC4, 0x48, 0x14, 0x14, 0x00, 0x00, 0x00, 0x00,
        0x12, 0x05, 0x02, 0x02, 0x08, 0x33, 0x03, 0x84} },
    {0xE1, 63, {0x12, 0x00, 0x00, 0x89, 0x30, 0x80, 0x07,
        0xD0, 0x02, 0x58, 0x00, 0x05, 0x02, 0x58, 0x02,
        0x58, 0x02, 0x00, 0x02, 0x2C, 0x00, 0x20, 0x00,
        0x75, 0x00, 0x08, 0x00, 0x0C, 0x18, 0x00, 0x12,
        0x4E, 0x18, 0x00, 0x10, 0xF0, 0x03, 0x0C, 0x20,
        0x00, 0x06, 0x0B, 0x0B, 0x33, 0x0E, 0x1C, 0x2A,
        0x38, 0x46, 0x54, 0x62, 0x69, 0x70, 0x77, 0x79,
        0x7B, 0x7D, 0x7E, 0x01, 0x02, 0x01, 0x00, 0x09} },
    {0xBD, 1, {0x01} },
    {0xE1, 25, {0x40, 0x09, 0xBE, 0x19, 0xFC, 0x19, 0xFA,
        0x19, 0xF8, 0x1A, 0x38, 0x1A, 0x78, 0x1A, 0xB6,
        0x2A, 0xF6, 0x2B, 0x34, 0x2B, 0x74, 0x3B, 0x74, 0x6B, 0x74} },
    {0xB1, 3, {0x01, 0xBF, 0x11} },
    {0xCB, 1, {0x86} },
    {0xD2, 1, {0xF0} },
    {0xE9, 1, {0xC9} },
    {0xD3, 1, {0x84} },
    {0xE9, 1, {0x3F} },
    {0xD8, 36, {0xFF, 0xFF, 0xFA, 0xAF, 0xFF, 0xA0, 0xFF,
        0xFF, 0xFA, 0xAF, 0xFF, 0xA0, 0xFF, 0xFF, 0xFA,
        0xAF, 0xFF, 0xA0, 0xFF, 0xFF, 0xFA, 0xAF, 0xFF,
        0xA0, 0xFF, 0xFF, 0xFA, 0xAF, 0xFF, 0xA0, 0xFF,
        0xFF, 0xFA, 0xAF, 0xFF, 0xA0} },
    {0xE2, 56, {0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x4D,
        0x43, 0x7C, 0x70, 0x04, 0x68, 0x01, 0x4D, 0x01,
        0x00, 0x20, 0x01, 0x03, 0x00, 0x00, 0x0F, 0x0F,
        0x0F, 0x0F, 0x00, 0x00, 0x00, 0x1F, 0x0E, 0x0E,
        0x32, 0x3C, 0x4C, 0x04, 0xFE, 0x4C, 0x14, 0x14,
        0x00, 0x00, 0xBF, 0x01, 0x33, 0x47, 0x33, 0x48,
        0x00, 0x20, 0x4D, 0x00, 0x36, 0x00, 0x07, 0x0B, 0x00} },
    {0xE7, 11, {0x02, 0x00, 0xBC, 0x01, 0x14, 0x07, 0xAA,
        0x08, 0xA0, 0x04, 0x00} },
    {0xBD, 1, {0x02} },
    {0xD8, 12, {0xFF, 0xFF, 0xFA, 0xAF, 0xFF, 0xA0, 0xFF,
        0xFF, 0xFA, 0xAF, 0xFF, 0xA0} },
    {0xE2, 56, {0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x4B,
        0x41, 0x7C, 0x70, 0x04, 0x68, 0x01, 0x4B, 0x01,
        0x00, 0x20, 0x01, 0x03, 0x00, 0x00, 0x0F, 0x0F,
        0x0F, 0x0F, 0x00, 0x00, 0x00, 0x25, 0x1C, 0x1C,
        0x38, 0x40, 0x4B, 0x02, 0xE8, 0x8B, 0x14, 0x14,
        0x00, 0x00, 0xC9, 0x01, 0x33, 0x47, 0x33, 0x48,
        0x00, 0x20, 0x4B, 0x00, 0x36, 0x00, 0x07, 0x0B, 0x00} },
    {0xE7, 28, {0xFE, 0x03, 0xFE, 0x03, 0xFE, 0x03, 0x02,
        0x02, 0x02, 0x25, 0x00, 0x25, 0x81, 0x02, 0x40,
        0x00, 0x20, 0x49, 0x04, 0x03, 0x02, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x01, 0x00} },
    {0xBD, 1, {0x03} },
    {0xD8, 36, {0xAA, 0xAA, 0xAA, 0xAA, 0xFA, 0xA0, 0xAA,
        0xAA, 0xAA, 0xAA, 0xFA, 0xA0, 0xFF, 0xFF, 0xFA,
        0xAF, 0xFF, 0xA0, 0xFF, 0xFF, 0xFA, 0xAF, 0xFF,
        0xA0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x50, 0x55,
        0x55, 0x55, 0x55, 0x55, 0x50} },
    {0xE1, 1, {0x01} },
    {0xBD, 1, {0x00} },
    {0xE9, 1, {0xC4} },
    {0xBA, 1, {0x96} },
    {0xE9, 1, {0x3F} },
    {0xBD, 1, {0x01} },
    {0xE9, 1, {0xC5} },
    {0xBA, 1, {0x4F} },
    {0xE9, 1, {0x3F} },
    {0xBD, 1, {0x00} },
    {0xC1, 1, {0x01} },
    {0xBD, 1, {0x03} },
    {0xC1, 58, {0x00, 0x04, 0x08, 0x0B, 0x10, 0x14, 0x18,
        0x1C, 0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x39,
        0x3E, 0x43, 0x47, 0x4B, 0x4F, 0x56, 0x5D, 0x65,
        0x6D, 0x74, 0x7C, 0x84, 0x8B, 0x94, 0x9C, 0xA4,
        0xAB, 0xB4, 0xBB, 0xC3, 0xCB, 0xD2, 0xDA, 0xE3,
        0xEB, 0xF3, 0xF7, 0xF9, 0xFB, 0xFD, 0xFF, 0x07,
        0x05, 0x92, 0x64, 0x69, 0xB0, 0x03, 0x63, 0x14,
        0xF4, 0x6F, 0xC0} },
    {0xBD, 1, {0x02} },
    {0xC1, 58, {0x00, 0x03, 0x08, 0x0B, 0x10, 0x14, 0x18,
        0x1C, 0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x39,
        0x3E, 0x43, 0x47, 0x4B, 0x4F, 0x56, 0x5D, 0x65,
        0x6D, 0x74, 0x7C, 0x84, 0x8C, 0x94, 0x9D, 0xA4,
        0xAC, 0xB4, 0xBB, 0xC3, 0xCC, 0xD4, 0xDC, 0xE4,
        0xEC, 0xF4, 0xF8, 0xFA, 0xFC, 0xFE, 0xFF, 0x17,
        0x05, 0x92, 0x64, 0x69, 0xB0, 0xFA, 0xC5, 0x69,
        0x05, 0x04, 0x00} },
    {0xBD, 1, {0x01} },
    {0xC1, 58, {0x00, 0x04, 0x08, 0x0B, 0x10, 0x14, 0x18,
        0x1C, 0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x39,
        0x3E, 0x43, 0x47, 0x4B, 0x4F, 0x56, 0x5D, 0x65,
        0x6D, 0x74, 0x7C, 0x84, 0x8C, 0x94, 0x9D, 0xA4,
        0xAC, 0xB4, 0xBB, 0xC3, 0xCC, 0xD4, 0xDC, 0xE4,
        0xEC, 0xF4, 0xF8, 0xFA, 0xFC, 0xFE, 0xFF, 0x17,
        0x05, 0x92, 0x64, 0x69, 0xB0, 0xFA, 0xC5, 0x69,
        0x05, 0x04, 0x00} },
    {0xBD, 1, {0x00} },
    {0x51, 2, {0x0F, 0xFF} },
    {0x53, 1, {0x2C} },
    {0xC9, 5, {0x00, 0x1E, 0x13, 0x88, 0x01} },
//    {0x5E, 2, {0x00, 0x30} },
    {0x35, 0, {} },
    {0xB9, 0, {} },
    {0x11, 0, {} },
    {REGFLAG_DELAY, 120, {} },
    {0x29, 0, {} },
    {REGFLAG_DELAY, 20, {} },
};

static struct LCM_setting_table bl_level[] = {
    {0x51, 2, {0x0F, 0xFF} },
    {REGFLAG_END_OF_TABLE, 0x00, {} }
};

static void push_table(struct LCM_setting_table *table, unsigned int count,
                       unsigned char force_update)
{
    unsigned int i;
    unsigned int cmd;

    for (i = 0; i < count; i++) {
        cmd = table[i].cmd;
        switch (cmd) {
        case REGFLAG_DELAY:
            if (table[i].count <= 10)
                MDELAY(table[i].count);
            else
                MDELAY(table[i].count);
        break;
        case REGFLAG_UDELAY:
            UDELAY(table[i].count);
            break;
        case REGFLAG_END_OF_TABLE:
            break;
        default:
            dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list,
                            force_update);
            break;
        }
    }
}

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));

    params->type   = LCM_TYPE_DSI;

    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;

    params->dsi.mode = SYNC_PULSE_VDO_MODE;
    // DSI
    /* Command mode setting */
    params->dsi.LANE_NUM = LCM_FOUR_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
    params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

    params->dsi.vertical_sync_active = 8;
    params->dsi.vertical_backporch = 12;
    params->dsi.vertical_frontporch = 198;
    params->dsi.vertical_active_line = FRAME_HEIGHT;//hight

    params->dsi.horizontal_sync_active = 20;
    params->dsi.horizontal_backporch = 46;
    params->dsi.horizontal_frontporch = 60;
    params->dsi.horizontal_active_pixel = FRAME_WIDTH;//=wight
    params->dsi.IsCphy = 0;

    params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

    params->dsi.clk_lp_per_line_enable = 0;
    params->dsi.esd_check_enable = 1;
    params->dsi.customization_esd_check_enable = 0;

    params->dsi.data_rate = 960;
    params->dsi.data_rate_khz = 960298;
    params->dsi.ssc_disable = 1;
    //D2 <-> D3
    params->dsi.lane_swap_en = 0;
    params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_0] = MIPITX_PHY_LANE_0;
    params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_1] = MIPITX_PHY_LANE_1;
    params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_2] = MIPITX_PHY_LANE_3;
    params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_3] = MIPITX_PHY_LANE_2;
    params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_CK] = MIPITX_PHY_LANE_CK;
    params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_RX] = MIPITX_PHY_LANE_0;

#ifdef MTK_RUNTIME_SWITCH_FPS_SUPPORT
    params->dsi.fps = 120;
#endif

    params->rotate = MTK_PANEL_ROTATE_180;

#ifdef MTK_ROUND_CORNER_SUPPORT
    params->round_corner_params.round_corner_en = 0;
    params->round_corner_params.full_content = 0;
    params->round_corner_params.h = ROUND_CORNER_H_TOP;
    params->round_corner_params.h_bot = ROUND_CORNER_H_BOT;
    params->round_corner_params.tp_size = sizeof(top_rc_pattern);
    params->round_corner_params.lt_addr = (void *)top_rc_pattern;
#endif

    params->dsi.dsc_enable = 1;
    params->dsi.dsc_params.ver = 18;
    params->dsi.dsc_params.slice_mode = 1;
    params->dsi.dsc_params.rgb_swap = 0;
    params->dsi.dsc_params.dsc_cfg = 34;
    params->dsi.dsc_params.rct_on = 1;
    params->dsi.dsc_params.bit_per_channel = 8;
    params->dsi.dsc_params.dsc_line_buf_depth = 9;
    params->dsi.dsc_params.bp_enable = 1;
    params->dsi.dsc_params.bit_per_pixel = 128;
    params->dsi.dsc_params.pic_height = 2000;
    params->dsi.dsc_params.pic_width = 1200;
    params->dsi.dsc_params.slice_height = 5;
    params->dsi.dsc_params.slice_width = 600;
    params->dsi.dsc_params.chunk_size = 600;//
    params->dsi.dsc_params.xmit_delay = 512;
    params->dsi.dsc_params.dec_delay = 556;
    params->dsi.dsc_params.scale_value = 32;
    params->dsi.dsc_params.increment_interval = 117;
    params->dsi.dsc_params.decrement_interval = 8;
    params->dsi.dsc_params.line_bpg_offset = 12;
    params->dsi.dsc_params.nfl_bpg_offset = 6144;
    params->dsi.dsc_params.slice_bpg_offset = 4686;
    params->dsi.dsc_params.initial_offset = 6144;
    params->dsi.dsc_params.final_offset = 4336;
    params->dsi.dsc_params.flatness_minqp = 3;
    params->dsi.dsc_params.flatness_maxqp = 12;
    params->dsi.dsc_params.rc_model_size = 8192;
    params->dsi.dsc_params.rc_edge_factor = 6;
    params->dsi.dsc_params.rc_quant_incr_limit0 = 11;
    params->dsi.dsc_params.rc_quant_incr_limit1 = 11;
    params->dsi.dsc_params.rc_tgt_offset_hi = 3;
    params->dsi.dsc_params.rc_tgt_offset_lo = 3;
    params->dsi.dsc_params.rc_buf_thresh[0] = 14;
    params->dsi.dsc_params.rc_buf_thresh[1] = 28;
    params->dsi.dsc_params.rc_buf_thresh[2] = 42;
    params->dsi.dsc_params.rc_buf_thresh[3] = 56;
    params->dsi.dsc_params.rc_buf_thresh[4] = 70;
    params->dsi.dsc_params.rc_buf_thresh[5] = 84;
    params->dsi.dsc_params.rc_buf_thresh[6] = 98;
    params->dsi.dsc_params.rc_buf_thresh[7] = 105;
    params->dsi.dsc_params.rc_buf_thresh[8] = 112;
    params->dsi.dsc_params.rc_buf_thresh[9] = 119;
    params->dsi.dsc_params.rc_buf_thresh[10] = 121;
    params->dsi.dsc_params.rc_buf_thresh[11] = 123;
    params->dsi.dsc_params.rc_buf_thresh[12] = 125;
    params->dsi.dsc_params.rc_buf_thresh[13] = 126;
    params->dsi.dsc_params.rc_range_parameters[0].range_min_qp = 0;
    params->dsi.dsc_params.rc_range_parameters[0].range_max_qp = 4;
    params->dsi.dsc_params.rc_range_parameters[0].range_bpg_offset = 2;
    params->dsi.dsc_params.rc_range_parameters[1].range_min_qp = 0;
    params->dsi.dsc_params.rc_range_parameters[1].range_max_qp = 4;
    params->dsi.dsc_params.rc_range_parameters[1].range_bpg_offset = 0;
    params->dsi.dsc_params.rc_range_parameters[2].range_min_qp = 1;
    params->dsi.dsc_params.rc_range_parameters[2].range_max_qp = 5;
    params->dsi.dsc_params.rc_range_parameters[2].range_bpg_offset = 0;
    params->dsi.dsc_params.rc_range_parameters[3].range_min_qp = 1;
    params->dsi.dsc_params.rc_range_parameters[3].range_max_qp = 6;
    params->dsi.dsc_params.rc_range_parameters[3].range_bpg_offset = -2;
    params->dsi.dsc_params.rc_range_parameters[4].range_min_qp = 3;
    params->dsi.dsc_params.rc_range_parameters[4].range_max_qp = 7;
    params->dsi.dsc_params.rc_range_parameters[4].range_bpg_offset = -4;
    params->dsi.dsc_params.rc_range_parameters[5].range_min_qp = 3;
    params->dsi.dsc_params.rc_range_parameters[5].range_max_qp = 7;
    params->dsi.dsc_params.rc_range_parameters[5].range_bpg_offset = -6;
    params->dsi.dsc_params.rc_range_parameters[6].range_min_qp = 3;
    params->dsi.dsc_params.rc_range_parameters[6].range_max_qp = 7;
    params->dsi.dsc_params.rc_range_parameters[6].range_bpg_offset = -8;
    params->dsi.dsc_params.rc_range_parameters[7].range_min_qp = 3;
    params->dsi.dsc_params.rc_range_parameters[7].range_max_qp = 8;
    params->dsi.dsc_params.rc_range_parameters[7].range_bpg_offset = -8;
    params->dsi.dsc_params.rc_range_parameters[8].range_min_qp = 3;
    params->dsi.dsc_params.rc_range_parameters[8].range_max_qp = 9;
    params->dsi.dsc_params.rc_range_parameters[8].range_bpg_offset = -8;
    params->dsi.dsc_params.rc_range_parameters[9].range_min_qp = 3;
    params->dsi.dsc_params.rc_range_parameters[9].range_max_qp = 10;
    params->dsi.dsc_params.rc_range_parameters[9].range_bpg_offset = -10;
    params->dsi.dsc_params.rc_range_parameters[10].range_min_qp = 5;
    params->dsi.dsc_params.rc_range_parameters[10].range_max_qp = 11;
    params->dsi.dsc_params.rc_range_parameters[10].range_bpg_offset = -10;
    params->dsi.dsc_params.rc_range_parameters[11].range_min_qp = 5;
    params->dsi.dsc_params.rc_range_parameters[11].range_max_qp = 12;
    params->dsi.dsc_params.rc_range_parameters[11].range_bpg_offset = -12;
    params->dsi.dsc_params.rc_range_parameters[12].range_min_qp = 5;
    params->dsi.dsc_params.rc_range_parameters[12].range_max_qp = 13;
    params->dsi.dsc_params.rc_range_parameters[12].range_bpg_offset = -12;
    params->dsi.dsc_params.rc_range_parameters[13].range_min_qp = 7;
    params->dsi.dsc_params.rc_range_parameters[13].range_max_qp = 13;
    params->dsi.dsc_params.rc_range_parameters[13].range_bpg_offset = -12;
    params->dsi.dsc_params.rc_range_parameters[14].range_min_qp = 13;
    params->dsi.dsc_params.rc_range_parameters[14].range_max_qp = 13;
    params->dsi.dsc_params.rc_range_parameters[14].range_bpg_offset = -12;
}

static void lcm_init_power(void *fdt)
{
    LCM_LOGE(" %s enter\n", __func__);
    lcm_set_gpio_output(GPIO_LED_BIAS_EN, GPIO_OUT_ONE);
    MDELAY(5);
    lcm_set_gpio_output(GPIO_LCD_BIAS_ENP, GPIO_OUT_ONE);
    MDELAY(5);
    lcm_set_gpio_output(GPIO_LCD_BIAS_ENN, GPIO_OUT_ONE);
    MDELAY(12);

    i2c_write_bytes(0x05, BIAS_I2C_ADDRESS, 0x0C, 0x30);

    i2c_write_bytes(0x05, BIAS_I2C_ADDRESS, 0x0D, 0x28);
    i2c_write_bytes(0x05, BIAS_I2C_ADDRESS, 0x0E, 0x28);
    i2c_write_bytes(0x05, BIAS_I2C_ADDRESS, 0x09, 0x9C);
    MDELAY(5);
    i2c_write_bytes(0x05, BIAS_I2C_ADDRESS, 0x09, 0x9E);

    i2c_write_bytes(0x05, BIAS_I2C_ADDRESS, 0x02, 0x3B);
    i2c_write_bytes(0x05, BIAS_I2C_ADDRESS, 0x11, 0x37);
    i2c_write_bytes(0x05, BIAS_I2C_ADDRESS, 0x15, 0xB0);
    i2c_write_bytes(0x05, BIAS_I2C_ADDRESS, 0x08, 0x5F);


    LCM_LOGE(" %s exit\n", __func__);
}

static void lcm_init(void *fdt)
{
    LCM_LOGE("=====%s=====\n", __func__);

    lcm_set_gpio_output(GPIO_LCD_RESET, GPIO_OUT_ONE);
    MDELAY(5);
    lcm_set_gpio_output(GPIO_LCD_RESET, GPIO_OUT_ZERO);
    MDELAY(10);
    lcm_set_gpio_output(GPIO_LCD_RESET, GPIO_OUT_ONE);
    MDELAY(60);

    push_table(init_setting_cmd, sizeof(init_setting_cmd) / sizeof(struct LCM_setting_table), 1);

}

static void lcm_setbacklight(unsigned int level)
{
    level = level << 1;

    LCM_LOGE("%s,dsi_panel_hx83102j_boe backlight:  level = %d\n", __func__, level);

    bl_level[0].para_list[0] = (level >> 8) & 0xF;
    bl_level[0].para_list[1] = level & 0xFF;

    push_table(bl_level, sizeof(bl_level) /
        sizeof(struct LCM_setting_table), 1);

}

static unsigned int lcm_compare_id(void *fdt)
{
    LCM_LOGE("[LCM][%s]------hx83102j_boe", __func__);
    int lcm_id0 = 0;
    int lcm_id1 = 0;
    int ret = 0;
    const char *lcd_name = "lcd_name=hx83102j_fhdp_dsi_vdo_120hz_boe_dphy";

    mt_set_gpio_mode(GPIO_LCM_ID0, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_ID0, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_LCM_ID0, GPIO_PULL_DISABLE);

    mt_set_gpio_mode(GPIO_LCM_ID1, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_ID1, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_LCM_ID1, GPIO_PULL_DISABLE);

    /* delay 100ms , for discharging capacitance*/
    MDELAY(50);

    /* get ID0 ID1 status*/
    lcm_id0 = mt_get_gpio_in(GPIO_LCM_ID0);
    lcm_id1 = mt_get_gpio_in(GPIO_LCM_ID1);
    LCM_LOGE("[LCM]%s,lcm_id0 = %d,lcm_id1 = %d\n", __func__, lcm_id0, lcm_id1);
    if (lcm_id0 == 0 && lcm_id1 == 0) {
        LCM_LOGE("[LCM]%s,hx83102j_boe moudle compare success\n", __func__);
        LCM_LOGE("[LCM]%s   lcd_name = %s   ,\n", __func__, lcd_name);
    //kcmdline_append(lcd_name);
    ret = kcmdline_append(lcd_name);
    if (ret != 0)
        LCM_LOGE("kcmdline append %s failed, rc=%d\n", lcd_name, ret);

        return 1;
    }
    LCM_LOGE("[LCM]%s,hx83102j_boe moudle compare fail\n", __func__);
    return 0;
}

LCM_DRIVER hx83102j_fhdp_dsi_vdo_120hz_boe_dphy_lcm_drv = {
    .name = "hx83102j_fhdp_dsi_vdo_120hz_boe_dphy",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params = lcm_get_params,
    .init = lcm_init,
    .init_power = lcm_init_power,
    .compare_id = lcm_compare_id,
    .set_backlight = lcm_setbacklight,
};
