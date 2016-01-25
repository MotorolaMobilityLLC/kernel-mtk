/*
 *
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
 * Definitions for TS3A225E Audio Switch chip.
 */
#ifndef __TS3A227E_H__
#define __TS3A227E_H__

int ts3a227e_read_byte(unsigned char cmd, unsigned char *returnData);
int ts3a227e_write_byte(unsigned char cmd, unsigned char writeData);

#endif
