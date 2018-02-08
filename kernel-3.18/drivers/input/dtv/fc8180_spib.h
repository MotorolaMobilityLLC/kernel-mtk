//add dtv ---shenyong@wind-mobi.com ---20161119 start 
/*****************************************************************************
 Copyright(c) 2014 FCI Inc. All Rights Reserved

 File name : fc8180_spib.c

 Description : API of 1-SEG baseband module

*******************************************************************************/

#ifndef __FC8180_SPIB__
#define __FC8180_SPIB__

#ifdef __cplusplus
extern "C" {
#endif

extern s32 fc8180_spib_init(HANDLE handle, u16 param1, u16 param2);
extern s32 fc8180_spib_byteread(HANDLE handle, u16 addr, u8 *data);
extern s32 fc8180_spib_wordread(HANDLE handle, u16 addr, u16 *data);
extern s32 fc8180_spib_longread(HANDLE handle, u16 addr, u32 *data);
extern s32 fc8180_spib_bulkread(HANDLE handle, u16 addr, u8 *data, u16 length);
extern s32 fc8180_spib_bytewrite(HANDLE handle, u16 addr, u8 data);
extern s32 fc8180_spib_wordwrite(HANDLE handle, u16 addr, u16 data);
extern s32 fc8180_spib_longwrite(HANDLE handle, u16 addr, u32 data);
extern s32 fc8180_spib_bulkwrite(HANDLE handle, u16 addr, u8 *data, u16 length);
extern s32 fc8180_spib_dataread(HANDLE handle, u16 addr, u8 *data, u32 length);
extern s32 fc8180_spib_deinit(HANDLE handle);

#ifdef __cplusplus
}
#endif

#endif /* __FC8180_SPIB__ */
//add dtv ---shenyong@wind-mobi.com ---20161119 end 
