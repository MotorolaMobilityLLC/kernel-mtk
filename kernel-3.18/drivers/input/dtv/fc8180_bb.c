//add dtv ---shenyong@wind-mobi.com ---20161119 start 
/*****************************************************************************
	Copyright(c) 2014 FCI Inc. All Rights Reserved

	File name : fc8180_bb.c

	Description : source of baseband driver

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
#include "fci_types.h"
#include "fci_oal.h"
#include "fci_hal.h"
#include "fci_tun.h"
#include "fc8180_regs.h"

static void fc8180_clock_mode(HANDLE handle, u32 xtal_freq, u32 band_width)
{
	switch (xtal_freq) {
	case 16000:
		bbm_write(handle, 0x0016, 0);

		switch (band_width) {
		case 6:
			bbm_word_write(handle, 0x1032, 0x30a4);
			bbm_long_write(handle, 0x103c, 0x1f800000);
			bbm_write(handle, 0x200a, 0x04);
			bbm_write(handle, 0x2009, 0x10);
			bbm_write(handle, 0x2008, 0x41);

			/* xtal_in : 24,16,32        (adc clk = 4.0000) */
			bbm_long_write(handle, 0x1040, 0x07000000);
			bbm_long_write(handle, 0x1044, 0x0403010f);
			bbm_long_write(handle, 0x1048, 0x18141901);
			bbm_long_write(handle, 0x104c, 0x4a40260a);
			break;
		case 7:
			bbm_word_write(handle, 0x1032, 0x30a4);
			bbm_long_write(handle, 0x103c, 0x1b000000);
			bbm_write(handle, 0x200a, 0x04);
			bbm_write(handle, 0x2009, 0xbd);
			bbm_write(handle, 0x2008, 0xa1);

			/* xtal_in : 24,16,32        (adc clk = 4.0000) */
			bbm_long_write(handle, 0x1040, 0x00000000);
			bbm_long_write(handle, 0x1044, 0x04000e0f);
			bbm_long_write(handle, 0x1048, 0x12151f05);
			bbm_long_write(handle, 0x104c, 0x55452201);
			break;
		case 8:
			bbm_word_write(handle, 0x1032, 0x3ae1);
			bbm_long_write(handle, 0x103c, 0x17a00000);
			bbm_write(handle, 0x200a, 0x05);
			bbm_write(handle, 0x2009, 0x6b);
			bbm_write(handle, 0x2008, 0x01);

			/* xtal_in : 24,16,32        (adc clk = 4.0000) */
			bbm_long_write(handle, 0x1040, 0x01000000);
			bbm_long_write(handle, 0x1044, 0x0f0d0f01);
			bbm_long_write(handle, 0x1048, 0x101d0605);
			bbm_long_write(handle, 0x104c, 0x624b1af6);
			break;
		}
		break;
	case 16384:
		bbm_write(handle, 0x0016, 0);

		switch (band_width) {
		case 6:
			bbm_word_write(handle, 0x1032, 0x2f80);
			bbm_long_write(handle, 0x103c, 0x20418937);
			bbm_write(handle, 0x200a, 0x03);
			bbm_write(handle, 0x2009, 0xf7);
			bbm_write(handle, 0x2008, 0xdf);

			/* xtal_in : 16.384, 24.576  (adc clk = 4.0960) */
			bbm_long_write(handle, 0x1040, 0x07070000);
			bbm_long_write(handle, 0x1044, 0x03030200);
			bbm_long_write(handle, 0x1048, 0x1a14181f);
			bbm_long_write(handle, 0x104c, 0x473e260c);
			break;
		case 7:
			bbm_word_write(handle, 0x1032, 0x2f80);
			bbm_long_write(handle, 0x103c, 0x1ba5e354);
			bbm_write(handle, 0x200a, 0x04);
			bbm_write(handle, 0x2009, 0xa1);
			bbm_write(handle, 0x2008, 0x2f);

			/* xtal_in : 16.384, 24.576  (adc clk = 4.0960) */
			bbm_long_write(handle, 0x1040, 0x00000000);
			bbm_long_write(handle, 0x1044, 0x04010f0f);
			bbm_long_write(handle, 0x1048, 0x13151e05);
			bbm_long_write(handle, 0x104c, 0x53442302);
			break;
		case 8:
			bbm_word_write(handle, 0x1032, 0x3980);
			bbm_long_write(handle, 0x103c, 0x183126e9);
			bbm_write(handle, 0x200a, 0x05);
			bbm_write(handle, 0x2009, 0x4a);
			bbm_write(handle, 0x2008, 0x7f);

			/* xtal_in : 16.384, 24.576  (adc clk = 4.0960) */
			bbm_long_write(handle, 0x1040, 0x01000000);
			bbm_long_write(handle, 0x1044, 0x000d0f00);
			bbm_long_write(handle, 0x1048, 0x101c0605);
			bbm_long_write(handle, 0x104c, 0x604a1bf7);
			break;
		}
		break;
	case 18000:
		bbm_write(handle, 0x0016, 0);

		switch (band_width) {
		case 6:
			bbm_word_write(handle, 0x1032, 0x2b3c);
			bbm_long_write(handle, 0x103c, 0x23700000);
			bbm_write(handle, 0x200a, 0x03);
			bbm_write(handle, 0x2009, 0x9c);
			bbm_write(handle, 0x2008, 0xac);

			/* xtal_in : 27,18           (adc clk = 4.5000) */
			bbm_long_write(handle, 0x1040, 0x00070700);
			bbm_long_write(handle, 0x1044, 0x00030301);
			bbm_long_write(handle, 0x1048, 0x1f16161b);
			bbm_long_write(handle, 0x104c, 0x413a2711);
			break;
		case 7:
			bbm_word_write(handle, 0x1032, 0x2b3c);
			bbm_long_write(handle, 0x103c, 0x1e600000);
			bbm_write(handle, 0x200a, 0x04);
			bbm_write(handle, 0x2009, 0x36);
			bbm_write(handle, 0x2008, 0xc8);

			/* xtal_in : 27,18           (adc clk = 4.5000) */
			bbm_long_write(handle, 0x1040, 0x07000000);
			bbm_long_write(handle, 0x1044, 0x0503000f);
			bbm_long_write(handle, 0x1048, 0x16141b02);
			bbm_long_write(handle, 0x104c, 0x4d422507);
			break;
		case 8:
			bbm_word_write(handle, 0x1032, 0x3456);
			bbm_long_write(handle, 0x103c, 0x1a940000);
			bbm_write(handle, 0x200a, 0x04);
			bbm_write(handle, 0x2009, 0xd0);
			bbm_write(handle, 0x2008, 0xe5);

			/* xtal_in : 27,18           (adc clk = 4.5000) */
			bbm_long_write(handle, 0x1040, 0x00000000);
			bbm_long_write(handle, 0x1044, 0x03000e0f);
			bbm_long_write(handle, 0x1048, 0x11160005);
			bbm_long_write(handle, 0x104c, 0x564621ff);
			break;
		}
		break;
	case 19200:
		bbm_write(handle, 0x0016, 0);

		switch (band_width) {
		case 6:
			bbm_word_write(handle, 0x1032, 0x2889);
			bbm_long_write(handle, 0x103c, 0x25cccccd);
			bbm_write(handle, 0x200a, 0x03);
			bbm_write(handle, 0x2009, 0x62);
			bbm_write(handle, 0x2008, 0xe1);

			/* xtal_in : 19.2, 38.4      (adc clk = 4.8000) */
			bbm_long_write(handle, 0x1040, 0x00070700);
			bbm_long_write(handle, 0x1044, 0x0f020302);
			bbm_long_write(handle, 0x1048, 0x0218161a);
			bbm_long_write(handle, 0x104c, 0x3e382713);
			break;
		case 7:
			bbm_word_write(handle, 0x1032, 0x2889);
			bbm_long_write(handle, 0x103c, 0x20666666);
			bbm_write(handle, 0x200a, 0x03);
			bbm_write(handle, 0x2009, 0xf3);
			bbm_write(handle, 0x2008, 0x5c);

			/* xtal_in : 19.2, 38.4      (adc clk = 4.8000) */
			bbm_long_write(handle, 0x1040, 0x07070000);
			bbm_long_write(handle, 0x1044, 0x03040200);
			bbm_long_write(handle, 0x1048, 0x1a14181f);
			bbm_long_write(handle, 0x104c, 0x473e260c);
			break;
		case 8:
			bbm_word_write(handle, 0x1032, 0x3111);
			bbm_long_write(handle, 0x103c, 0x1c59999a);
			bbm_write(handle, 0x200a, 0x04);
			bbm_write(handle, 0x2009, 0x83);
			bbm_write(handle, 0x2008, 0xd6);

			/* xtal_in : 19.2, 38.4      (adc clk = 4.8000) */
			bbm_long_write(handle, 0x1040, 0x07000000);
			bbm_long_write(handle, 0x1044, 0x05010f0e);
			bbm_long_write(handle, 0x1048, 0x13141d04);
			bbm_long_write(handle, 0x104c, 0x51442304);
			break;
		}
		break;
	case 24000:
		bbm_write(handle, 0x0016, 2);

		switch (band_width) {
		case 6:
			bbm_word_write(handle, 0x1032, 0x30a4);
			bbm_long_write(handle, 0x103c, 0x1f800000);
			bbm_write(handle, 0x200a, 0x04);
			bbm_write(handle, 0x2009, 0x10);
			bbm_write(handle, 0x2008, 0x41);

			/* xtal_in : 24,16,32        (adc clk = 4.0000) */
			bbm_long_write(handle, 0x1040, 0x07000000);
			bbm_long_write(handle, 0x1044, 0x0403010f);
			bbm_long_write(handle, 0x1048, 0x18141901);
			bbm_long_write(handle, 0x104c, 0x4a40260a);
			break;
		case 7:
			bbm_word_write(handle, 0x1032, 0x30a4);
			bbm_long_write(handle, 0x103c, 0x1b000000);
			bbm_write(handle, 0x200a, 0x04);
			bbm_write(handle, 0x2009, 0xbd);
			bbm_write(handle, 0x2008, 0xa1);

			/* xtal_in : 24,16,32        (adc clk = 4.0000) */
			bbm_long_write(handle, 0x1040, 0x00000000);
			bbm_long_write(handle, 0x1044, 0x04000e0f);
			bbm_long_write(handle, 0x1048, 0x12151f05);
			bbm_long_write(handle, 0x104c, 0x55452201);
			break;
		case 8:
			bbm_word_write(handle, 0x1032, 0x3ae1);
			bbm_long_write(handle, 0x103c, 0x17a00000);
			bbm_write(handle, 0x200a, 0x05);
			bbm_write(handle, 0x2009, 0x6b);
			bbm_write(handle, 0x2008, 0x01);

			/* xtal_in : 24,16,32        (adc clk = 4.0000) */
			bbm_long_write(handle, 0x1040, 0x01000000);
			bbm_long_write(handle, 0x1044, 0x0f0d0f01);
			bbm_long_write(handle, 0x1048, 0x101d0605);
			bbm_long_write(handle, 0x104c, 0x624b1af6);
			break;
		}
		break;
	case 24576:
		bbm_write(handle, 0x0016, 2);

		switch (band_width) {
		case 6:
			bbm_word_write(handle, 0x1032, 0x2f80);
			bbm_long_write(handle, 0x103c, 0x20418937);
			bbm_write(handle, 0x200a, 0x03);
			bbm_write(handle, 0x2009, 0xf7);
			bbm_write(handle, 0x2008, 0xdf);

			/* xtal_in : 16.384, 24.576  (adc clk = 4.0960) */
			bbm_long_write(handle, 0x1040, 0x07070000);
			bbm_long_write(handle, 0x1044, 0x03030200);
			bbm_long_write(handle, 0x1048, 0x1a14181f);
			bbm_long_write(handle, 0x104c, 0x473e260c);
			break;
		case 7:
			bbm_word_write(handle, 0x1032, 0x2f80);
			bbm_long_write(handle, 0x103c, 0x1ba5e354);
			bbm_write(handle, 0x200a, 0x04);
			bbm_write(handle, 0x2009, 0xa1);
			bbm_write(handle, 0x2008, 0x2f);

			/* xtal_in : 16.384, 24.576  (adc clk = 4.0960) */
			bbm_long_write(handle, 0x1040, 0x00000000);
			bbm_long_write(handle, 0x1044, 0x04010f0f);
			bbm_long_write(handle, 0x1048, 0x13151e05);
			bbm_long_write(handle, 0x104c, 0x53442302);
			break;
		case 8:
			bbm_word_write(handle, 0x1032, 0x3980);
			bbm_long_write(handle, 0x103c, 0x183126e9);
			bbm_write(handle, 0x200a, 0x05);
			bbm_write(handle, 0x2009, 0x4a);
			bbm_write(handle, 0x2008, 0x7f);

			/* xtal_in : 16.384, 24.576  (adc clk = 4.0960) */
			bbm_long_write(handle, 0x1040, 0x01000000);
			bbm_long_write(handle, 0x1044, 0x000d0f00);
			bbm_long_write(handle, 0x1048, 0x101c0605);
			bbm_long_write(handle, 0x104c, 0x604a1bf7);
			break;
		}
		break;
	case 26000:
		bbm_write(handle, 0x0016, 2);

		switch (band_width) {
		case 6:
			bbm_word_write(handle, 0x1032, 0x2ce6);
			bbm_long_write(handle, 0x103c, 0x221fffd4);
			bbm_write(handle, 0x200a, 0x03);
			bbm_write(handle, 0x2009, 0xc0);
			bbm_write(handle, 0x2008, 0x3c);

			/* xtal_in : 26              (adc clk = 4.3333) */
			bbm_long_write(handle, 0x1040, 0x00070700);
			bbm_long_write(handle, 0x1044, 0x01030301);
			bbm_long_write(handle, 0x1048, 0x1e16171c);
			bbm_long_write(handle, 0x104c, 0x433b2710);
			break;
		case 7:
			bbm_word_write(handle, 0x1032, 0x2ce6);
			bbm_long_write(handle, 0x103c, 0x1d3fffda);
			bbm_write(handle, 0x200a, 0x04);
			bbm_write(handle, 0x2009, 0x60);
			bbm_write(handle, 0x2008, 0x46);

			/* xtal_in : 26              (adc clk = 4.3333) */
			bbm_long_write(handle, 0x1040, 0x07000000);
			bbm_long_write(handle, 0x1044, 0x05020f0e);
			bbm_long_write(handle, 0x1048, 0x14141c04);
			bbm_long_write(handle, 0x104c, 0x50432405);
			break;
		case 8:
			bbm_word_write(handle, 0x1032, 0x365a);
			bbm_long_write(handle, 0x103c, 0x1997ffdf);
			bbm_write(handle, 0x200a, 0x05);
			bbm_write(handle, 0x2009, 0x00);
			bbm_write(handle, 0x2008, 0x50);

			/* xtal_in : 26              (adc clk = 4.3333) */
			bbm_long_write(handle, 0x1040, 0x01000000);
			bbm_long_write(handle, 0x1044, 0x020e0e00);
			bbm_long_write(handle, 0x1048, 0x10190406);
			bbm_long_write(handle, 0x104c, 0x5b481efb);
			break;
		}
		break;
	case 27000:
		bbm_write(handle, 0x0016, 2);

		switch (band_width) {
		case 6:
			bbm_word_write(handle, 0x1032, 0x2b3c);
			bbm_long_write(handle, 0x103c, 0x23700000);
			bbm_write(handle, 0x200a, 0x03);
			bbm_write(handle, 0x2009, 0x9c);
			bbm_write(handle, 0x2008, 0xac);

			/* xtal_in : 27,18           (adc clk = 4.5000) */
			bbm_long_write(handle, 0x1040, 0x00070700);
			bbm_long_write(handle, 0x1044, 0x00030301);
			bbm_long_write(handle, 0x1048, 0x1f16161b);
			bbm_long_write(handle, 0x104c, 0x413a2711);
			break;
		case 7:
			bbm_word_write(handle, 0x1032, 0x2b3c);
			bbm_long_write(handle, 0x103c, 0x1e600000);
			bbm_write(handle, 0x200a, 0x04);
			bbm_write(handle, 0x2009, 0x36);
			bbm_write(handle, 0x2008, 0xc8);

			/* xtal_in : 27,18           (adc clk = 4.5000) */
			bbm_long_write(handle, 0x1040, 0x07000000);
			bbm_long_write(handle, 0x1044, 0x0503000f);
			bbm_long_write(handle, 0x1048, 0x16141b02);
			bbm_long_write(handle, 0x104c, 0x4d422507);
			break;
		case 8:
			bbm_word_write(handle, 0x1032, 0x3456);
			bbm_long_write(handle, 0x103c, 0x1a940000);
			bbm_write(handle, 0x200a, 0x04);
			bbm_write(handle, 0x2009, 0xd0);
			bbm_write(handle, 0x2008, 0xe5);

			/* xtal_in : 27,18           (adc clk = 4.5000) */
			bbm_long_write(handle, 0x1040, 0x00000000);
			bbm_long_write(handle, 0x1044, 0x03000e0f);
			bbm_long_write(handle, 0x1048, 0x11160005);
			bbm_long_write(handle, 0x104c, 0x564621ff);
			break;
		}
		break;
	case 27120:
		bbm_write(handle, 0x0016, 2);

		switch (band_width) {
		case 6:
			bbm_word_write(handle, 0x1032, 0x2b0b);
			bbm_long_write(handle, 0x103c, 0x239851ec);
			bbm_write(handle, 0x200a, 0x03);
			bbm_write(handle, 0x2009, 0x98);
			bbm_write(handle, 0x2008, 0x94);

			/* xtal_in : 27.12           (adc clk = 4.5200 */
			bbm_long_write(handle, 0x1040, 0x00070700);
			bbm_long_write(handle, 0x1044, 0x00030301);
			bbm_long_write(handle, 0x1048, 0x1f16161b);
			bbm_long_write(handle, 0x104c, 0x413a2711);
			break;
		case 7:
			bbm_word_write(handle, 0x1032, 0x2b0b);
			bbm_long_write(handle, 0x103c, 0x1e828f5c);
			bbm_write(handle, 0x200a, 0x04);
			bbm_write(handle, 0x2009, 0x32);
			bbm_write(handle, 0x2008, 0x02);

			/* xtal_in : 27.12           (adc clk = 4.5200) */
			bbm_long_write(handle, 0x1040, 0x07000000);
			bbm_long_write(handle, 0x1044, 0x0503000f);
			bbm_long_write(handle, 0x1048, 0x16141b02);
			bbm_long_write(handle, 0x104c, 0x4d412507);
			break;
		case 8:
			bbm_word_write(handle, 0x1032, 0x341b);
			bbm_long_write(handle, 0x103c, 0x1ab23d71);
			bbm_write(handle, 0x200a, 0x04);
			bbm_write(handle, 0x2009, 0xcb);
			bbm_write(handle, 0x2008, 0x70);

			/* xtal_in : 27.12           (adc clk = 4.5200) */
			bbm_long_write(handle, 0x1040, 0x00000000);
			bbm_long_write(handle, 0x1044, 0x04000e0f);
			bbm_long_write(handle, 0x1048, 0x12160005);
			bbm_long_write(handle, 0x104c, 0x56462100);
			break;
		}
		break;
	case 32000:
		/* Default Clock */
		bbm_write(handle, 0x0016, 1);

		switch (band_width) {
		case 6:
			/* Default Band-width */
			bbm_word_write(handle, 0x1032, 0x30a4);
			bbm_long_write(handle, 0x103c, 0x1f800000);
			bbm_write(handle, 0x200a, 0x04);
			bbm_write(handle, 0x2009, 0x10);
			bbm_write(handle, 0x2008, 0x41);

			/* xtal_in : 24,16,32        (adc clk = 4.0000) */
			bbm_long_write(handle, 0x1040, 0x07000000);
			bbm_long_write(handle, 0x1044, 0x0403010f);
			bbm_long_write(handle, 0x1048, 0x18141901);
			bbm_long_write(handle, 0x104c, 0x4a40260a);
			break;
		case 7:
			bbm_word_write(handle, 0x1032, 0x30a4);
			bbm_long_write(handle, 0x103c, 0x1b000000);
			bbm_write(handle, 0x200a, 0x04);
			bbm_write(handle, 0x2009, 0xbd);
			bbm_write(handle, 0x2008, 0xa1);

			/* xtal_in : 24,16,32        (adc clk = 4.0000) */
			bbm_long_write(handle, 0x1040, 0x00000000);
			bbm_long_write(handle, 0x1044, 0x04000e0f);
			bbm_long_write(handle, 0x1048, 0x12151f05);
			bbm_long_write(handle, 0x104c, 0x55452201);
			break;
		case 8:
			bbm_word_write(handle, 0x1032, 0x3ae1);
			bbm_long_write(handle, 0x103c, 0x17a00000);
			bbm_write(handle, 0x200a, 0x05);
			bbm_write(handle, 0x2009, 0x6b);
			bbm_write(handle, 0x2008, 0x01);

			/* xtal_in : 24,16,32        (adc clk = 4.0000) */
			bbm_long_write(handle, 0x1040, 0x01000000);
			bbm_long_write(handle, 0x1044, 0x0f0d0f01);
			bbm_long_write(handle, 0x1048, 0x101d0605);
			bbm_long_write(handle, 0x104c, 0x624b1af6);
			break;
		}
		break;
	case 37200:
		bbm_write(handle, 0x0016, 1);

		switch (band_width) {
		case 6:
			bbm_word_write(handle, 0x1032, 0x29d7);
			bbm_long_write(handle, 0x103c, 0x249e6666);
			bbm_write(handle, 0x200a, 0x03);
			bbm_write(handle, 0x2009, 0x7e);
			bbm_write(handle, 0x2008, 0xd8);

			/* xtal_in : 37.2            (adc clk = 4.6500 */
			bbm_long_write(handle, 0x1040, 0x00070700);
			bbm_long_write(handle, 0x1044, 0x00030302);
			bbm_long_write(handle, 0x1048, 0x0117161a);
			bbm_long_write(handle, 0x104c, 0x3f392712);
			break;
		case 7:
			bbm_word_write(handle, 0x1032, 0x29d7);
			bbm_long_write(handle, 0x103c, 0x1f633333);
			bbm_write(handle, 0x200a, 0x04);
			bbm_write(handle, 0x2009, 0x13);
			bbm_write(handle, 0x2008, 0xfc);

			/* xtal_in : 37.2            (adc clk = 4.6500) */
			bbm_long_write(handle, 0x1040, 0x07000000);
			bbm_long_write(handle, 0x1044, 0x0403010f);
			bbm_long_write(handle, 0x1048, 0x18141900);
			bbm_long_write(handle, 0x104c, 0x4a3f260a);
			break;
		case 8:
			bbm_word_write(handle, 0x1032, 0x32a6);
			bbm_long_write(handle, 0x103c, 0x1b76cccd);
			bbm_write(handle, 0x200a, 0x04);
			bbm_write(handle, 0x2009, 0xa9);
			bbm_write(handle, 0x2008, 0x1f);

			/* xtal_in : 37.2            (adc clk = 4.6500) */
			bbm_long_write(handle, 0x1040, 0x00000000);
			bbm_long_write(handle, 0x1044, 0x04010f0f);
			bbm_long_write(handle, 0x1048, 0x13151e05);
			bbm_long_write(handle, 0x104c, 0x53442302);
			break;
		}
		break;
	case 37400:
		bbm_write(handle, 0x0016, 1);

		switch (band_width) {
		case 6:
			bbm_word_write(handle, 0x1032, 0x299e);
			bbm_long_write(handle, 0x103c, 0x24d0cccd);
			bbm_write(handle, 0x200a, 0x03);
			bbm_write(handle, 0x2009, 0x7a);
			bbm_write(handle, 0x2008, 0x0f);

			/* xtal_in : 37.4            (adc clk = 4.6750) */
			bbm_long_write(handle, 0x1040, 0x00070700);
			bbm_long_write(handle, 0x1044, 0x00030302);
			bbm_long_write(handle, 0x1048, 0x0117161a);
			bbm_long_write(handle, 0x104c, 0x3f392712);
			break;
		case 7:
			bbm_word_write(handle, 0x1032, 0x299e);
			bbm_long_write(handle, 0x103c, 0x1f8e6666);
			bbm_write(handle, 0x200a, 0x04);
			bbm_write(handle, 0x2009, 0x0e);
			bbm_write(handle, 0x2008, 0x66);

			/* xtal_in : 37.4            (adc clk = 4.6750) */
			bbm_long_write(handle, 0x1040, 0x07000000);
			bbm_long_write(handle, 0x1044, 0x0403010f);
			bbm_long_write(handle, 0x1048, 0x18141900);
			bbm_long_write(handle, 0x104c, 0x4a3f260a);
			break;
		case 8:
			bbm_word_write(handle, 0x1032, 0x3261);
			bbm_long_write(handle, 0x103c, 0x1b9c999a);
			bbm_write(handle, 0x200a, 0x04);
			bbm_write(handle, 0x2009, 0xa2);
			bbm_write(handle, 0x2008, 0xbe);

			/* xtal_in : 37.4            (adc clk = 4.6750) */
			bbm_long_write(handle, 0x1040, 0x00000000);
			bbm_long_write(handle, 0x1044, 0x04010f0f);
			bbm_long_write(handle, 0x1048, 0x13151e05);
			bbm_long_write(handle, 0x104c, 0x53442302);
			break;
		}
		break;
	case 38400:
		bbm_write(handle, 0x0016, 1);

		switch (band_width) {
		case 6:
			bbm_word_write(handle, 0x1032, 0x2889);
			bbm_long_write(handle, 0x103c, 0x25cccccd);
			bbm_write(handle, 0x200a, 0x03);
			bbm_write(handle, 0x2009, 0x62);
			bbm_write(handle, 0x2008, 0xe1);

			/* xtal_in : 19.2, 38.4      (adc clk = 4.8000) */
			bbm_long_write(handle, 0x1040, 0x00070700);
			bbm_long_write(handle, 0x1044, 0x0f020302);
			bbm_long_write(handle, 0x1048, 0x0218161a);
			bbm_long_write(handle, 0x104c, 0x3e382713);
			break;
		case 7:
			bbm_word_write(handle, 0x1032, 0x2889);
			bbm_long_write(handle, 0x103c, 0x20666666);
			bbm_write(handle, 0x200a, 0x03);
			bbm_write(handle, 0x2009, 0xf3);
			bbm_write(handle, 0x2008, 0x5c);

			/* xtal_in : 19.2, 38.4      (adc clk = 4.8000) */
			bbm_long_write(handle, 0x1040, 0x07070000);
			bbm_long_write(handle, 0x1044, 0x03040200);
			bbm_long_write(handle, 0x1048, 0x1a14181f);
			bbm_long_write(handle, 0x104c, 0x473e260c);
			break;
		case 8:
			bbm_word_write(handle, 0x1032, 0x3111);
			bbm_long_write(handle, 0x103c, 0x1c59999a);
			bbm_write(handle, 0x200a, 0x04);
			bbm_write(handle, 0x2009, 0x83);
			bbm_write(handle, 0x2008, 0xd6);

			/* xtal_in : 19.2, 38.4      (adc clk = 4.8000) */
			bbm_long_write(handle, 0x1040, 0x07000000);
			bbm_long_write(handle, 0x1044, 0x05010f0e);
			bbm_long_write(handle, 0x1048, 0x13141d04);
			bbm_long_write(handle, 0x104c, 0x51442304);
			break;
		}
	}
}

