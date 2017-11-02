//add dtv ---lenovo@lenovo.com ---20161119 start 
/*****************************************************************************
	Copyright(c) 2014 FCI Inc. All Rights Reserved

	File name : fci_oal.h

	Description : header of OS adaptation layer

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

	History :
	----------------------------------------------------------------------
*******************************************************************************/
#ifndef __FCI_OAL_H__
#define __FCI_OAL_H__

#ifdef __cplusplus
extern "C" {
#endif

extern void print_log(HANDLE handle, s8 *fmt, ...);
extern void ms_wait(s32 ms);
extern void OAL_CREATE_SEMAPHORE(void);
extern void OAL_DELETE_SEMAPHORE(void);
extern void OAL_OBTAIN_SEMAPHORE(void);
extern void OAL_RELEASE_SEMAPHORE(void);

#ifdef __cplusplus
}
#endif

#endif /* __FCI_OAL_H__ */
//add dtv ---lenovo@lenovo.com ---20161119 end 