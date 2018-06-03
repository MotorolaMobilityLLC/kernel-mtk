/*
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
/*
 * Definitions for CM36652 als/ps sensor chip.
 */
#ifndef __CM36652_CS_H__
#define __CM36652_CS_H__

/*DEFINE_MUTEX(cm36652_cs_mutex);*/
extern struct mutex	cm36652_cs_mutex;

extern ulong           cs_enable;         /*enable mask*/

enum CMC_CS_BIT {
	CMC_BIT_AS      = 1,
	CMC_BIT_RGB     = 2,
};
extern enum CMC_CS_BIT CS_BIT;

#endif