s32 fc8180_reset(HANDLE handle)
{
	bbm_write(handle, BBM_SW_RESET, 0x7f);
	bbm_write(handle, BBM_SW_RESET, 0xff);

	return BBM_OK;
}

s32 fc8180_probe(HANDLE handle)
{
	u16 ver;
	bbm_word_read(handle, BBM_CHIP_ID_L, &ver);

	return (ver == 0x8180) ? BBM_OK : BBM_NOK;
}

s32 fc8180_init(HANDLE handle)
{
	fc8180_reset(handle);
	fc8180_clock_mode(handle, BBM_XTAL_FREQ, BBM_BAND_WIDTH);

	bbm_write(handle, 0x1000, 0x27);
	bbm_write(handle, 0x1004, 0x4d);
	bbm_write(handle, 0x1064, 0x04);
	bbm_write(handle, 0x1065, 0xe0);
	bbm_write(handle, 0x1069, 0x09);
	bbm_write(handle, 0x1075, 0x00);
	bbm_write(handle, 0x1083, 0x14);
	bbm_write(handle, 0x1084, 0x0f);
	bbm_write(handle, 0x2004, 0x41);
	bbm_write(handle, 0x2060, 0x54);
	bbm_write(handle, 0x5010, 0x00);
#ifdef BBM_EXT_LNA
	bbm_write(handle, 0x9001, 0x01);
#endif

	bbm_write(handle, 0x00b0, 0x07);
	bbm_write(handle, 0x00b4, 0x00);
	bbm_write(handle, 0x00b5, 0x00);
	bbm_write(handle, 0x00b6, 0x04);
	bbm_write(handle, 0x00b9, 0x00);
	bbm_write(handle, 0x00ba, 0x01);

	bbm_word_write(handle, BBM_BUF_TS_START, TS_BUF_START);
	bbm_word_write(handle, BBM_BUF_TS_END, TS_BUF_END);
	bbm_word_write(handle, BBM_BUF_TS_THR, TS_BUF_THR);

	bbm_word_write(handle, BBM_BUF_AC_START, AC_BUF_START);
	bbm_word_write(handle, BBM_BUF_AC_END, AC_BUF_END);
	bbm_word_write(handle, BBM_BUF_AC_THR, AC_BUF_THR);

#ifdef BBM_I2C_TSIF
#ifdef BBM_TSIF_LOW_ACTIVE
	bbm_write(handle, BBM_TS_CTRL, 0x00);
#else
	bbm_write(handle, BBM_TS_CTRL, 0x0e);
#endif

#ifdef BBM_TS_204
	bbm_write(handle, BBM_TS_SEL, 0xc0);
#else
	bbm_write(handle, BBM_TS_SEL, 0x80);
#endif
#endif

#ifdef BBM_FAIL_FRAME
	bbm_write(handle, BBM_RS_FAIL_TX, 0x02);
#endif

#ifdef BBM_INT_LOW_ACTIVE
	bbm_write(handle, BBM_INT_POLAR_SEL, 0x00);
#endif

#ifdef BBM_I2C_SPI
	bbm_write(handle, BBM_BUF_SPIOUT, 0x10 | (BBM_I2C_SPI_PHA << 1) |
							(BBM_I2C_SPI_POL));
#endif

	bbm_write(handle, BBM_INT_AUTO_CLEAR, 0x00);
	bbm_write(handle, BBM_BUF_ENABLE, 0x01);
	bbm_write(handle, BBM_BUF_INT, 0x01);
	bbm_write(handle, BBM_INT_MASK, 0x01);
	bbm_write(handle, BBM_INT_STS_EN, 0x01);

	return BBM_OK;
}

