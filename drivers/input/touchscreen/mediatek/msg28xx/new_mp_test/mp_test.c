/*
 * Copyright (C) 2006-2017 ILITEK TECHNOLOGY CORP.
 *
 * Description: ILITEK I2C touchscreen driver for linux platform.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Dicky Chiang
 * Maintain: Luca Hsu, Tigers Huang
 */

#include "mp_common.h"
extern void DebugShowArray2(void *pBuf, u16 nLen, int nDataType, int nCarry, int nChangeLine);
extern u32 SLAVE_I2C_ID_DBBUS;
extern u32 SLAVE_I2C_ID_DWI2C;
extern void DrvDBBusI2cResponseAck(void);

int run_type = 0;

static int new_clear_switch_status(void)
{
    //int regData = 0;
    //int timeout = 280;
    //int t = 0;
    
    RegSet16BitValue(0x1402, 0xFFFF);

    return 0;
}

static int new_polling_data_ready(void)
{
    int ret = 0;
    int timer = 500;
    u16 RegData = 0;

    mp_info("*** %s() ***\n", __func__);

    while(RegData != 0x7744) {
        RegData = RegGet16BitValueByAddressMode(0x1401, ADDRESS_MODE_16BIT);
        mp_info("TIMER = %d, RegData = 0x%04x\n", timer, RegData);
        mdelay(20);
        timer--;
        if(timer < 0)
         break;
    }

    if(timer <= 0)
        ret = -1;

    return ret;
}

static void new_send_dbbus_access_command(u8 byte)
{
    u8 cmd[1] = {0};

    cmd[0] = byte;
    IicWriteData(SLAVE_I2C_ID_DBBUS, cmd, 1);
}

static int new_get_raw_data(s16 *pRawData, u16 count,u8 dump_time)
{
    int i, j, offset;
    u8 *pShotOriData = NULL;
    s32 *pShotDataAll = NULL;
    u16 fout_base_addr = 0x0, RegData = 0;
    u16 touch_info_length = 35;
    u16 data_length = count;

    mp_info( "*** %s() data_length = %d ***\n", __func__, data_length);

    if(run_type == RUN_SHORT_TEST)
        touch_info_length = 0;

    /* one byte original data */
    pShotOriData = kcalloc(data_length * 2*2 + touch_info_length, sizeof(u8), GFP_KERNEL);
    if(ERR_ALLOC_MEM(pShotOriData)) {
		mp_err("Failed to allocate pShotOriData mem\n");
		return -1;
	}

    /* two bytes combined by above */
    pShotDataAll = kcalloc(data_length*2, sizeof(s32), GFP_KERNEL);
    if(ERR_ALLOC_MEM(pShotDataAll)) {
		mp_err("Failed to allocate pShotDataAll mem\n");
		return -1;
	}

    /* Read dump time coeff */
    if(run_type == RUN_SHORT_TEST)
    {
        if(dump_time == mp_test_data->SHORT_Dump1)
        {
            mp_test_data->SHORT_Dump1 = RegGet16BitValueByAddressMode(0x1018, ADDRESS_MODE_16BIT);
            mp_info(" ***  Dump1 = 0x%x***\n", mp_test_data->SHORT_Dump1);
        }
        if(dump_time == mp_test_data->SHORT_Dump2)
        {
            mp_test_data->SHORT_Dump2 = RegGet16BitValueByAddressMode(0x1018, ADDRESS_MODE_16BIT);
            mp_info(" ***  Dump2 = 0x%x***\n", mp_test_data->SHORT_Dump2);
        }

    }
    /* Read DQ base */
    RegData = RegGet16BitValueByAddressMode(p_mp_func->fout_data_addr, ADDRESS_MODE_16BIT);
    fout_base_addr = (int)(RegData << 2);

    mp_info("fout_base_addr = 0x%x , data_length*2 = %d\n", fout_base_addr, data_length*2);

    if(fout_base_addr <= 0) {
        mp_err("Failed to get fout_base_addr\n");
        return -1;
    }

    StopMCU();

    /* Read data segmentally */
    for(offset = 0; offset < data_length * 2 + touch_info_length; offset += MAX_I2C_TRANSACTION_LENGTH_LIMIT) {
        Msg30xxDBBusReadDQMemStart();
        if(offset == 0)
        {
            RegGetXBitWrite4ByteValue(fout_base_addr + offset, pShotOriData + offset, MAX_I2C_TRANSACTION_LENGTH_LIMIT, MAX_I2C_TRANSACTION_LENGTH_LIMIT);
        }
        else if(offset + MAX_I2C_TRANSACTION_LENGTH_LIMIT < data_length * 2 + touch_info_length)
        {
            RegGetXBitWrite4ByteValue(fout_base_addr + offset, pShotOriData + offset, MAX_I2C_TRANSACTION_LENGTH_LIMIT, MAX_I2C_TRANSACTION_LENGTH_LIMIT);
        }
        else
        {
            RegGetXBitWrite4ByteValue(fout_base_addr + offset, pShotOriData + offset, data_length * 2 + touch_info_length - offset, MAX_I2C_TRANSACTION_LENGTH_LIMIT);
        }
        Msg30xxDBBusReadDQMemEnd();
    }

#ifdef MP_DEBUG
    for(i = 0; i < data_length * 2 + touch_info_length; i++) {
        printk(" %02x ", pShotOriData[i]);
        if(i != 0 && (i % 16 == 0))
            printk("\n");
    }
    printk(" \n\n ");
#endif

    if(run_type == RUN_SHORT_TEST) {
        for (j = 0; j < data_length; j++) {
            pShotDataAll[j] = (pShotOriData[2 * j] | pShotOriData[2 * j + 1 ] << 8);
        }
    } else {
        for (j = 0; j < data_length; j++) {
            pShotDataAll[j] = (pShotOriData[2 * j + touch_info_length + 1] | pShotOriData[2 * j + touch_info_length ] << 8);
        }
    }

#ifdef MP_DEBUG
    for(i = 0; i < data_length; i++) {
        printk(" %04x ", pShotDataAll[i]);
        if(i != 0 && (i % 16 == 0))
            printk("\n");
    }
    printk(" \n\n ");
#endif

    for (i = 0; i < data_length; i++) {
        pRawData[i] = pShotDataAll[i];
    }

    mp_kfree((void **)&pShotOriData);
    mp_kfree((void **)&pShotDataAll);
    StartMCU();
    if(run_type == RUN_SHORT_TEST)
    {
        DbBusWaitMCU();
        DbBusIICUseBus(); //0x35
        DbBusIICReshape(); //0x71
    }
    return 0;
}

