/* MPU6515 motion sensor driver
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

#ifndef MPU6515A_H
#define MPU6515A_H

int MPU6515_gse_power(void);
int MPU6515_gse_mode(void);
int MPU6515_SCP_SetPowerMode(bool enable, int sensorType);

int MPU6515_i2c_master_send(u8 *buf, u8 len);
int MPU6515_i2c_master_recv(u8 *buf, u8 len);
int MPU6515_hwmsen_read_block(u8 addr, u8 *buf, u8 len);
int MPU6515_hwmsen_read_byte(u8 addr, u8 *buf);

#endif