s32 fc8180_deinit(HANDLE handle)
{
	bbm_write(handle, BBM_SW_RESET, 0x00);

	return BBM_OK;
}

s32 fc8180_scan_status(HANDLE handle)
{
	u32   ifagc_timeout       = 7;
	u32   ofdm_timeout        = 16;
	u32   ffs_lock_timeout    = 10;
	u32   dagc_timeout        = 100; /* always done */
	u32   cfs_timeout         = 12;
	u32   tmcc_timeout        = 105;
	u32   ts_err_free_timeout = 0;
	/*s32   rssi;*/
	u8    data, data1;
	u8    lay0_mod/*, lay0_cr*/;
	s32   i;

	for (i = 0; i < ifagc_timeout; i++) {
		bbm_read(handle, 0x3025, &data);

		if (data & 0x01)
			break;

		ms_wait(10);
	}

	if (i == ifagc_timeout)
		return BBM_NOK;

	/*tuner_get_rssi(handle, &rssi);

	if (rssi < -107)
		return BBM_NOK;*/

	for (; i < ofdm_timeout; i++) {
		bbm_read(handle, 0x3025, &data);

		if (data & 0x08)
			break;

		ms_wait(10);
	}

	if (i == ofdm_timeout)
		return BBM_NOK;

	if (0 == (data & 0x04))
		return BBM_NOK;

	for (; i < ffs_lock_timeout; i++) {
		bbm_read(handle, 0x3026, &data);

		if (data & 0x10)
			break;

		ms_wait(10);
	}

	if (i == ffs_lock_timeout)
		return BBM_NOK;

	for (i = 0; i < dagc_timeout; i++) {
		bbm_read(handle, 0x3026, &data);

		if (data & 0x01)
			break;

		ms_wait(10);
	}

	if (i == dagc_timeout)
		return BBM_NOK;

	for (i = 0; i < cfs_timeout; i++) {
		bbm_read(handle, 0x3025, &data);

		if (data & 0x40)
			break;

		ms_wait(10);
	}

	if (i == cfs_timeout)
		return BBM_NOK;

	bbm_read(handle, 0x2023, &data1);
	if (data1 & 1)
		return BBM_NOK;

	for (i = 0; i < tmcc_timeout; i++) {
		bbm_read(handle, 0x3026, &data);
		if (data & 0x02)
			break;

		ms_wait(10);
	}

	if (i == tmcc_timeout)
		return BBM_NOK;

	bbm_read(handle, 0x4113, &data);
	bbm_read(handle, 0x4114, &data1);

	if (((data >> 3) & 0x1) == 0)
		return BBM_NOK;

	lay0_mod = ((data & 0x10) >> 2) | ((data & 0x20) >> 4) |
					((data & 0x40) >> 6);

	/*lay0_cr = ((data & 0x80) >> 5) | ((data1 & 0x01) << 1) |
					((data1 & 0x02) >> 1);*/

	if (!((lay0_mod == 1) || (lay0_mod == 2)))
		return BBM_NOK;

	if (((0x70 & data) == 0x40) && ((0x1c & data1) == 0x18))
		ts_err_free_timeout = 40;
	else
		ts_err_free_timeout = 65;

	for (i = 0; i < ts_err_free_timeout; i++) {
		bbm_read(handle, 0x5053, &data);
		if (data & 0x01)
			break;

		ms_wait(10);
	}

	if (i == ts_err_free_timeout)
		return BBM_NOK;

	return BBM_OK;
}
//add dtv ---shenyong@wind-mobi.com ---20161119 end 