static int new_get_deltac(int *delt, u8 dump_time)
{
    int i;
    s16 *pRawData = NULL, count = 0;

    mp_info("*** %s ***\n",__func__);

    pRawData = kcalloc(MAX_CHANNEL_SEN * MAX_CHANNEL_DRV*2, sizeof(s16), GFP_KERNEL);

    if(ERR_ALLOC_MEM(pRawData)) {
		mp_err("Failed to allocate pRawData mem \n");
		return -1;
	}

    if(run_type == RUN_SHORT_TEST) {
        if(p_mp_func->chip_type == CHIP_TYPE_MSG28XX)
            count = MAX_CHANNEL_NUM_28XX;
        else
            count = MAX_SUBFRAMES_30XX * MAX_AFENUMs_30XX;
        if(new_get_raw_data(pRawData, count, dump_time) < 0) {
            mp_err("*** Get DeltaC failed! ***\n");
            return -1;
        }
    } else {
        count = mp_test_data->sensorInfo.numSen * mp_test_data->sensorInfo.numDrv + mp_test_data->sensorInfo.numSen + mp_test_data->sensorInfo.numDrv;
        if(new_get_raw_data(pRawData, count, dump_time) < 0) {
            mp_err("*** Get DeltaC failed! ***\n");
            return -1;
        }
    }

    for (i = 0; i < count; i++)
         delt[i] = pRawData[i];

#ifdef MP_DEBUG
    if(run_type == RUN_SHORT_TEST) {
        for(i = 0; i < count; i++) {
            printk(" %05d ", delt[i]);
            if(i != 0 && (i % 10) == 0)
                printk(" \n ");
        }
        printk(" \n ");
    }
    else{
        for(i = 0; i < count; i++) {
            printk(" %05d ", delt[i]);
            if(i != 0 && (i % mp_test_data->sensorInfo.numDrv) == 0)
                printk(" \n ");
        }
        printk(" \n ");
    }
#endif

    mp_kfree((void **)&pRawData);

    return 0;
}

