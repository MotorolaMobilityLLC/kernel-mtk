//add dtv ---lenovo@lenovo.com ---20161119 start 
/*****************************************************************************
	Copyright(c) 2014 FCI Inc. All Rights Reserved

	File name : fc8180_bb.h

	Description : header of baseband driver

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
#ifndef __FC8180_BB__
#define __FC8180_BB__

#include "fci_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern s32 fc8180_reset(HANDLE handle);
extern s32 fc8180_probe(HANDLE handle);
extern s32 fc8180_init(HANDLE handle);
extern s32 fc8180_deinit(HANDLE handle);
extern s32 fc8180_scan_status(HANDLE handle);

#ifdef __cplusplus
}
#endif

#endif /*__FC8180_BB__ */
//add dtv ---lenovo@lenovo.com ---20161119 end 

