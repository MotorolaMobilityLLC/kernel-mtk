//add dtv ---shenyong@wind-mobi.com ---20161119 start 
/*****************************************************************************
 Copyright(c) 2014 FCI Inc. All Rights Reserved

 File name : fc8180_spib.c

 Description : fc8180 host interface

*******************************************************************************/
#include "fci_types.h"
#include "fc8180_regs.h"
#include "fci_oal.h"

#define SPI_BMODE           0x00
#define SPI_WMODE           0x10
#define SPI_LMODE           0x20
#define SPI_READ            0x40
#define SPI_WRITE           0x00
#define SPI_AINC            0x80
#define CHIPID              (0 << 3)

static s32 spi_bulkread(HANDLE handle, u16 addr, u8 command, u8 *data,
							u32 length)
{
	/*unsigned char *cmd;

	cmd = g_SpiCmd;

	cmd[0] = addr & 0xff;
	cmd[1] = (addr >> 8) & 0xff;
	cmd[2] = (command & 0xf0) | CHIPID | ((length >> 16) & 0x07);
	cmd[3] = (length >> 8) & 0xff;
	cmd[4] = length & 0xff;

	spi_cmd.pCmd = cmd;
	spi_cmd.cmdSize = 5;
	spi_cmd.pData = g_SpiData;
	spi_cmd.dataSize  length;

	// Send Command and data through the SPI
	if (SPID_SendCommand_ByteRead(&spid, &spi_cmd))
		return BBM_NOK;

	memcpy(data, g_SpiData, length);*/

	return BBM_OK;
}

static s32 spi_bulkwrite(HANDLE handle, u16 addr, u8 command, u8 *data,
							u32 length)
{
	/*unsigned char *cmd;

	cmd = g_SpiCmd;

	cmd[0] = addr & 0xff;
	cmd[1] = (addr >> 8) & 0xff;
	cmd[2] = (command & 0xf0) | CHIPID | ((length >> 16) & 0x07);
	cmd[3] = (length >> 8) & 0xff;
	cmd[4] = length & 0xff;

	spi_cmd.pCmd = cmd;
	spi_cmd.cmdSize = 5;
	spi_cmd.pData = g_SpiData;
	memcpy(g_SpiData, data, length);
	spi_cmd.dataSize = length;

	// Send Command and data through the SPI
	if (SPID_SendCommand_ByteWrite(&spid, &spi_cmd))
		return BBM_NOK;*/

	return BBM_OK;
}

static s32 spi_dataread(HANDLE handle, u16 addr, u8 command, u8 *data,
							u32 length)
{
	/*unsigned char *cmd;

	cmd = g_SpiCmd;

	cmd[0] = addr & 0xff;
	cmd[1] = (addr >> 8) & 0xff;
	cmd[2] = (command & 0xf0) | CHIPID | ((length >> 16) & 0x07);
	cmd[3] = (length >> 8) & 0xff;
	cmd[4] = length & 0xff;

	spi_cmd.pCmd = cmd;
	spi_cmd.cmdSize = 5;
	spi_cmd.pData = data;
	spi_cmd.dataSize = length;

	// Send Command and data through the SPI
	if (SPID_SendCommand_ByteRead(&spid, &spi_cmd))
		return BBM_NOK;*/

	return BBM_OK;
}

s32 fc8180_spib_init(HANDLE handle, u16 param1, u16 param2)
{
	OAL_CREATE_SEMAPHORE();

	return BBM_OK;
}

s32 fc8180_spib_byteread(HANDLE handle, u16 addr, u8 *data)
{
	s32 res;
	u8 command = SPI_READ;

	OAL_OBTAIN_SEMAPHORE();
	res = spi_bulkread(handle, addr, command, data, 1);
	OAL_RELEASE_SEMAPHORE();
	return res;
}

s32 fc8180_spib_wordread(HANDLE handle, u16 addr, u16 *data)
{
	s32 res;
	u8 command = SPI_READ | SPI_AINC;

	OAL_OBTAIN_SEMAPHORE();
	res = spi_bulkread(handle, addr, command, (u8 *) data, 2);
	OAL_RELEASE_SEMAPHORE();
	return res;
}

s32 fc8180_spib_longread(HANDLE handle, u16 addr, u32 *data)
{
	s32 res;
	u8 command = SPI_READ | SPI_AINC;

	OAL_OBTAIN_SEMAPHORE();
	res = spi_bulkread(handle, addr, command, (u8 *) data, 4);
	OAL_RELEASE_SEMAPHORE();
	return res;
}

s32 fc8180_spib_bulkread(HANDLE handle, u16 addr, u8 *data, u16 length)
{
	s32 res;
	u8 command = SPI_READ | SPI_AINC;

	OAL_OBTAIN_SEMAPHORE();
	res = spi_bulkread(handle, addr, command, data, length);
	OAL_RELEASE_SEMAPHORE();
	return res;
}

s32 fc8180_spib_bytewrite(HANDLE handle, u16 addr, u8 data)
{
	s32 res;
	u8 command = SPI_WRITE;

	OAL_OBTAIN_SEMAPHORE();
	res = spi_bulkwrite(handle, addr, command, (u8 *) &data, 1);
	OAL_RELEASE_SEMAPHORE();
	return res;
}

s32 fc8180_spib_wordwrite(HANDLE handle, u16 addr, u32 data)
{
	s32 res;
	u8 command = SPI_WRITE | SPI_AINC;

	OAL_OBTAIN_SEMAPHORE();
	res = spi_bulkwrite(handle, addr, command, (u8 *) &data, 2);
	OAL_RELEASE_SEMAPHORE();
	return res;
}

s32 fc8180_spib_longwrite(HANDLE handle, u16 addr, u32 data)
{
	s32 res;
	u8 command = SPI_WRITE | SPI_AINC;

	OAL_OBTAIN_SEMAPHORE();
	res = spi_bulkwrite(handle, addr, command, (u8 *) &data, 4);
	OAL_RELEASE_SEMAPHORE();
	return res;
}

s32 fc8180_spib_bulkwrite(HANDLE handle, u16 addr, u8 *data, u16 length)
{
	s32 res;
	u8 command = SPI_WRITE | SPI_AINC;

	OAL_OBTAIN_SEMAPHORE();
	res = spi_bulkwrite(handle, addr, command, data, length);
	OAL_RELEASE_SEMAPHORE();
	return res;
}

s32 fc8180_spib_dataread(HANDLE handle, u16 addr, u8 *data, u32 length)
{
	s32 res;
	u8 command = SPI_READ;

	OAL_OBTAIN_SEMAPHORE();
	res = spi_dataread(handle, addr, command, data, length);
	OAL_RELEASE_SEMAPHORE();
	return res;
}

s32 fc8180_spib_deinit(HANDLE handle)
{
	OAL_DELETE_SEMAPHORE();

	return BBM_OK;
}
//add dtv ---shenyong@wind-mobi.com ---20161119 end 