static int new_send_test_cmd(u16 fmode, int mode)
{
    int ret = 0;
    u8 cmd[8] = {0};
    u8 freq, freq1, csub, cfb, chargeP, short_charge, short_dump, sensorpin;
    u8 post_idle, self_sample_hi, self_sample_lo, short_sample_hi, short_sample_lo;
    u16 chargeT, dumpT;
    int fout_max_1, fout_max_2, hopping_en, capture_count, avg_count, postidle; 

    mp_info("*** Mode = %d ***\n", mode);

    freq = mp_test_data->Open_fixed_carrier;
    freq1 = mp_test_data->Open_fixed_carrier1;
    csub = mp_test_data->Open_test_csub;
    cfb = mp_test_data->Open_test_cfb;
    chargeP = (mp_test_data->Open_test_chargepump ? 0x02 : 0x00);
    chargeT = mp_test_data->OPEN_Charge;
    dumpT = mp_test_data->OPEN_Dump;
    post_idle = mp_test_data->post_idle;
    self_sample_lo = mp_test_data->self_sample_hi;
    self_sample_hi = mp_test_data->self_sample_lo;
    short_charge = mp_test_data->SHORT_Charge;
    short_dump =(u8)(mp_test_data->SHORT_Dump2 - mp_test_data->SHORT_Dump1);
    short_sample_hi = (mp_test_data->SHORT_sample_number & 0xFF00) >> 8;
    short_sample_lo = (mp_test_data->SHORT_sample_number & 0x00FF);
    fout_max_1 = mp_test_data->SHORT_Fout_max_1;
    fout_max_2 = mp_test_data->SHORT_Fout_max_2;
    hopping_en = mp_test_data->SHORT_Hopping;
    capture_count = mp_test_data->SHORT_Capture_count;
    avg_count = mp_test_data->SHORT_AVG_count;
    postidle = mp_test_data->SHORT_Postidle;
    sensorpin = mp_test_data->SHORT_sensor_pin_number;

    cmd[0] = 0xF1; /* Header */
    cmd[1] = mode;

    switch(cmd[1])
    {
        case TYPE_SHORT:
            if(mp_test_data->SHORT_Hopping)
                cmd[1] = TYPE_SHORT_HOPPING;
            cmd[2] = (u8)short_charge;
            cmd[3] = (u8)fmode;
            cmd[4] = (u8)postidle;
            cmd[5] = sensorpin;
            cmd[6] = short_sample_hi;
            cmd[7] = short_sample_lo;
        break;

        case TYPE_SELF:
            if (chargeT == 0 || dumpT == 0) {
                chargeT = 0x18;
                dumpT = 0x16;
            }
            cmd[2] = chargeT;
            cmd[3] = dumpT;
            cmd[4] = post_idle;
            cmd[5] = self_sample_hi;
            cmd[6] = self_sample_lo;
            cmd[7] = 0x0;
        break;

        case TYPE_OPEN:
            if(fmode == MUTUAL_SINE) {
                cmd[2] = (0x01 | chargeP);

                /* Open test by each frequency */
                cmd[3] = freq;
                cmd[4] = freq1;
                cmd[5] = 0x00;
            } else {
                cmd[2] = (0x00 | chargeP);

                if (chargeT == 0 || dumpT == 0) {
                    chargeT = 0x18;
                    dumpT = 0x16;
                }

                /* Switch cap mode */
                cmd[3] = chargeT;
                cmd[4] = dumpT;
                cmd[5] = post_idle; //postidle
            }

            cmd[6] = 0x0;   //Csub, default : 0
            cmd[7] = 0x0;
        break;

        default:
            mp_err("Mode is invalid\n");
            ret = -1;
        break;
    }

#ifdef MP_DEBUG
    {
        int i;
        for(i = 0; i < ARRAY_SIZE(cmd); i++)
            mp_info("*** Test CMD[%d] = 0x%x  ***\n", i, cmd[i]);
    }
#endif

    /* Writting commands via SMBus */
    IicWriteData(SLAVE_I2C_ID_DWI2C, &cmd[0], ARRAY_SIZE(cmd));

    mdelay(5);

    if(new_polling_data_ready() < 0) {
        mp_err("New Flow polling data timout !!\n");
    	return -1;
    }

    /* Clear MP mode */
    RegSet16BitValue(0x1402, 0xFFFF);
    return ret;
}

