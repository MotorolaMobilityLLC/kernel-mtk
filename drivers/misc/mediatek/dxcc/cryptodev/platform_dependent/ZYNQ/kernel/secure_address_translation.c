/*****************************************************************************
* Copyright (C) 2015 ARM Limited or its affiliates.	                         *
* This program is free software; you can redistribute it and/or modify it    *
* under the terms of the GNU General Public License as published by the Free *
* Software Foundation; either version 2 of the License, or (at your option)  * 
* any later version.							     *
* This program is distributed in the hope that it will be useful, but 	     *
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
* or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   *
* for more details.							     *	
* You should have received a copy of the GNU General Public License along    *
* with this program; if not, write to the Free Software Foundation, 	     *
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.        *
*****************************************************************************/

#include <linux/crypto.h>
#include "secure_address_translation.h"

/***************************************************************
* The Secure_address_translation file provides a kernel Platform dependent function to translate an address handle
* into a an address accessible by secure zone.
* In this model, it is assumed that public side is never given a real secure address, but instead it is given an opaque 
* secure address handle, that need to be translated by the cryptodev kernel driver into a real secure address.
* For ZYNQ, we assumed that the handle is the real secure address, but you might need to implement it differently.
****************************************************************/

void* Get_Secure_Address_From_Handle(void* address_handle)
{
    return address_handle;
}

