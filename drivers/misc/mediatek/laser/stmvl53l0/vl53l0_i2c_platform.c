/*
 * COPYRIGHT (C) STMicroelectronics 2015. All rights reserved.
 *
 * This software is the confidential and proprietary information of
 * STMicroelectronics ("Confidential Information").  You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered into
 * with STMicroelectronics
 *
 * Programming Golden Rule: Keep it Simple!
 *
 */

/*!
 * \file   VL53L0_platform.c
 * \brief  Code function defintions for Doppler Testchip Platform Layer
 *
 */


//#include <windows.h>
//#include <stdio.h>    // sprintf(), vsnprintf(), printf()

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#include "vl53l0_i2c_platform.h"
#include "vl53l0_def.h"

//#include "ranging_sensor_comms.h"
//#include "comms_platform.h"

#include "vl53l0_platform_log.h"

#ifdef VL53L0_LOG_ENABLE
#define trace_print(level, ...) trace_print_module_function(TRACE_MODULE_PLATFORM, level, TRACE_FUNCTION_NONE, ##__VA_ARGS__)
#define trace_i2c(...) trace_print_module_function(TRACE_MODULE_NONE, TRACE_LEVEL_NONE, TRACE_FUNCTION_I2C, ##__VA_ARGS__)
#endif



char  debug_string[VL53L0_MAX_STRING_LENGTH_PLT];


#define STATUS_OK              0x00
#define STATUS_FAIL            0x01

//lcz@meizu.com add for i2c
extern int s4VL53l0_ReadRegByte(uint8_t addr, uint8_t *data);
extern int s4VL53l0_WriteRegByte(uint8_t addr, uint8_t data);

int32_t VL53L0_write_multi(uint8_t address, uint8_t index, uint8_t *pdata, int32_t count)
{
    int32_t i = 0;
    int32_t status = STATUS_OK;

#ifdef VL53L0_LOG_ENABLE
    char value_as_str[VL53L0_MAX_STRING_LENGTH_PLT];
    char *pvalue_as_str;

    pvalue_as_str =  value_as_str;

    for(i = 0 ; i < count ; i++)
    {
        sprintf(pvalue_as_str,"%02X", *(pdata+i));

        pvalue_as_str += 2;
    }
    trace_i2c("Write reg : 0x%04X, Val : 0x%s\n", index, value_as_str);
#endif

    // write 8-bit standard V2W8 write (not paging mode)
    //status = RANGING_SENSOR_COMMS_Write_V2W8(address, 0, index, pdata, count);
    //lcz@meizu.com,add for i2c 
    for(i= 0;i < count; i++)
	    s4VL53l0_WriteRegByte(index+i, *(pdata+i));

#if 0
    if(status != STATUS_OK)
    {
        RANGING_SENSOR_COMMS_Get_Error_Text(debug_string);
    }
#endif

    return status;
}

int32_t VL53L0_read_multi(uint8_t address, uint8_t index, uint8_t *pdata, int32_t count)
{
    int32_t i = 0;
    int32_t status = STATUS_OK;

#ifdef VL53L0_LOG_ENABLE
    char   value_as_str[VL53L0_MAX_STRING_LENGTH_PLT];
    char *pvalue_as_str;
#endif

    // read 8-bit standard V2W8 write (not paging mode)
    //status = RANGING_SENSOR_COMMS_Read_V2W8(address, 0, index, pdata, count);
    //lcz@meizu.com
    for(i= 0;i < count; i++)
 		s4VL53l0_ReadRegByte(index+i,(pdata+i));

#if 0
    if(status != STATUS_OK)
    {
        RANGING_SENSOR_COMMS_Get_Error_Text(debug_string);
    }
#endif

#ifdef VL53L0_LOG_ENABLE
    // Build  value as string;
    pvalue_as_str =  value_as_str;

    for(i = 0 ; i < count ; i++)
    {
        sprintf(pvalue_as_str, "%02X", *(pdata+i));
        pvalue_as_str += 2;
    }

    trace_i2c("Read  reg : 0x%04X, Val : 0x%s\n", index, value_as_str);
#endif

    return status;
}


int32_t VL53L0_write_byte(uint8_t address, uint8_t index, uint8_t data)
{
    int32_t status = STATUS_OK;
    const int32_t cbyte_count = 1;

    status = VL53L0_write_multi(address, index, &data, cbyte_count);

    return status;

}


int32_t VL53L0_write_word(uint8_t address, uint8_t index, uint16_t data)
{
    int32_t status = STATUS_OK;

    uint8_t  buffer[BYTES_PER_WORD];

    // Split 16-bit word into MS and LS uint8_t
    buffer[0] = (uint8_t)(data >> 8);
    buffer[1] = (uint8_t)(data &  0x00FF);

    status = VL53L0_write_multi(address, index, buffer, BYTES_PER_WORD);

    return status;

}


int32_t VL53L0_write_dword(uint8_t address, uint8_t index, uint32_t data)
{
    int32_t status = STATUS_OK;
    uint8_t  buffer[BYTES_PER_DWORD];

    // Split 32-bit word into MS ... LS bytes
    buffer[0] = (uint8_t) (data >> 24);
    buffer[1] = (uint8_t)((data &  0x00FF0000) >> 16);
    buffer[2] = (uint8_t)((data &  0x0000FF00) >> 8);
    buffer[3] = (uint8_t) (data &  0x000000FF);

    status = VL53L0_write_multi(address, index, buffer, BYTES_PER_DWORD);

    return status;

}


int32_t VL53L0_read_byte(uint8_t address, uint8_t index, uint8_t *pdata)
{
    int32_t status = STATUS_OK;
    int32_t cbyte_count = 1;

    status = VL53L0_read_multi(address, index, pdata, cbyte_count);

    return status;

}


int32_t VL53L0_read_word(uint8_t address, uint8_t index, uint16_t *pdata)
{
    int32_t  status = STATUS_OK;
	uint8_t  buffer[BYTES_PER_WORD];

    status = VL53L0_read_multi(address, index, buffer, BYTES_PER_WORD);
	*pdata = ((uint16_t)buffer[0]<<8) + (uint16_t)buffer[1];

    return status;

}

int32_t VL53L0_read_dword(uint8_t address, uint8_t index, uint32_t *pdata)
{
    int32_t status = STATUS_OK;
	uint8_t  buffer[BYTES_PER_DWORD];

    status = VL53L0_read_multi(address, index, buffer, BYTES_PER_DWORD);
    *pdata = ((uint32_t)buffer[0]<<24) + ((uint32_t)buffer[1]<<16) + ((uint32_t)buffer[2]<<8) + (uint32_t)buffer[3];

    return status;

}