static int new_flow_start_test(void)
{
    int ret = 0, deltac_size = 0, i = 0;//, count = 0;
    //static int get_deltac_flag = 0;
    u16 fmode = MUTUAL_MODE;
    int *deltac_data = NULL, *short_deltac_data = NULL;

    switch (mp_test_data->Open_mode) {
    	case 0:
    		fmode = MUTUAL_MODE;
    		break;
    	case 1:
    	case 2:
    		fmode = MUTUAL_SINE;
    		break;
    }
    deltac_data = kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
    if(ERR_ALLOC_MEM(deltac_data)) {
		mp_err("Failed to allocate deltac_data mem \n");
        ret = -1;
		goto out;
	}

    deltac_size = MAX_MUTUAL_NUM;
    short_deltac_data = kcalloc(MAX_MUTUAL_NUM, sizeof(int), GFP_KERNEL);
    if(ERR_ALLOC_MEM(short_deltac_data)) {
		mp_err("Failed to allocate deltac_data mem \n");
        ret = -1;
		goto out;
	}
        ret = p_mp_func->enter_mp_mode();
        if(ret < 0) {
            mp_err("Failed to enter MP mode\n");
            //goto out;
        }

        ret = p_mp_func->check_mp_switch();
        if(ret < 0) {
            mp_err("*** Switch FW Mode Busy, return fail ***\n");
            //goto out;
        }

        if(new_clear_switch_status() < 0) {
            mp_err("*** Clear Switch status fail ***\n");
            ret = -1;
            //goto out;
        }


        if(run_type == RUN_SHORT_TEST) {
            fmode = mp_test_data->SHORT_Dump1;
            ret = new_send_test_cmd(fmode, TYPE_SHORT);
            if(ret < 0) {
                mp_err("Send cmd busy\n");
                goto out;
            }
        } else {
            ret = new_send_test_cmd(fmode, TYPE_SELF);
                if(ret < 0) {
                pr_err("Send cmd busy\n");
                goto out;
            }

            ret = new_send_test_cmd(fmode, TYPE_OPEN);
                if(ret < 0) {
                mp_err("Send cmd busy\n");
                goto out;
            }
        }
        /* Get DeltaC */
        if(mp_test_data->get_deltac_flag == 0 && run_type != RUN_SHORT_TEST)
        {
            ret = new_get_deltac(deltac_data, fmode);
            mp_test_data->get_deltac_flag = 1;
            for(i = 0; i < deltac_size; i++)
            {
	            mp_test_result->pdeltac_buffer[i] = deltac_data[i];
            }
        }


        if(run_type == RUN_SHORT_TEST) {
            fmode = mp_test_data->SHORT_Dump2;
            ret = new_send_test_cmd(fmode, TYPE_SHORT);
            if(ret < 0) {
                mp_err("Send cmd busy\n");
                goto out;
            }
            new_get_deltac(short_deltac_data, fmode);
            for(i = 0; i < MAX_SUBFRAMES_30XX * MAX_AFENUMs_30XX; i++)
            {
                if(abs(deltac_data[i]) >= mp_test_data->SHORT_Fout_max_1)
                {
                    deltac_data[i] = deltac_data[i];
                }
                else if(abs(short_deltac_data[i]) >= mp_test_data->SHORT_Fout_max_2)
                {
                    deltac_data[i] = short_deltac_data[i];
                }
                else
                {
                    deltac_data[i] = short_deltac_data[i] - deltac_data[i];
                }
            }
            mp_info("----------------short Dump2 - Dump1 -------------------\n");
            DebugShowArray2(deltac_data, MAX_SUBFRAMES_30XX * MAX_AFENUMs_30XX, -32, 10, 10);
        }
    if(ret == 0)
    {
        /* Judge values */
        if(run_type == RUN_SHORT_TEST)
        {
            mp_test_result->nShortResult = ITO_TEST_OK;
            ret = p_mp_func->short_judge(deltac_data);
        }
        else if(run_type == RUN_OPEN_TEST)
        {
            mp_test_result->nOpenResult = ITO_TEST_OK;
            ret = p_mp_func->open_judge(mp_test_result->pdeltac_buffer, deltac_size);
        }
        else
        {
            mp_test_result->nUniformityResult = ITO_TEST_OK;

            ret = p_mp_func->uniformity_judge(mp_test_result->pdeltac_buffer, deltac_size);
        }
        mp_info("Judge return = %d\n", ret);
    }

out:
    mp_kfree((void **)&deltac_data);
    mp_kfree((void **)&short_deltac_data);
    return ret;
}

int mp_new_flow_main(int item)
{
    int ret = 0;

    run_type = item;

    DrvDisableFingerTouchReport();
    DrvTouchDeviceHwReset();
    mdelay(10);

    /* Enter DBbus */
    DbBusEnterSerialDebugMode();
    DbBusWaitMCU();
    DbBusIICUseBus(); //0x35
    DbBusIICReshape(); //0x71

    new_send_dbbus_access_command(0x80);
    new_send_dbbus_access_command(0x82);
    new_send_dbbus_access_command(0x84);
    RegSet16BitValueOff(0x1E08, BIT15);

    DrvDBBusI2cResponseAck();

    ret = p_mp_func->enter_mp_mode();
    if(ret < 0) {
        mp_err("Failed to enter MP mode\n");
        goto out;
    }

    ret = p_mp_func->check_mp_switch();
    if(ret < 0) {
        mp_err("*** Switch FW Mode Busy, return fail ***\n");
        goto out;
    }

    if(new_clear_switch_status() < 0) {
        mp_err("*** Clear Switch status fail ***\n");
        ret = -1;
        goto out;
    }

    ret = new_flow_start_test();

out:
    /* Exit DBbus */
    RegSet16BitValueOn(0x1E04, BIT15);
    new_send_dbbus_access_command(0x84);
    ExitDBBus();

    DrvTouchDeviceHwReset();
    mdelay(10);
    DrvEnableFingerTouchReport();
    return ret;
}
