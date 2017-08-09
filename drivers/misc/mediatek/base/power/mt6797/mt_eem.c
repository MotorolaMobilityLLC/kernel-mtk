unsigned int reg_dump_addr_off[] = {
	0x0000,
	0x0004,
	0x0008,
	0x000C,
	0x0010,
	0x0014,
	0x0018,
	0x001c,
	0x0024,
	0x0028,
	0x002c,
	0x0030,
	0x0034,
	0x0038,
	0x003c,
	0x0040,
	0x0044,
	0x0048,
	0x004c,
	0x0050,
	0x0054,
	0x0058,
	0x005c,
	0x0060,
	0x0064,
	0x0068,
	0x006c,
	0x0070,
	0x0074,
	0x0078,
	0x007c,
	0x0080,
	0x0084,
	0x0088,
	0x008c,
	0x0090,
	0x0094,
	0x0098,
	0x00a0,
	0x00a4,
	0x00a8,
	0x00B0,
	0x00B4,
	0x00B8,
	0x00BC,
	0x00C0,
	0x00C4,
	0x00C8,
	0x00CC,
	0x00F0,
	0x00F4,
	0x00F8,
	0x00FC,
	0x0200,
	0x0204,
	0x0208,
	0x020C,
	0x0210,
	0x0214,
	0x0218,
	0x021C,
	0x0220,
	0x0224,
	0x0228,
	0x022C,
	0x0230,
	0x0234,
	0x0238,
	0x023C,
	0x0240,
	0x0244,
	0x0248,
	0x024C,
	0x0250,
	0x0254,
	0x0258,
	0x025C,
	0x0260,
	0x0264,
	0x0268,
	0x026C,
	0x0270,
	0x0274,
	0x0278,
	0x027C,
	0x0280,
	0x0284,
	0x0400,
	0x0404,
	0x0408,
	0x040C,
	0x0410,
	0x0414,
	0x0418,
	0x041C,
	0x0420,
	0x0424,
	0x0428,
	0x042C,
	0x0430,
};

/* BIG */
unsigned int bigFreq_FY[16] = {
	2288000, 2262000, 2197000, 2132000, 2093000, 1989000, 1781000, 1677000,
	1495000, 1378000, 1248000, 1131000, 1001000, 845000, 676000, 338000};

unsigned int bigFreq_SB[16] = {
	2457000, 2405000, 2340000, 2262000, 2223000, 2158000, 2093000, 1885000,
	1677000, 1495000, 1378000, 1131000, 1001000, 845000, 676000, 338000};

unsigned int bigFreq_MB[16] = {
	1989000, 1976000, 1911000, 1859000, 1820000, 1729000, 1547000, 1456000,
	1300000, 1196000, 1092000, 988000, 871000, 728000, 585000, 304000};

/* GPU */
unsigned int gpuFreq_FY[16] = {
	700000, 700000, 610000, 610000, 520000, 520000, 442500, 442500,
	365000, 365000, 301500, 301500, 238000, 238000, 154500, 154500};

unsigned int gpuFreq_SB[16] = {
	850000, 850000, 700000, 700000, 610000, 610000, 520000, 520000,
	442500, 442500, 365000, 365000, 238000, 238000, 154500, 154500};

unsigned int gpuFreq_MB[16] = {
	600000, 600000, 522000, 522000, 445000, 445000, 379000, 379000,
	313000, 313000, 258000, 258000, 204000, 204000, 132000, 132000};

/* L */
unsigned int L_Freq_FY[16] = {
	1950000, 1872000, 1794000, 1716000, 1573000, 1495000, 1352000, 1274000,
	1144000, 1014000, 871000, 780000, 689000, 598000, 494000, 338000};

unsigned int L_Freq_SB[16] = {
	2054000, 2015000, 1898000, 1833000, 1755000, 1677000, 1599000, 1482000,
	1326000, 1183000, 1066000, 962000, 819000, 637000, 494000 , 338000};

unsigned int L_Freq_MB[16] = {
	1898000, 1807000, 1729000, 1625000, 1495000, 1417000, 1274000, 1209000,
	1079000, 949000, 832000, 741000, 650000, 559000, 468000, 325000};

/* LL */
unsigned int LL_Freq_FY[16] = {
	1391000, 1339000, 1287000, 1222000, 1118000, 1066000, 949000, 897000,
	806000, 715000, 624000, 559000, 481000, 416000, 338000, 221000};

unsigned int LL_Freq_SB[16] = {
	1547000, 1495000, 1443000, 1391000, 1339000, 1274000, 1222000, 1118000,
	1014000, 897000, 806000, 715000, 624000, 481000, 338000, 221000};

unsigned int LL_Freq_MB[16] = {
	1391000, 1339000, 1287000, 1222000, 1118000, 1066000, 949000, 897000,
	806000, 715000, 624000, 559000, 481000, 416000, 338000, 221000};

/* CCI */
unsigned int cciFreq_FY[16] = {
	975000, 936000, 897000, 858000, 793000, 754000, 689000, 624000,
	559000, 494000, 429000, 390000, 338000, 299000, 247000, 182000};

unsigned int cciFreq_SB[16] = {
	1014000, 988000, 949000, 910000, 871000, 845000, 806000, 741000,
	650000, 598000, 533000, 494000, 403000, 325000, 247000, 182000};

unsigned int cciFreq_MB[16] = {
	949000, 923000, 884000, 858000, 793000, 741000, 663000, 624000,
	559000, 494000, 429000, 390000, 338000, 299000, 247000, 182000};

/*
[25:21] dcmdiv
[20:12] DDS
[11:7] clkdiv; ARM PLL Divider
[6:4] postdiv
[3:0] CF index

[31:14] iDVFS %(/2)
[13:7] Vsram pmic value
[6:0] Vproc pmic value
*/
/*For DA9210 PMIC (0.3 + (x * 0.01)) */
unsigned int fyTbl[][8] = {
/* dcmdiv, DDS, clk_div, post_div, CF_index, iDVFS, Vsram, Vproc */
	{0x13, 0x0D6, 0x8, 0x0, 0xA, 0x0, 0x0F, 0x5A},/* 1391 (LL) */
	{0x13, 0x0CE, 0x8, 0x0, 0x9, 0x0, 0x0F, 0x58},/* 1339 */
	{0x12, 0x0C6, 0x8, 0x0, 0x9, 0x0, 0x0F, 0x55},/* 1287 */
	{0x11, 0x0BC, 0x8, 0x0, 0x8, 0x0, 0x0F, 0x53},/* 1222 */
	{0x0F, 0x158, 0x8, 0x1, 0x7, 0x0, 0x0F, 0x4F},/* 1118 */
	{0x0F, 0x148, 0x8, 0x1, 0x7, 0x0, 0x0E, 0x4D},/* 1066 */
	{0x0D, 0x124, 0x8, 0x1, 0x6, 0x0, 0x0D, 0x4A},/* 949 */
	{0x0C, 0x114, 0x8, 0x1, 0x6, 0x0, 0x0B, 0x46},/* 897 */
	{0x0B, 0x0F8, 0x8, 0x1, 0x5, 0x0, 0x0A, 0x43},/* 806 */
	{0x0A, 0x0DC, 0x8, 0x1, 0x4, 0x0, 0x09, 0x3F},/* 715 */
	{0x08, 0x0C0, 0x8, 0x1, 0x3, 0x0, 0x07, 0x3C},/* 624 */
	{0x07, 0x158, 0xA, 0x1, 0x2, 0x0, 0x07, 0x3A},/* 559 */
	{0x06, 0x128, 0xA, 0x1, 0x1, 0x0, 0x07, 0x37},/* 481 */
	{0x05, 0x100, 0xA, 0x1, 0x1, 0x0, 0x07, 0x35},/* 416 */
	{0x04, 0x0D0, 0xA, 0x1, 0x0, 0x0, 0x07, 0x32},/* 338 */
	{0x03, 0x110, 0xB, 0x1, 0x0, 0x0, 0x07, 0x30},/* 221 */

	{0x1B, 0x12C, 0x8, 0x0, 0xF, 0x0, 0x0F, 0x5A},/* 1950 (L) */
	{0x1A, 0x120, 0x8, 0x0, 0xE, 0x0, 0x0F, 0x58},/* 1872 */
	{0x19, 0x114, 0x8, 0x0, 0xD, 0x0, 0x0F, 0x55},/* 1794 */
	{0x18, 0x108, 0x8, 0x0, 0xC, 0x0, 0x0F, 0x53},/* 1716 */
	{0x16, 0x0F2, 0x8, 0x0, 0xB, 0x0, 0x0F, 0x4F},/* 1573 */
	{0x15, 0x0E6, 0x8, 0x0, 0xA, 0x0, 0x0E, 0x4D},/* 1495 */
	{0x13, 0x0D0, 0x8, 0x0, 0x9, 0x0, 0x0D, 0x4A},/* 1352 */
	{0x12, 0x0C4, 0x8, 0x0, 0x9, 0x0, 0x0B, 0x46},/* 1274 */
	{0x10, 0x0B0, 0x8, 0x0, 0x8, 0x0, 0x0A, 0x43},/* 1144 */
	{0x0E, 0x09C, 0x8, 0x0, 0x7, 0x0, 0x09, 0x3F},/* 1014 */
	{0x0C, 0x10C, 0x8, 0x1, 0x6, 0x0, 0x07, 0x3C},/* 871 */
	{0x0B, 0x0F0, 0x8, 0x1, 0x4, 0x0, 0x07, 0x3A},/* 780 */
	{0x09, 0x0D4, 0x8, 0x1, 0x4, 0x0, 0x07, 0x37},/* 689 */
	{0x08, 0x0B8, 0x8, 0x1, 0x2, 0x0, 0x07, 0x35},/* 598 */
	{0x07, 0x130, 0xA, 0x1, 0x1, 0x0, 0x07, 0x32},/* 494 */
	{0x04, 0x0D0, 0xA, 0x1, 0x0, 0x0, 0x07, 0x30},/* 338 */

	{0x0D, 0x12C, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x5A},/* 975 (CCI) */
	{0x0D, 0x120, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x58},/* 936 */
	{0x0C, 0x114, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x55},/* 897 */
	{0x0C, 0x108, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x53},/* 858 */
	{0x0B, 0x0F4, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x4F},/* 793 */
	{0x0A, 0x0E8, 0x8, 0x1, 0x0, 0x0, 0x0E, 0x4D},/* 754 */
	{0x09, 0x0D4, 0x8, 0x1, 0x0, 0x0, 0x0D, 0x4A},/* 689 */
	{0x08, 0x0C0, 0x8, 0x1, 0x0, 0x0, 0x0B, 0x46},/* 624 */
	{0x07, 0x158, 0xA, 0x1, 0x0, 0x0, 0x0A, 0x43},/* 559 */
	{0x07, 0x130, 0xA, 0x1, 0x0, 0x0, 0x09, 0x3F},/* 494 */
	{0x06, 0x108, 0xA, 0x1, 0x0, 0x0, 0x07, 0x3C},/* 429 */
	{0x05, 0x0F0, 0xA, 0x1, 0x0, 0x0, 0x07, 0x3A},/* 390 */
	{0x04, 0x0D0, 0xA, 0x1, 0x0, 0x0, 0x07, 0x37},/* 338 */
	{0x04, 0x0B8, 0xA, 0x1, 0x0, 0x0, 0x07, 0x35},/* 299 */
	{0x03, 0x130, 0xB, 0x1, 0x0, 0x0, 0x07, 0x32},/* 247 */
	{0x02, 0x0E0, 0xB, 0x1, 0x0, 0x0, 0x07, 0x30},/* 182 */

	{0x1F, 0x160, 0x8, 0x0, 0xF, 0x2DC29, 0x0F, 0x58},/* 2288 (BIG) */
	{0x1F, 0x15C, 0x8, 0x0, 0xF, 0x2D3D7, 0x0F, 0x58},/* 2262 */
	{0x1F, 0x152, 0x8, 0x0, 0xF, 0x2BF0A, 0x0F, 0x56},/* 2197 */
	{0x1E, 0x148, 0x8, 0x0, 0xF, 0x2AA3D, 0x0F, 0x54},/* 2132 */
	{0x1D, 0x142, 0x8, 0x0, 0xF, 0x29DC3, 0x0F, 0x53},/* 2093 */
	{0x1C, 0x132, 0x8, 0x0, 0xF, 0x27C7B, 0x0F, 0x51},/* 1989 */
	{0x19, 0x112, 0x8, 0x0, 0xD, 0x239EC, 0x0E, 0x4D},/* 1781 */
	{0x17, 0x102, 0x8, 0x0, 0xC, 0x218A4, 0x0D, 0x4B},/* 1677 */
	{0x15, 0x0E6, 0x8, 0x0, 0xA, 0x1DE66, 0x0B, 0x46},/* 1495 */
	{0x13, 0x0D4, 0x8, 0x0, 0x9, 0x1B8F6, 0x0B, 0x44},/* 1378 */
	{0x11, 0x0C0, 0x8, 0x0, 0x8, 0x18F5C, 0x09, 0x41},/* 1248 */
	{0x10, 0x15C, 0x8, 0x1, 0x8, 0x169EC, 0x09, 0x3F},/* 1131 */
	{0x0E, 0x134, 0x8, 0x1, 0x7, 0x14052, 0x07, 0x3C},/* 1001 */
	{0x0C, 0x104, 0x8, 0x1, 0x5, 0x10E66, 0x07, 0x3A},/* 845 */
	{0x09, 0x0D0, 0x8, 0x1, 0x3, 0x0D852, 0x07, 0x37},/* 676 */
	{0x04, 0x09C, 0x1A, 0x1, 0x0, 0x06C29, 0x07, 0x32},/* 338 */
};

unsigned int sbTbl[][8] = {
	{0x16, 0x0EE, 0x8, 0x0, 0x9, 0x0, 0x0F, 0x5A},/* 1547 (LL) */
	{0x15, 0x0E6, 0x8, 0x0, 0x9, 0x0, 0x0F, 0x59},/* 1495 */
	{0x14, 0x0DE, 0x8, 0x0, 0x8, 0x0, 0x0F, 0x57},/* 1443 */
	{0x13, 0x0D6, 0x8, 0x0, 0x8, 0x0, 0x0F, 0x55},/* 1391 */
	{0x13, 0x0CE, 0x8, 0x0, 0x8, 0x0, 0x0F, 0x53},/* 1339 */
	{0x12, 0x0C4, 0x8, 0x0, 0x7, 0x0, 0x0F, 0x51},/* 1274 */
	{0x11, 0x0BC, 0x8, 0x0, 0x7, 0x0, 0x0F, 0x4F},/* 1222 */
	{0x0F, 0x158, 0x8, 0x1, 0x6, 0x0, 0x0E, 0x4C},/* 1118 */
	{0x0E, 0x138, 0x8, 0x1, 0x5, 0x0, 0x0C, 0x48},/* 1014 */
	{0x0C, 0x114, 0x8, 0x1, 0x4, 0x0, 0x0B, 0x45},/* 897 */
	{0x0B, 0x0F8, 0x8, 0x1, 0x3, 0x0, 0x09, 0x41},/* 806 */
	{0x0A, 0x0DC, 0x8, 0x1, 0x3, 0x0, 0x09, 0x3F},/* 715 */
	{0x08, 0x0C0, 0x8, 0x1, 0x2, 0x0, 0x07, 0x3B},/* 624 */
	{0x06, 0x128, 0xA, 0x1, 0x1, 0x0, 0x07, 0x37},/* 481 */
	{0x04, 0x0D0, 0xA, 0x1, 0x0, 0x0, 0x07, 0x32},/* 338 */
	{0x03, 0x110, 0xB, 0x1, 0x0, 0x0, 0x07, 0x2F},/* 221 */

	{0x1D, 0x13C, 0x8, 0x0, 0xF, 0x0, 0x0F, 0x5A},/* 2054  (L) */
	{0x1C, 0x136, 0x8, 0x0, 0xF, 0x0, 0x0F, 0x59},/* 2015  */
	{0x1B, 0x124, 0x8, 0x0, 0xD, 0x0, 0x0F, 0x57},/* 1898  */
	{0x1A, 0x11A, 0x8, 0x0, 0xD, 0x0, 0x0F, 0x55},/* 1833  */
	{0x19, 0x10E, 0x8, 0x0, 0xC, 0x0, 0x0F, 0x53},/* 1755  */
	{0x17, 0x102, 0x8, 0x0, 0xA, 0x0, 0x0F, 0x51},/* 1677  */
	{0x16, 0x0F6, 0x8, 0x0, 0x9, 0x0, 0x0F, 0x4F},/* 1599  */
	{0x15, 0x0E4, 0x8, 0x0, 0x8, 0x0, 0x0E, 0x4C},/* 1482  */
	{0x12, 0x0CC, 0x8, 0x0, 0x8, 0x0, 0x0C, 0x48},/* 1326  */
	{0x10, 0x0B6, 0x8, 0x0, 0x6, 0x0, 0x0B, 0x45},/* 1183  */
	{0x0F, 0x148, 0x8, 0x1, 0x5, 0x0, 0x09, 0x41},/* 1066  */
	{0x0D, 0x128, 0x8, 0x1, 0x4, 0x0, 0x09, 0x3F},/* 962  */
	{0x0B, 0x0FC, 0x8, 0x1, 0x4, 0x0, 0x07, 0x3B},/* 819  */
	{0x09, 0x0C4, 0x8, 0x1, 0x2, 0x0, 0x07, 0x37},/* 637  */
	{0x07, 0x130, 0xA, 0x1, 0x1, 0x0, 0x07, 0x32},/* 494  */
	{0x04, 0x0D0, 0xA, 0x1, 0x0, 0x0, 0x07, 0x2F},/* 338  */

	{0x0E, 0x138, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x5A},/* 1014  (CCI) */
	{0x0E, 0x130, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x59},/* 988  */
	{0x0D, 0x124, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x57},/* 949  */
	{0x0D, 0x118, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x55},/* 910  */
	{0x0C, 0x10C, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x53},/* 871  */
	{0x0C, 0x104, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x51},/* 845  */
	{0x0B, 0x0F8, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x4F},/* 806  */
	{0x0A, 0x0E4, 0x8, 0x1, 0x0, 0x0, 0x0E, 0x4C},/* 741  */
	{0x09, 0x0C8, 0x8, 0x1, 0x0, 0x0, 0x0C, 0x48},/* 650  */
	{0x08, 0x0B8, 0x8, 0x1, 0x0, 0x0, 0x0B, 0x45},/* 598  */
	{0x07, 0x148, 0xA, 0x1, 0x0, 0x0, 0x09, 0x41},/* 533  */
	{0x07, 0x130, 0xA, 0x1, 0x0, 0x0, 0x09, 0x3F},/* 494  */
	{0x05, 0x0F8, 0xA, 0x1, 0x0, 0x0, 0x07, 0x3B},/* 403  */
	{0x04, 0x0C8, 0xA, 0x1, 0x0, 0x0, 0x07, 0x37},/* 325  */
	{0x03, 0x130, 0xB, 0x1, 0x0, 0x0, 0x07, 0x32},/* 247  */
	{0x02, 0x0E0, 0xB, 0x1, 0x0, 0x0, 0x07, 0x2F},/* 182  */

	{0x1F, 0x17A, 0x8, 0x0, 0xF, 0x3123D, 0x0F, 0x58},/* 2457 (BIG) */
	{0x1F, 0x172, 0x8, 0x0, 0xF, 0x3019A, 0x0F, 0x57},/* 2405 */
	{0x1F, 0x168, 0x8, 0x0, 0xF, 0x2ECCD, 0x0F, 0x56},/* 2340 */
	{0x1F, 0x15C, 0x8, 0x0, 0xF, 0x2D3D7, 0x0F, 0x55},/* 2262 */
	{0x1F, 0x156, 0x8, 0x0, 0xF, 0x2C75C, 0x0F, 0x54},/* 2223 */
	{0x1E, 0x14C, 0x8, 0x0, 0xF, 0x2B28F, 0x0F, 0x53},/* 2158 */
	{0x1D, 0x142, 0x8, 0x0, 0xF, 0x29DC3, 0x0F, 0x51},/* 2093 */
	{0x1A, 0x122, 0x8, 0x0, 0xD, 0x25B33, 0x0F, 0x4E},/* 1885 */
	{0x17, 0x102, 0x8, 0x0, 0xA, 0x218A4, 0x0D, 0x4A},/* 1677 */
	{0x15, 0x0E6, 0x8, 0x0, 0x9, 0x1DE66, 0x0B, 0x46},/* 1495 */
	{0x13, 0x0D4, 0x8, 0x0, 0x8, 0x1B8F6, 0x0B, 0x44},/* 1378 */
	{0x10, 0x15C, 0x8, 0x1, 0x6, 0x169EC, 0x09, 0x3F},/* 1131 */
	{0x0E, 0x134, 0x8, 0x1, 0x5, 0x14052, 0x07, 0x3C},/* 1001 */
	{0x0C, 0x104, 0x8, 0x1, 0x4, 0x10E66, 0x07, 0x3A},/* 845 */
	{0x09, 0x0D0, 0x8, 0x1, 0x3, 0x0D852, 0x07, 0x37},/* 676 */
	{0x04, 0x09C, 0x1A, 0x1, 0x0, 0x06C29, 0x07, 0x32},/* 338 */
};

/* dcmdiv, DDS, clk_div, post_div, CF_index, iDVFS, Vsram, Vproc */
unsigned int mbTbl[][8] = {
	{0x13, 0x0D6, 0x8, 0x0, 0xA, 0x0, 0x0F, 0x5A},/* 1391 (LL) */
	{0x13, 0x0CE, 0x8, 0x0, 0xA, 0x0, 0x0F, 0x58},/* 1339 */
	{0x12, 0x0C6, 0x8, 0x0, 0x9, 0x0, 0x0F, 0x55},/* 1287 */
	{0x11, 0x0BC, 0x8, 0x0, 0x8, 0x0, 0x0F, 0x53},/* 1222 */
	{0x0F, 0x158, 0x8, 0x1, 0x7, 0x0, 0x0F, 0x4F},/* 1118 */
	{0x0F, 0x148, 0x8, 0x1, 0x7, 0x0, 0x0E, 0x4D},/* 1066 */
	{0x0D, 0x124, 0x8, 0x1, 0x6, 0x0, 0x0C, 0x48},/* 949 */
	{0x0C, 0x114, 0x8, 0x1, 0x6, 0x0, 0x0B, 0x46},/* 897 */
	{0x0B, 0x0F8, 0x8, 0x1, 0x5, 0x0, 0x0A, 0x43},/* 806 */
	{0x0A, 0x0DC, 0x8, 0x1, 0x4, 0x0, 0x09, 0x3F},/* 715 */
	{0x08, 0x0C0, 0x8, 0x1, 0x3, 0x0, 0x07, 0x3C},/* 624 */
	{0x07, 0x0AC, 0x8, 0x1, 0x2, 0x0, 0x07, 0x3A},/* 559 */
	{0x06, 0x128, 0xA, 0x1, 0x1, 0x0, 0x07, 0x37},/* 481 */
	{0x05, 0x100, 0xA, 0x1, 0x1, 0x0, 0x07, 0x35},/* 416 */
	{0x04, 0x0D0, 0xA, 0x1, 0x0, 0x0, 0x07, 0x32},/* 338 */
	{0x03, 0x110, 0xB, 0x1, 0x0, 0x0, 0x07, 0x30},/* 221 */

	{0x1B, 0x124, 0x8, 0x0, 0xF, 0x0, 0x0F, 0x5A},/* 1898 (L) */
	{0x19, 0x116, 0x8, 0x0, 0xE, 0x0, 0x0F, 0x58},/* 1807 */
	{0x18, 0x10A, 0x8, 0x0, 0xD, 0x0, 0x0F, 0x55},/* 1729 */
	{0x17, 0x0FA, 0x8, 0x0, 0xC, 0x0, 0x0F, 0x53},/* 1625 */
	{0x15, 0x0E6, 0x8, 0x0, 0xB, 0x0, 0x0F, 0x4F},/* 1495 */
	{0x14, 0x0DA, 0x8, 0x0, 0xA, 0x0, 0x0E, 0x4D},/* 1417 */
	{0x12, 0x0C4, 0x8, 0x0, 0x9, 0x0, 0x0C, 0x48},/* 1274 */
	{0x11, 0x0BA, 0x8, 0x0, 0x8, 0x0, 0x0B, 0x46},/* 1209 */
	{0x0F, 0x0A6, 0x8, 0x0, 0x7, 0x0, 0x0A, 0x43},/* 1079 */
	{0x0D, 0x124, 0x8, 0x1, 0x6, 0x0, 0x09, 0x3F},/* 949 */
	{0x0B, 0x100, 0x8, 0x1, 0x5, 0x0, 0x07, 0x3C},/* 832 */
	{0x0A, 0x0E4, 0x8, 0x1, 0x4, 0x0, 0x07, 0x3A},/* 741 */
	{0x09, 0x0C8, 0x8, 0x1, 0x3, 0x0, 0x07, 0x37},/* 650 */
	{0x07, 0x0AC, 0x8, 0x1, 0x2, 0x0, 0x07, 0x35},/* 559 */
	{0x06, 0x120, 0xA, 0x1, 0x1, 0x0, 0x07, 0x32},/* 468 */
	{0x04, 0x0C8, 0xA, 0x1, 0x0, 0x0, 0x07, 0x30},/* 325 */

	{0x0D, 0x124, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x5A},/* 949 (CCI) */
	{0x0D, 0x11C, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x58},/* 923 */
	{0x0C, 0x110, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x55},/* 884 */
	{0x0C, 0x108, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x53},/* 858 */
	{0x0B, 0x0F4, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x4F},/* 793 */
	{0x0A, 0x0E4, 0x8, 0x1, 0x0, 0x0, 0x0E, 0x4D},/* 741 */
	{0x09, 0x0CC, 0x8, 0x1, 0x0, 0x0, 0x0C, 0x48},/* 663 */
	{0x08, 0x0C0, 0x8, 0x1, 0x0, 0x0, 0x0B, 0x46},/* 624 */
	{0x07, 0x158, 0xA, 0x1, 0x0, 0x0, 0x0A, 0x43},/* 559 */
	{0x07, 0x130, 0xA, 0x1, 0x0, 0x0, 0x09, 0x3F},/* 494 */
	{0x06, 0x108, 0xA, 0x1, 0x0, 0x0, 0x07, 0x3C},/* 429 */
	{0x05, 0x0F0, 0xA, 0x1, 0x0, 0x0, 0x07, 0x3A},/* 390 */
	{0x04, 0x0D0, 0xA, 0x1, 0x0, 0x0, 0x07, 0x37},/* 338 */
	{0x04, 0x0B8, 0xA, 0x1, 0x0, 0x0, 0x07, 0x35},/* 299 */
	{0x03, 0x130, 0xB, 0x1, 0x0, 0x0, 0x07, 0x32},/* 247 */
	{0x02, 0x0E0, 0xB, 0x1, 0x0, 0x0, 0x07, 0x30},/* 182 */

	{0x1C, 0x132, 0x8, 0x0, 0xF, 0x27C7B, 0x0F, 0x58},/* 1989 (BIG) */
	{0x1C, 0x130, 0x8, 0x0, 0xF, 0x27852, 0x0F, 0x58},/* 1976 */
	{0x1B, 0x126, 0x8, 0x0, 0xF, 0x26385, 0x0F, 0x56},/* 1911 */
	{0x1A, 0x11E, 0x8, 0x0, 0xF, 0x252E1, 0x0F, 0x54},/* 1859 */
	{0x1A, 0x118, 0x8, 0x0, 0xE, 0x24666, 0x0F, 0x53},/* 1820 */
	{0x18, 0x10A, 0x8, 0x0, 0xD, 0x22948, 0x0F, 0x51},/* 1729 */
	{0x16, 0x0EE, 0x8, 0x0, 0xB, 0x1EF0A, 0x0E, 0x4D},/* 1547 */
	{0x14, 0x0E0, 0x8, 0x0, 0xA, 0x1D1EC, 0x0D, 0x4B},/* 1456 */
	{0x12, 0x0C8, 0x8, 0x0, 0x9, 0x1A000, 0x0B, 0x46},/* 1300 */
	{0x11, 0x0B8, 0x8, 0x0, 0x8, 0x17EB8, 0x0B, 0x44},/* 1196 */
	{0x0F, 0x0A8, 0x8, 0x0, 0x7, 0x15D71, 0x09, 0x41},/* 1092 */
	{0x0E, 0x130, 0x8, 0x1, 0x6, 0x13C29, 0x09, 0x3F},/* 988 */
	{0x0C, 0x10C, 0x8, 0x1, 0x6, 0x116B8, 0x07, 0x3C},/* 871 */
	{0x0A, 0x0E0, 0x8, 0x1, 0x4, 0x0E8F6, 0x07, 0x3A},/* 728 */
	{0x08, 0x0B4, 0x8, 0x1, 0x2, 0x0BB33, 0x07, 0x37},/* 585 */
	{0x04, 0x09C, 0x12, 0x1, 0x0, 0x06148, 0x07, 0x32},/* 304 */
};

unsigned int gpuFy[16] = {0x29, 0x29, 0x25, 0x25, 0x1F, 0x1F, 0x1D, 0x1D,
			0x18, 0x18, 0x17, 0x17, 0x14, 0x14, 0x10, 0x10};

unsigned int gpuSb[16] = {0x27, 0x27, 0x22, 0x22, 0x1E, 0x1E, 0x1C, 0x1C,
			0x19, 0x19, 0x17, 0x17, 0x13, 0x13, 0x10, 0x10};

unsigned int gpuMb[16] = {0x29, 0x29, 0x26, 0x26, 0x21, 0x21, 0x1E, 0x1E,
			0x1A, 0x1A, 0x17, 0x17, 0x14, 0x14, 0x10, 0x10};

unsigned int gpuOutput[8];
static unsigned int record_tbl_locked[16];
static unsigned int *recordTbl, *gpuTbl;
static unsigned int cpu_speed;

/**
 * @file    mt_ptp.c
 * @brief   Driver for EEM
 *
 */

#define __MT_EEM_C__

/* Early porting use that avoid to cause compiler error */
/* #define EARLY_PORTING */

/*=============================================================
 * Include files
 *=============================================================*/

/* system includes */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/platform_device.h>
#include <linux/completion.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/topology.h>

/* project includes */
#include "mach/irqs.h"
#include "mach/mt_freqhopping.h"
#include "mach/mtk_rtc_hal.h"
#include "mach/mt_rtc_hw.h"

#ifdef CONFIG_OF
	#include <linux/cpu.h>
	#include <linux/of.h>
	#include <linux/of_irq.h>
	#include <linux/of_address.h>
	#include <linux/of_fdt.h>
	#include <mt-plat/aee.h>
	#if defined(CONFIG_MTK_CLKMGR)
		#include <mach/mt_clkmgr.h>
	#else
		#include <linux/clk.h>
	#endif
#endif

/* local includes */

#ifdef __KERNEL__
	#include <mt-plat/mt_chip.h>
	#include <mt-plat/mt_gpio.h>
	#include "mt-plat/upmu_common.h"
	#include "../../../include/mt-plat/mt6797/include/mach/mt_thermal.h"
	#include "mach/mt_ppm_api.h"
	#include "mt_gpufreq.h"
	#include "mt_cpufreq.h"
	#include "mt_ptp.h"
	#include "mt_defptp.h"
	#include "spm_v2/mt_spm_idle.h"
	#include "spm_v2/mt_spm.h"
	#include "../../../power/mt6797/da9214.h"
	#include "../../../power/mt6797/fan53555.h"
#else
	#include "mach/mt_cpufreq.h"
	#include "mach/mt_ptp.h"
	#include "mach/mt_ptpslt.h"
	#include "mach/mt_defptp.h"
	#include "kernel2ctp.h"
	#include "ptp.h"
	#include "upmu_common.h"
	/* #include "mt6311.h" */
	#ifndef EARLY_PORTING
		/* #include "gpu_dvfs.h" */
		#include "thermal.h"
		/* #include "gpio_const.h" */
	#endif
#endif

#ifdef ENABLE_IDVFS
static unsigned int ctrl_EEM_Enable = 1;
#else
static unsigned int ctrl_EEM_Enable = 1;
#endif
static unsigned int ctrl_ITurbo = 0, ITurboRun;
static int eem_log_en;
static int isGPUDCBDETOverflow, is2LDCBDETOverflow, isLDCBDETOverflow, isCCIDCBDETOverflow;

static unsigned int checkEfuse;
static unsigned int informGpuEEMisReady;

/* Global variable for slow idle*/
volatile unsigned int ptp_data[3] = {0, 0, 0};

/* Global variable for I DVFS use */
unsigned int infoIdvfs;

struct eem_det;
struct eem_ctrl;
u32 *recordRef;

#if (defined(__KERNEL__) && !defined(CONFIG_MTK_CLKMGR)) && !defined(EARLY_PORTING)
	struct clk *clk_thermal;
	struct clk *clk_mfg, *clk_mfg_scp; /* for gpu clock use */
#endif

static void eem_set_eem_volt(struct eem_det *det);
static void eem_restore_eem_volt(struct eem_det *det);

/*=============================================================
 * Macro definition
 *=============================================================*/

/*
 * CONFIG (SW related)
 */
/* #define CONFIG_EEM_SHOWLOG */

#define EN_ISR_LOG		(1)

#define EEM_GET_REAL_VAL	(1) /* get val from efuse */
#define SET_PMIC_VOLT		(1) /* apply PMIC voltage */

#define DUMP_DATA_TO_DE		(1)

#define LOG_INTERVAL		(2LL * NSEC_PER_SEC)
#define NR_FREQ			16


/*
 * 100 us, This is the EEM Detector sampling time as represented in
 * cycles of bclk_ck during INIT. 52 MHz
 */
#define DETWINDOW_VAL		0x514

/*
 * mili Volt to config value. voltage = 600mV + val * 6.25mV
 * val = (voltage - 600) / 6.25
 * @mV:	mili volt
 */

/* 1mV=>10uV */
/* EEM */
#define EEM_V_BASE	(70000)
#define EEM_STEP	(625)
#define BIG_EEM_BASE	(30000) /* 45000 for MT6313 */
#define BIG_EEM_STEP	(1000) /* 625 for MT6313 */

/* SRAM */
#define SRAM_BASE	(90000)
#define SRAM_STEP	(2500)
#define EEM_2_RMIC(value)	(((((value) * EEM_STEP) + EEM_V_BASE - SRAM_BASE + SRAM_STEP - 1) / SRAM_STEP) + 3)
#define PMIC_2_RMIC(det, value)	\
	(((((value) * det->pmic_step) + det->pmic_base - SRAM_BASE + SRAM_STEP - 1) / SRAM_STEP) + 3)
#define VOLT_2_RMIC(volt)	(((((volt) - SRAM_BASE) + SRAM_STEP - 1) / SRAM_STEP) + 3)

/* BIG CPU */
#define BIG_CPU_PMIC_BASE	(30000) /* 45000 for MT6313 */
#define BIG_CPU_PMIC_STEP	(1000) /* 625 for MT6313 */

/* CPU */
#define CPU_PMIC_BASE	(30000) /* 45000 for MT6313 */
#define CPU_PMIC_STEP	(1000) /* 625 for MT6313 */

/* GPU */
#define GPU_PMIC_BASE	(60300)
#define GPU_PMIC_STEP	(1282)

#define VMAX_VAL_BIG		(120000) /* 118125 for MT6313; 120000 for DA9214 */
#define VMIN_VAL_BIG		(80000) /* 82500 for MT6313; 80000 for DA9214 */
#define VMAX_SRAM		(120000)
#define VMIN_SRAM               (100000)
#define VMAX_VAL_GPU            (112900)
#define VMIN_VAL_GPU		(80000)
#define VMAX_VAL_LITTLE		(120000)
#define VMIN_VAL_LITTLE		(78000)
#define VMIN_VAL_LITTLE_SB	(77000)

#define DTHI_VAL		0x01		/* positive */
#define DTLO_VAL		0xfe		/* negative (2's compliment) */
#define DETMAX_VAL		0xffff		/* This timeout value is in cycles of bclk_ck. */
#define AGECONFIG_VAL		0x555555
#define AGEM_VAL		0x0
#define DVTFIXED_VAL_BIG	0x4 /* 0x07 for MT6313; 0x04 for DA9214 */
#define DVTFIXED_VAL_GPU	0x5
#define DVTFIXED_VAL		0x7

#define VCO_VAL			0x0C
#define VCO_VAL_BIG             0x35 /* 0x3C for MT6313, 0x35 for DA9214 */
/* After ATE program version 5 */
#define VCO_VAL_AFTER_5		0x04
#define VCO_VAL_AFTER_5_BIG	0x32

#define VCO_VAL_GPU		0x10

#define DCCONFIG_VAL		0x555555

#if defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING)
static void _mt_eem_aee_init(void)
{
	aee_rr_rec_ptp_vboot(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_big_volt(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_big_volt_1(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_big_volt_2(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_big_volt_3(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_gpu_volt(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_gpu_volt_1(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_gpu_volt_2(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_gpu_volt_3(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_little_volt(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_little_volt_1(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_little_volt_2(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_little_volt_3(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_2_little_volt(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_2_little_volt_1(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_2_little_volt_2(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_2_little_volt_3(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_cci_volt(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_cci_volt_1(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_cci_volt_2(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_cci_volt_3(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_temp(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_status(0xFF);
}
#endif

/*
 * bit operation
 */
#undef  BIT
#define BIT(bit)	(1U << (bit))

#define MSB(range)	(1 ? range)
#define LSB(range)	(0 ? range)
/**
 * Genearte a mask wher MSB to LSB are all 0b1
 * @r:	Range in the form of MSB:LSB
 */
#define BITMASK(r)	\
	(((unsigned) -1 >> (31 - MSB(r))) & ~((1U << LSB(r)) - 1))

/**
 * Set value at MSB:LSB. For example, BITS(7:3, 0x5A)
 * will return a value where bit 3 to bit 7 is 0x5A
 * @r:	Range in the form of MSB:LSB
 */
/* BITS(MSB:LSB, value) => Set value at MSB:LSB  */
#define BITS(r, val)	((val << LSB(r)) & BITMASK(r))

#define GET_BITS_VAL(_bits_, _val_)   (((_val_) & (BITMASK(_bits_))) >> ((0) ? _bits_))
/*
 * LOG
 */
#ifdef __KERNEL__
#define EEM_TAG     "[xxxx1] "
#ifdef USING_XLOG
	#include <linux/xlog.h>
	#define eem_emerg(fmt, args...)     pr_err(ANDROID_LOG_ERROR, EEM_TAG, fmt, ##args)
	#define eem_alert(fmt, args...)     pr_err(ANDROID_LOG_ERROR, EEM_TAG, fmt, ##args)
	#define eem_crit(fmt, args...)      pr_err(ANDROID_LOG_ERROR, EEM_TAG, fmt, ##args)
	#define eem_error(fmt, args...)     pr_err(ANDROID_LOG_ERROR, EEM_TAG, fmt, ##args)
	#define eem_warning(fmt, args...)   pr_warn(ANDROID_LOG_WARN, EEM_TAG, fmt, ##args)
	#define eem_notice(fmt, args...)    pr_info(ANDROID_LOG_INFO, EEM_TAG, fmt, ##args)
	#define eem_info(fmt, args...)      pr_info(ANDROID_LOG_INFO, EEM_TAG, fmt, ##args)
	#define eem_debug(fmt, args...)     pr_debug(ANDROID_LOG_DEBUG, EEM_TAG, fmt, ##args)
#else
	#define eem_emerg(fmt, args...)     pr_emerg(EEM_TAG fmt, ##args)
	#define eem_alert(fmt, args...)     pr_alert(EEM_TAG fmt, ##args)
	#define eem_crit(fmt, args...)      pr_crit(EEM_TAG fmt, ##args)
	#define eem_error(fmt, args...)     pr_err(EEM_TAG fmt, ##args)
	#define eem_warning(fmt, args...)   pr_warn(EEM_TAG fmt, ##args)
	#define eem_notice(fmt, args...)    pr_notice(EEM_TAG fmt, ##args)
	#define eem_info(fmt, args...)      pr_info(EEM_TAG fmt, ##args)
	#define eem_debug(fmt, args...)     /* pr_debug(EEM_TAG fmt, ##args) */
#endif

	#if EN_ISR_LOG /* For Interrupt use */
		#define eem_isr_info(fmt, args...)  eem_debug(fmt, ##args)
	#else
		#define eem_isr_info(fmt, args...)
	#endif
#endif

#define FUNC_LV_MODULE          BIT(0)  /* module, platform driver interface */
#define FUNC_LV_CPUFREQ         BIT(1)  /* cpufreq driver interface          */
#define FUNC_LV_API             BIT(2)  /* mt_cpufreq driver global function */
#define FUNC_LV_LOCAL           BIT(3)  /* mt_cpufreq driver lcaol function  */
#define FUNC_LV_HELP            BIT(4)  /* mt_cpufreq driver help function   */


#if defined(CONFIG_EEM_SHOWLOG)
	static unsigned int func_lv_mask = (
		FUNC_LV_MODULE |
		FUNC_LV_CPUFREQ |
		FUNC_LV_API |
		FUNC_LV_LOCAL |
		FUNC_LV_HELP
		);
	#define FUNC_ENTER(lv)	\
		do { if ((lv) & func_lv_mask) eem_debug(">> %s()\n", __func__); } while (0)
	#define FUNC_EXIT(lv)	\
		do { if ((lv) & func_lv_mask) eem_debug("<< %s():%d\n", __func__, __LINE__); } while (0)
#else
	#define FUNC_ENTER(lv)
	#define FUNC_EXIT(lv)
#endif /* CONFIG_CPU_DVFS_SHOWLOG */

/*
 * REG ACCESS
 */
#ifdef __KERNEL__
	#define eem_read(addr)	DRV_Reg32(addr)
	#define eem_read_field(addr, range)	\
		((eem_read(addr) & BITMASK(range)) >> LSB(range))
	#define eem_write(addr, val)	mt_reg_sync_writel(val, addr)
#endif
/**
 * Write a field of a register.
 * @addr:	Address of the register
 * @range:	The field bit range in the form of MSB:LSB
 * @val:	The value to be written to the field
 */
#define eem_write_field(addr, range, val)	\
	eem_write(addr, (eem_read(addr) & ~BITMASK(range)) | BITS(range, val))

/**
 * Helper macros
 */

/* EEM detector is disabled by who */
enum {
	BY_PROCFS	= BIT(0),
	BY_INIT_ERROR	= BIT(1),
	BY_MON_ERROR	= BIT(2),
	BY_PROCFS_INIT2 = BIT(3),
};

#ifdef CONFIG_OF
void __iomem *eem_base;
static u32 eem_irq_number;
#endif

/**
 * iterate over list of detectors
 * @det:	the detector * to use as a loop cursor.
 */
#define for_each_det(det) for (det = eem_detectors; det < (eem_detectors + ARRAY_SIZE(eem_detectors)); det++)

/**
 * iterate over list of detectors and its controller
 * @det:	the detector * to use as a loop cursor.
 * @ctrl:	the eem_ctrl * to use as ctrl pointer of current det.
 */
#define for_each_det_ctrl(det, ctrl)				\
	for (det = eem_detectors,				\
	     ctrl = id_to_eem_ctrl(det->ctrl_id);		\
	     det < (eem_detectors + ARRAY_SIZE(eem_detectors)); \
	     det++,						\
	     ctrl = id_to_eem_ctrl(det->ctrl_id))

/**
 * iterate over list of controllers
 * @pos:	the eem_ctrl * to use as a loop cursor.
 */
#define for_each_ctrl(ctrl) for (ctrl = eem_ctrls; ctrl < (eem_ctrls + ARRAY_SIZE(eem_ctrls)); ctrl++)

/**
 * Given a eem_det * in eem_detectors. Return the id.
 * @det:	pointer to a eem_det in eem_detectors
 */
#define det_to_id(det)	((det) - &eem_detectors[0])

/**
 * Given a eem_ctrl * in eem_ctrls. Return the id.
 * @det:	pointer to a eem_ctrl in eem_ctrls
 */
#define ctrl_to_id(ctrl)	((ctrl) - &eem_ctrls[0])

/**
 * Check if a detector has a feature
 * @det:	pointer to a eem_det to be check
 * @feature:	enum eem_features to be checked
 */
#define HAS_FEATURE(det, feature)	((det)->features & feature)

#define PERCENT(numerator, denominator)	\
	(unsigned char)(((numerator) * 100 + (denominator) - 1) / (denominator))

/*=============================================================
 * Local type definition
 *=============================================================*/

/*
 * CONFIG (CHIP related)
 * EEMCORESEL.APBSEL
 */

enum eem_phase {
	EEM_PHASE_INIT01,
	EEM_PHASE_INIT02,
	EEM_PHASE_MON,

	NR_EEM_PHASE,
};

enum {
	EEM_VOLT_NONE    = 0,
	EEM_VOLT_UPDATE  = BIT(0),
	EEM_VOLT_RESTORE = BIT(1),
};

struct eem_ctrl {
	const char *name;
	enum eem_det_id det_id;
	/* struct completion init_done; */
	/* atomic_t in_init; */
#ifdef __KERNEL__
	/* for voltage setting thread */
	wait_queue_head_t wq;
#endif
	int volt_update;
	struct task_struct *thread;
};

struct eem_det_ops {
	/* interface to EEM */
	void (*enable)(struct eem_det *det, int reason);
	void (*disable)(struct eem_det *det, int reason);
	void (*disable_locked)(struct eem_det *det, int reason);
	void (*switch_bank)(struct eem_det *det);

	int (*init01)(struct eem_det *det);
	int (*init02)(struct eem_det *det);
	int (*mon_mode)(struct eem_det *det);

	int (*get_status)(struct eem_det *det);
	void (*dump_status)(struct eem_det *det);

	void (*set_phase)(struct eem_det *det, enum eem_phase phase);

	/* interface to thermal */
	int (*get_temp)(struct eem_det *det);

	/* interface to DVFS */
	int (*get_volt)(struct eem_det *det);
	int (*set_volt)(struct eem_det *det);
	void (*restore_default_volt)(struct eem_det *det);
	void (*get_freq_table)(struct eem_det *det);

	/* interface to PMIC */
	int (*volt_2_pmic)(struct eem_det *det, int volt);
	int (*volt_2_eem)(struct eem_det *det, int volt);
	int (*pmic_2_volt)(struct eem_det *det, int pmic_val);
	int (*eem_2_pmic)(struct eem_det *det, int eev_val);
};

enum eem_features {
	FEA_INIT01	= BIT(EEM_PHASE_INIT01),
	FEA_INIT02	= BIT(EEM_PHASE_INIT02),
	FEA_MON		= BIT(EEM_PHASE_MON),
};

struct eem_det {
	const char *name;
	struct eem_det_ops *ops;
	int status;			/* TODO: enable/disable */
	int features;		/* enum eem_features */
	enum eem_ctrl_id ctrl_id;

	/* devinfo */
	unsigned int EEMINITEN;
	unsigned int EEMMONEN;
	unsigned int MDES;
	unsigned int BDES;
	unsigned int DCMDET;
	unsigned int DCBDET;
	unsigned int AGEDELTA;
	unsigned int MTDES;

	/* constant */
	unsigned int DETWINDOW;
	unsigned int VMAX;
	unsigned int VMIN;
	unsigned int DTHI;
	unsigned int DTLO;
	unsigned int VBOOT;
	unsigned int DETMAX;
	unsigned int AGECONFIG;
	unsigned int AGEM;
	unsigned int DVTFIXED;
	unsigned int VCO;
	unsigned int DCCONFIG;

	/* Generated by EEM init01. Used in EEM init02 */
	unsigned int DCVOFFSETIN;
	unsigned int AGEVOFFSETIN;

	/* for PMIC */
	unsigned int eem_v_base;
	unsigned int eem_step;
	unsigned int pmic_base;
	unsigned int pmic_step;

	/* for debug */
	unsigned int dcvalues[NR_EEM_PHASE];

	unsigned int freqpct30[NR_EEM_PHASE];
	unsigned int eem_26c[NR_EEM_PHASE];
	unsigned int vop30[NR_EEM_PHASE];
	unsigned int eem_eemEn[NR_EEM_PHASE];
	u32 *recordRef;
#if DUMP_DATA_TO_DE
	unsigned int reg_dump_data[ARRAY_SIZE(reg_dump_addr_off)][NR_EEM_PHASE];
#endif
	/* slope */
	unsigned int MTS;
	unsigned int BTS;
	unsigned int t250;
	/* dvfs */
	unsigned int num_freq_tbl; /* could be got @ the same time
				      with freq_tbl[] */
	unsigned int max_freq_khz; /* maximum frequency used to
				      calculate percentage */
	unsigned char freq_tbl[NR_FREQ]; /* percentage to maximum freq */

	unsigned int volt_tbl[NR_FREQ]; /* pmic value */
	unsigned int volt_tbl_init2[NR_FREQ]; /* pmic value */
	unsigned int volt_tbl_pmic[NR_FREQ]; /* pmic value */
	unsigned int volt_tbl_bin[NR_FREQ]; /* pmic value */
	int volt_offset;
	int pi_offset;

	unsigned int disabled; /* Disabled by error or sysfs */
};


struct eem_devinfo {
	/* M_HW_RES0 0x10206960 */
	unsigned int BIG_BDES:8;
	unsigned int BIG_MDES:8;
	unsigned int BIG_DCBDET:8;
	unsigned int BIG_DCMDET:8;

	/* M_HW_RES1 0x10206964 */
	unsigned int BIG_INITEN:1;
	unsigned int BIG_MONEN:1;
	unsigned int BIG_DVFS_L:2;
	unsigned int BIG_TURBO:1;
	unsigned int BIG_BIN_SPEC:3;
	unsigned int BIG_LEAKAGE:8;
	unsigned int BIG_MTDES:8;
	unsigned int BIG_AGEDELTA:8;

	/* M_HW_RES2 0x10206968 */
	unsigned int GPU_BDES:8;
	unsigned int GPU_MDES:8;
	unsigned int GPU_DCBDET:8;
	unsigned int GPU_DCMDET:8;

	/* M_HW_RES3 0x1020696C */
	unsigned int GPU_INITEN:1;
	unsigned int GPU_MONEN:1;
	unsigned int GPU_DVFS_L:2;
	unsigned int GPU_TURBO:1;
	unsigned int GPU_BIN_SPEC:3;
	unsigned int GPU_LEAKAGE:8;
	unsigned int GPU_MTDES:8;
	unsigned int GPU_AGEDELTA:8;

	/* M_HW_RES6 0x10206978 */
	unsigned int CPU_L_BDES:8;
	unsigned int CPU_L_MDES:8;
	unsigned int CPU_L_DCBDET:8;
	unsigned int CPU_L_DCMDET:8;

	/* M_HW_RES7 0x1020697C */
	unsigned int CPU_L_INITEN:1;
	unsigned int CPU_L_MONEN:1;
	unsigned int CPU_L_DVFS_L:2;
	unsigned int CPU_L_TURBO:1;
	unsigned int CPU_L_BIN_SPEC:3;
	unsigned int CPU_L_LEAKAGE:8; /* L & LL LEAKAGE */
	unsigned int CPU_L_MTDES:8;
	unsigned int CPU_L_AGEDELTA:8;

	/* M_HW_RES8 0x10206980 */
	unsigned int CPU_2L_BDES:8;
	unsigned int CPU_2L_MDES:8;
	unsigned int CPU_2L_DCBDET:8;
	unsigned int CPU_2L_DCMDET:8;

	/* M_HW_RES9 0x10206984 */
	unsigned int CPU_2L_INITEN:1;
	unsigned int CPU_2L_MONEN:1;
	unsigned int CPU_2L_DVFS_L:2;
	unsigned int CPU_2L_TURBO:1;
	unsigned int CPU_2L_BIN_SPEC:3;
	unsigned int CCI_INITEN:1;
	unsigned int CCI_MONEN:1;
	unsigned int CCI_DVFS_L:2;
	unsigned int CCI_TURBO:1;
	unsigned int CCI_BIN_SPEC:3;
	unsigned int CPU_2L_MTDES:8;
	unsigned int CPU_2L_AGEDELTA:8;

	/* M_HW_RES10 0x10206988 */
	unsigned int CCI_BDES:8;
	unsigned int CCI_MDES:8;
	unsigned int CCI_DCBDET:8;
	unsigned int CCI_DCMDET:8;

	/* M_HW_RES11 0x1020698C */
	unsigned int CCI_RESERVED:16;
	unsigned int CCI_MTDES:8;
	unsigned int CCI_AGEDELTA:8;

	/* M_HW_RES15 0x1020699C */
	unsigned int BIG_9214_MTDES:8;
	unsigned int BIG_9214_AGEDELTA:8;
	unsigned int BIG_9214_RESERVED:16;

	/* M_HW_RES16 0x102069A0 */
	unsigned int BIG_9214_BDES:8;
	unsigned int BIG_9214_MDES:8;
	unsigned int BIG_9214_DCBDET:8;
	unsigned int BIG_9214_DCMDET:8;
};

/*=============================================================
 *Local variable definition
 *=============================================================*/

#ifdef __KERNEL__
/*
 * lock
 */
static DEFINE_SPINLOCK(eem_spinlock);
static DEFINE_MUTEX(record_mutex);

/* CPU callback */
static int __cpuinit _mt_eem_cpu_CB(struct notifier_block *nfb,
					unsigned long action, void *hcpu);
static struct notifier_block __refdata _mt_eem_cpu_notifier = {
	.notifier_call = _mt_eem_cpu_CB,
};
#endif

/**
 * EEM controllers
 */
struct eem_ctrl eem_ctrls[NR_EEM_CTRL] = {
	[EEM_CTRL_BIG] = {
		.name = __stringify(EEM_CTRL_BIG),
		.det_id = EEM_DET_BIG,
	},

	[EEM_CTRL_GPU] = {
		.name = __stringify(EEM_CTRL_GPU),
		.det_id = EEM_DET_GPU,
	},

	[EEM_CTRL_SOC] = {
		.name = __stringify(EEM_CTRL_SOC),
		.det_id = EEM_DET_SOC,
	},

	[EEM_CTRL_L] = {
		.name = __stringify(EEM_CTRL_L),
		.det_id = EEM_DET_L,
	},

	[EEM_CTRL_2L] = {
		.name = __stringify(EEM_CTRL_2L),
		.det_id = EEM_DET_2L,
	},

	[EEM_CTRL_CCI] = {
		.name = __stringify(EEM_CTRL_CCI),
		.det_id = EEM_DET_CCI,
	}
};

/*
 * operation of EEM detectors
 */
static void base_ops_enable(struct eem_det *det, int reason);
static void base_ops_disable(struct eem_det *det, int reason);
static void base_ops_disable_locked(struct eem_det *det, int reason);
static void base_ops_switch_bank(struct eem_det *det);

static int base_ops_init01(struct eem_det *det);
static int base_ops_init02(struct eem_det *det);
static int base_ops_mon_mode(struct eem_det *det);

static int base_ops_get_status(struct eem_det *det);
static void base_ops_dump_status(struct eem_det *det);

static void base_ops_set_phase(struct eem_det *det, enum eem_phase phase);
static int base_ops_get_temp(struct eem_det *det);
static int base_ops_get_volt(struct eem_det *det);
static int base_ops_set_volt(struct eem_det *det);
static void base_ops_restore_default_volt(struct eem_det *det);
static void base_ops_get_freq_table(struct eem_det *det);
static int base_ops_volt_2_pmic(struct eem_det *det, int volt); /* PMIC interface */
static int base_ops_volt_2_eem(struct eem_det *det, int volt);
static int base_ops_pmic_2_volt(struct eem_det *det, int pmic_val);
static int base_ops_eem_2_pmic(struct eem_det *det, int eev_val);

static int get_volt_cpu(struct eem_det *det);
static int set_volt_cpu(struct eem_det *det);
static void restore_default_volt_cpu(struct eem_det *det);
static void get_freq_table_cpu(struct eem_det *det);

static int get_volt_gpu(struct eem_det *det);
static int set_volt_gpu(struct eem_det *det);
static void restore_default_volt_gpu(struct eem_det *det);
static void get_freq_table_gpu(struct eem_det *det);

#if 0
static int get_volt_lte(struct eem_det *det);
static int set_volt_lte(struct eem_det *det);
static void restore_default_volt_lte(struct eem_det *det);

static int get_volt_vcore(struct eem_det *det);
#ifndef __KERNEL__
static int set_volt_vcore(struct eem_det *det);
#endif

static void switch_to_vcore_ao(struct eem_det *det);
static void switch_to_vcore_pdn(struct eem_det *det);
#endif

#define BASE_OP(fn)	.fn = base_ops_ ## fn
static struct eem_det_ops eem_det_base_ops = {
	BASE_OP(enable),
	BASE_OP(disable),
	BASE_OP(disable_locked),
	BASE_OP(switch_bank),

	BASE_OP(init01),
	BASE_OP(init02),
	BASE_OP(mon_mode),

	BASE_OP(get_status),
	BASE_OP(dump_status),

	BASE_OP(set_phase),

	BASE_OP(get_temp),

	BASE_OP(get_volt),
	BASE_OP(set_volt),
	BASE_OP(restore_default_volt),
	BASE_OP(get_freq_table),

	BASE_OP(volt_2_pmic),
	BASE_OP(volt_2_eem),
	BASE_OP(pmic_2_volt),
	BASE_OP(eem_2_pmic),
};

static struct eem_det_ops big_det_ops = {
	.get_volt		= get_volt_cpu,
	.set_volt		= set_volt_cpu,
	.restore_default_volt	= restore_default_volt_cpu,
	.get_freq_table		= get_freq_table_cpu,
};

static struct eem_det_ops gpu_det_ops = {
	.get_volt		= get_volt_gpu,
	.set_volt		= set_volt_gpu,
	.restore_default_volt	= restore_default_volt_gpu,
	.get_freq_table		= get_freq_table_gpu,
};

static struct eem_det_ops soc_det_ops = {
	.get_volt		= NULL,
	.set_volt		= NULL,
	.restore_default_volt	= NULL,
	.get_freq_table		= NULL,
};

static struct eem_det_ops little_det_ops = {
	.get_volt		= get_volt_cpu,
	.set_volt		= set_volt_cpu,
	.restore_default_volt	= restore_default_volt_cpu,
	.get_freq_table		= get_freq_table_cpu,
};

static struct eem_det_ops dual_little_det_ops = {
	.get_volt		= get_volt_cpu,
	.set_volt		= set_volt_cpu,
	.restore_default_volt	= restore_default_volt_cpu,
	.get_freq_table		= get_freq_table_cpu,
};

static struct eem_det_ops cci_det_ops = {
	.get_volt		= get_volt_cpu,
	.set_volt		= set_volt_cpu,
	.restore_default_volt	= restore_default_volt_cpu,
	.get_freq_table		= get_freq_table_cpu,
};

#if 0
static struct eem_det_ops lte_det_ops = {
	.get_volt		= get_volt_lte,
	.set_volt		= set_volt_lte,
	.restore_default_volt	= restore_default_volt_lte,
	.get_freq_table		= NULL,
};
#endif

static struct eem_det eem_detectors[NR_EEM_DET] = {
	[EEM_DET_BIG] = {
		.name		= __stringify(EEM_DET_BIG),
		.ops		= &big_det_ops,
		.ctrl_id	= EEM_CTRL_BIG,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		.max_freq_khz	= 2496000,/* 2496Mhz */
		.VBOOT		= 100000, /* 10uV */
		.volt_offset	= 0,
		.eem_v_base	= BIG_EEM_BASE,
		.eem_step	= BIG_EEM_STEP,
		.pmic_base	= BIG_CPU_PMIC_BASE,
		.pmic_step	= BIG_CPU_PMIC_STEP,
	},

	[EEM_DET_GPU] = {
		.name		= __stringify(EEM_DET_GPU),
		.ops		= &gpu_det_ops,
		.ctrl_id	= EEM_CTRL_GPU,
		.features	= 0, /* FEA_INIT01 | FEA_INIT02 | FEA_MON, */
		.max_freq_khz	= 850000,/* 850 MHz */
		.VBOOT		= 100060, /* 1.0006V */
		.volt_offset	= 0,
		.eem_v_base	= EEM_V_BASE,
		.eem_step	= EEM_STEP,
		.pmic_base	= GPU_PMIC_BASE,
		.pmic_step	= GPU_PMIC_STEP,
	},

	[EEM_DET_SOC] = {
		.name		= __stringify(EEM_DET_SOC),
		.ops		= &soc_det_ops,
		.ctrl_id	= EEM_CTRL_SOC,
		.features	= 0,
		.max_freq_khz	= 800000,/* 800 MHz */
		.VBOOT		= 100000, /* 1.0v */
		.volt_offset	= 0,
		.eem_v_base	= EEM_V_BASE,
		.eem_step       = EEM_STEP,
		.pmic_base	= SRAM_BASE,
		.pmic_step	= SRAM_STEP,
	},

	[EEM_DET_L] = {
		.name		= __stringify(EEM_DET_L),
		.ops		= &little_det_ops,
		.ctrl_id	= EEM_CTRL_L,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		.max_freq_khz	= 2249000,/* 2249 MHz */
		.VBOOT		= 100000, /* 1.0v: 0x30 */
		.volt_offset	= 0,
		.eem_v_base	= EEM_V_BASE,
		.eem_step       = EEM_STEP,
		.pmic_base	= CPU_PMIC_BASE,
		.pmic_step	= CPU_PMIC_STEP,
	},

	[EEM_DET_2L] = {
		.name		= __stringify(EEM_DET_2L),
		.ops		= &dual_little_det_ops,
		.ctrl_id	= EEM_CTRL_2L,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		.max_freq_khz	= 1599000,/* 1599 MHz */
		.VBOOT		= 100000, /* 1.0v: 0x30 */
		.volt_offset	= 0,
		.eem_v_base	= EEM_V_BASE,
		.eem_step       = EEM_STEP,
		.pmic_base	= CPU_PMIC_BASE,
		.pmic_step	= CPU_PMIC_STEP,
	},

	[EEM_DET_CCI] = {
		.name		= __stringify(EEM_DET_CCI),
		.ops		= &cci_det_ops,
		.ctrl_id	= EEM_CTRL_CCI,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		.max_freq_khz	= 1118000,/* 1118 MHz */
		.VBOOT		= 100000, /* 1.0v: 0x30 */
		.volt_offset	= 0,
		.eem_v_base	= EEM_V_BASE,
		.eem_step       = EEM_STEP,
		.pmic_base	= CPU_PMIC_BASE,
		.pmic_step	= CPU_PMIC_STEP,
	},
};

static struct eem_devinfo eem_devinfo;

#ifdef __KERNEL__
/**
 * timer for log
 */
static struct hrtimer eem_log_timer;
#endif

/*=============================================================
 * Local function definition
 *=============================================================*/

static struct eem_det *id_to_eem_det(enum eem_det_id id)
{
	if (likely(id < NR_EEM_DET))
		return &eem_detectors[id];
	else
		return NULL;
}

static struct eem_ctrl *id_to_eem_ctrl(enum eem_ctrl_id id)
{
	if (likely(id < NR_EEM_CTRL))
		return &eem_ctrls[id];
	else
		return NULL;
}

static void base_ops_enable(struct eem_det *det, int reason)
{
	/* FIXME: UNDER CONSTRUCTION */
	FUNC_ENTER(FUNC_LV_HELP);
	det->disabled &= ~reason;
	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_switch_bank(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	eem_write_field(EEMCORESEL, 2:0 /* APBSEL */, det->ctrl_id);
	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_disable_locked(struct eem_det *det, int reason)
{
	FUNC_ENTER(FUNC_LV_HELP);

	switch (reason) {
	case BY_MON_ERROR:
		/* disable EEM */
		eem_write(EEMEN, 0x0);

		/* Clear EEM interrupt EEMINTSTS */
		eem_write(EEMINTSTS, 0x00ffffff);
		/* fall through */

	case BY_PROCFS_INIT2: /* 2 */
		/* set init2 value to DVFS table (PMIC) */
		memcpy(det->volt_tbl, det->volt_tbl_init2, sizeof(det->volt_tbl_init2));
		eem_set_eem_volt(det);
		det->disabled |= reason;
		break;

	case BY_INIT_ERROR:
		/* disable EEM */
		eem_write(EEMEN, 0x0);

		/* Clear EEM interrupt EEMINTSTS */
		eem_write(EEMINTSTS, 0x00ffffff);
		/* fall through */

	case BY_PROCFS: /* 1 */
		det->disabled |= reason;
		/* restore default DVFS table (PMIC) */
		eem_restore_eem_volt(det);
		break;

	default:
		det->disabled &= ~BY_PROCFS;
		det->disabled &= ~BY_PROCFS_INIT2;
		eem_set_eem_volt(det);
		break;
	}

	eem_debug("Disable EEM[%s] done. reason=[%d]\n", det->name, det->disabled);

	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_disable(struct eem_det *det, int reason)
{
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_HELP);

	mt_ptp_lock(&flags);
	det->ops->switch_bank(det);
	det->ops->disable_locked(det, reason);
	mt_ptp_unlock(&flags);

	FUNC_EXIT(FUNC_LV_HELP);
}

static int base_ops_init01(struct eem_det *det)
{
	/* struct eem_ctrl *ctrl = id_to_eem_ctrl(det->ctrl_id); */

	FUNC_ENTER(FUNC_LV_HELP);

	if (unlikely(!HAS_FEATURE(det, FEA_INIT01))) {
		eem_debug("det %s has no INIT01\n", det->name);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	/*
	if (det->disabled & BY_PROCFS) {
		eem_debug("[%s] Disabled by PROCFS\n", __func__);
		FUNC_EXIT(FUNC_LV_HELP);
		return -2;
	}
	*/

	/* atomic_inc(&ctrl->in_init); */
	/* det->ops->dump_status(det); */
	det->ops->set_phase(det, EEM_PHASE_INIT01);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_init02(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	if (unlikely(!HAS_FEATURE(det, FEA_INIT02))) {
		eem_debug("det %s has no INIT02\n", det->name);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	if (det->disabled & BY_INIT_ERROR) {
		eem_error("[%s] Disabled by INIT_ERROR\n", ((char *)(det->name) + 8));
		FUNC_EXIT(FUNC_LV_HELP);
		return -2;
	}
	eem_debug("DCV = 0x%08X, AGEV = 0x%08X\n", det->DCVOFFSETIN, det->AGEVOFFSETIN);

	/* det->ops->dump_status(det); */
	det->ops->set_phase(det, EEM_PHASE_INIT02);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_mon_mode(struct eem_det *det)
{
	#ifndef EARLY_PORTING
	struct TS_PTPOD ts_info;
	thermal_bank_name ts_bank;
	#endif

	FUNC_ENTER(FUNC_LV_HELP);

	if (!HAS_FEATURE(det, FEA_MON)) {
		eem_debug("det %s has no MON mode\n", det->name);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	if (det->disabled & BY_INIT_ERROR) {
		eem_error("[%s] Disabled BY_INIT_ERROR\n", ((char *)(det->name) + 8));
		FUNC_EXIT(FUNC_LV_HELP);
		return -2;
	}

#if !defined(EARLY_PORTING)
	ts_bank = det->ctrl_id;
	get_thermal_slope_intercept(&ts_info, ts_bank);
	det->MTS = ts_info.ts_MTS;
	det->BTS = ts_info.ts_BTS;
#else
	det->MTS =  0x2cf; /* (2048 * TS_SLOPE) + 2048; */
	det->BTS =  0x80E; /* 4 * TS_INTERCEPT; */
#endif
	eem_debug("Bk = %d, MTS = %d, BTS = %d\n", det->ctrl_id, det->MTS, det->BTS);
	/*
	if ((det->EEMINITEN == 0x0) || (det->EEMMONEN == 0x0)) {
		eem_error("EEMINITEN = 0x%08X, EEMMONEN = 0x%08X\n", det->EEMINITEN, det->EEMMONEN);
		FUNC_EXIT(FUNC_LV_HELP);
		return 1;
	}
	*/
	/* det->ops->dump_status(det); */
	det->ops->set_phase(det, EEM_PHASE_MON);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_get_status(struct eem_det *det)
{
	int status;
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_HELP);

	mt_ptp_lock(&flags);
	det->ops->switch_bank(det);
	status = (eem_read(EEMEN) != 0) ? 1 : 0;
	mt_ptp_unlock(&flags);

	FUNC_EXIT(FUNC_LV_HELP);

	return status;
}

static void base_ops_dump_status(struct eem_det *det)
{
	unsigned int i;

	FUNC_ENTER(FUNC_LV_HELP);

	eem_isr_info("[%s]\n",			det->name);

	eem_isr_info("EEMINITEN = 0x%08X\n",	det->EEMINITEN);
	eem_isr_info("EEMMONEN = 0x%08X\n",	det->EEMMONEN);
	eem_isr_info("MDES = 0x%08X\n",		det->MDES);
	eem_isr_info("BDES = 0x%08X\n",		det->BDES);
	eem_isr_info("DCMDET = 0x%08X\n",	det->DCMDET);

	eem_isr_info("DCCONFIG = 0x%08X\n",	det->DCCONFIG);
	eem_isr_info("DCBDET = 0x%08X\n",	det->DCBDET);

	eem_isr_info("AGECONFIG = 0x%08X\n",	det->AGECONFIG);
	eem_isr_info("AGEM = 0x%08X\n",		det->AGEM);

	eem_isr_info("AGEDELTA = 0x%08X\n",	det->AGEDELTA);
	eem_isr_info("DVTFIXED = 0x%08X\n",	det->DVTFIXED);
	eem_isr_info("MTDES = 0x%08X\n",	det->MTDES);
	eem_isr_info("VCO = 0x%08X\n",		det->VCO);

	eem_isr_info("DETWINDOW = 0x%08X\n",	det->DETWINDOW);
	eem_isr_info("VMAX = 0x%08X\n",		det->VMAX);
	eem_isr_info("VMIN = 0x%08X\n",		det->VMIN);
	eem_isr_info("DTHI = 0x%08X\n",		det->DTHI);
	eem_isr_info("DTLO = 0x%08X\n",		det->DTLO);
	eem_isr_info("VBOOT = 0x%08X\n",	det->VBOOT);
	eem_isr_info("DETMAX = 0x%08X\n",	det->DETMAX);

	eem_isr_info("DCVOFFSETIN = 0x%08X\n",	det->DCVOFFSETIN);
	eem_isr_info("AGEVOFFSETIN = 0x%08X\n",	det->AGEVOFFSETIN);

	eem_isr_info("MTS = 0x%08X\n",		det->MTS);
	eem_isr_info("BTS = 0x%08X\n",		det->BTS);

	eem_isr_info("num_freq_tbl = %d\n", det->num_freq_tbl);

	for (i = 0; i < det->num_freq_tbl; i++)
		eem_isr_info("freq_tbl[%d] = %d\n", i, det->freq_tbl[i]);

	for (i = 0; i < det->num_freq_tbl; i++)
		eem_isr_info("volt_tbl[%d] = %d\n", i, det->volt_tbl[i]);

	for (i = 0; i < det->num_freq_tbl; i++)
		eem_isr_info("volt_tbl_init2[%d] = %d\n", i, det->volt_tbl_init2[i]);

	for (i = 0; i < det->num_freq_tbl; i++)
		eem_isr_info("volt_tbl_pmic[%d] = %d\n", i, det->volt_tbl_pmic[i]);

	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_set_phase(struct eem_det *det, enum eem_phase phase)
{
	unsigned int i, filter, val;
	/* unsigned long flags; */

	FUNC_ENTER(FUNC_LV_HELP);

	/* mt_ptp_lock(&flags); */

	det->ops->switch_bank(det);
	/* config EEM register */
	eem_write(EEM_DESCHAR,
		  ((det->BDES << 8) & 0xff00) | (det->MDES & 0xff));
	eem_write(EEM_TEMPCHAR,
		  (((det->VCO << 16) & 0xff0000) |
		   ((det->MTDES << 8) & 0xff00) | (det->DVTFIXED & 0xff)));
	eem_write(EEM_DETCHAR,
		  ((det->DCBDET << 8) & 0xff00) | (det->DCMDET & 0xff));
	eem_write(EEM_AGECHAR,
		  ((det->AGEDELTA << 8) & 0xff00) | (det->AGEM & 0xff));
	eem_write(EEM_DCCONFIG, det->DCCONFIG);
	eem_write(EEM_AGECONFIG, det->AGECONFIG);

	if (EEM_PHASE_MON == phase)
		eem_write(EEM_TSCALCS,
			  ((det->BTS << 12) & 0xfff000) | (det->MTS & 0xfff));

	if (det->AGEM == 0x0)
		eem_write(EEM_RUNCONFIG, 0x80000000);
	else {
		val = 0x0;

		for (i = 0; i < 24; i += 2) {
			filter = 0x3 << i;

			if (((det->AGECONFIG) & filter) == 0x0)
				val |= (0x1 << i);
			else
				val |= ((det->AGECONFIG) & filter);
		}

		eem_write(EEM_RUNCONFIG, val);
	}

	eem_write(EEM_FREQPCT30,
		  ((det->freq_tbl[3 * (NR_FREQ / 8)] << 24) & 0xff000000)	|
		  ((det->freq_tbl[2 * (NR_FREQ / 8)] << 16) & 0xff0000)	|
		  ((det->freq_tbl[1 * (NR_FREQ / 8)] << 8) & 0xff00)	|
		  (det->freq_tbl[0] & 0xff));
	eem_write(EEM_FREQPCT74,
		  ((det->freq_tbl[7 * (NR_FREQ / 8)] << 24) & 0xff000000)	|
		  ((det->freq_tbl[6 * (NR_FREQ / 8)] << 16) & 0xff0000)	|
		  ((det->freq_tbl[5 * (NR_FREQ / 8)] << 8) & 0xff00)	|
		  ((det->freq_tbl[4 * (NR_FREQ / 8)]) & 0xff));
	eem_write(EEM_LIMITVALS,
		  ((det->VMAX << 24) & 0xff000000)	|
		  ((det->VMIN << 16) & 0xff0000)	|
		  ((det->DTHI << 8) & 0xff00)		|
		  (det->DTLO & 0xff));
	/* eem_write(EEM_LIMITVALS, 0xFF0001FE); */
	eem_write(EEM_VBOOT, (((det->VBOOT) & 0xff)));
	eem_write(EEM_DETWINDOW, (((det->DETWINDOW) & 0xffff)));
	eem_write(EEMCONFIG, (((det->DETMAX) & 0xffff)));

	/* clear all pending EEM interrupt & config EEMINTEN */
	eem_write(EEMINTSTS, 0xffffffff);

	/*fix DCBDET overflow issue */
	if (((det_to_id(det) == EEM_DET_GPU) && isGPUDCBDETOverflow) ||
		((det_to_id(det) == EEM_DET_2L) && is2LDCBDETOverflow) ||
		((det_to_id(det) == EEM_DET_L) && isLDCBDETOverflow) ||
		((det_to_id(det) == EEM_DET_CCI) && isCCIDCBDETOverflow)) {
		eem_write(EEM_CHKSHIFT, (eem_read(EEM_CHKSHIFT) & ~0x0F) | 0x07); /* 0x07 = DCBDETOFF */
	}

	eem_debug("%s phase = %d\n", ((char *)(det->name) + 8), phase);
	switch (phase) {
	case EEM_PHASE_INIT01:
		eem_write(EEMINTEN, 0x00005f01);
		/* enable EEM INIT measurement */
		eem_write(EEMEN, 0x00000001);
		break;

	case EEM_PHASE_INIT02:
		eem_write(EEMINTEN, 0x00005f01);
		eem_write(EEM_INIT2VALS,
			  ((det->AGEVOFFSETIN << 16) & 0xffff0000) |
			  (det->DCVOFFSETIN & 0xffff));
		/* enable EEM INIT measurement */
		eem_write(EEMEN, 0x00000005);
		break;

	case EEM_PHASE_MON:
		eem_write(EEMINTEN, 0x00FF0000);
		/* enable EEM monitor mode */
		eem_write(EEMEN, 0x00000002);
		break;

	default:
		BUG();
		break;
	}

	#if defined(__MTK_SLT_)
		mdelay(5);
	#endif
	/* mt_ptp_unlock(&flags); */

	FUNC_EXIT(FUNC_LV_HELP);
}

static int base_ops_get_temp(struct eem_det *det)
{
	thermal_bank_name ts_bank;
#if 1 /* TODO: FIXME */
	FUNC_ENTER(FUNC_LV_HELP);
	/* THERMAL_BANK0 = 0, */ /* BIG  */
	/* THERMAL_BANK1 = 1, */ /* GPU */
	/* THERMAL_BANK3 = 3, */ /* L */
	/* THERMAL_BANK3 = 4, */ /* 2L */
	/* THERMAL_BANK3 = 5, */ /* CCI */

	ts_bank = det_to_id(det);

	FUNC_EXIT(FUNC_LV_HELP);
#endif

#if !defined(EARLY_PORTING)
	#ifdef __KERNEL__
		return tscpu_get_temp_by_bank(ts_bank);
	#else
		thermal_init();
		udelay(1000);
		return mtktscpu_get_hw_temp();
	#endif
#else
	return 0;
#endif
}

static int base_ops_get_volt(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	eem_debug("[%s] default func\n", __func__);
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_set_volt(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	eem_debug("[%s] default func\n", __func__);
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static void base_ops_restore_default_volt(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	eem_debug("[%s] default func\n", __func__);
	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_get_freq_table(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	det->freq_tbl[0] = 100;
	det->num_freq_tbl = 1;

	FUNC_EXIT(FUNC_LV_HELP);
}

static int base_ops_volt_2_pmic(struct eem_det *det, int volt)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
	/*
	eem_debug("[%s][%s] volt = %d, pmic = %x\n",
		__func__,
		((char *)(det->name) + 8),
		volt,
		(((volt) - det->pmic_base + det->pmic_step - 1) / det->pmic_step)
		);
	*/
	return  (((volt) - det->pmic_base + det->pmic_step - 1) / det->pmic_step);
}

static int base_ops_volt_2_eem(struct eem_det *det, int volt)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
	/*
	eem_debug("[%s][%s] volt = %d, eem = %x\n",
		__func__,
		((char *)(det->name) + 8),
		volt,
		(((volt) - det->eem_v_base + det->eem_step - 1) / det->eem_step)
		);
	*/
	return (((volt) - det->eem_v_base + det->eem_step - 1) / det->eem_step);
}

static int base_ops_pmic_2_volt(struct eem_det *det, int pmic_val)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
	/*
	eem_debug("[%s][%s] pmic = %x, volt = %d\n",
		__func__,
		((char *)(det->name) + 8),
		pmic_val,
		(((pmic_val) * det->pmic_step) + det->pmic_base)
		);
	*/
	return (((pmic_val) * det->pmic_step) + det->pmic_base);
}

static int base_ops_eem_2_pmic(struct eem_det *det, int eem_val)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
	/*
	eem_debug("[%s][%s] eem = %x, pmic = %x\n",
		__func__,
		((char *)(det->name) + 8),
		eem_val,
		(((eem_val) * det->eem_step) + det->eem_v_base - det->pmic_base + det->pmic_step - 1) / det->pmic_step
		);
	*/
	return ((((eem_val) * det->eem_step) + det->eem_v_base - det->pmic_base + det->pmic_step - 1) / det->pmic_step);
}

#ifndef EARLY_PORTING
static void record(struct eem_det *det)
{
	int i;
	unsigned int vSram, vWrite;

	eem_debug("record() called !!\n");
	det->recordRef[NR_FREQ * 2] = 0x00000000;
	mb(); /* SRAM writing */
	for (i = 0; i < NR_FREQ; i++) {
		vSram = clamp(
				(unsigned int)(record_tbl_locked[i] + 0xA),
				(unsigned int)(det->ops->volt_2_pmic(det, VMIN_SRAM)),
				(unsigned int)(det->ops->volt_2_pmic(det, VMAX_SRAM)));

		vWrite = det->recordRef[i*2];
		vWrite = (vWrite & (~0x3FFF)) |
			((((PMIC_2_RMIC(det, vSram) & 0x7F) << 7) | (record_tbl_locked[i] & 0x7F)) & 0x3fff);

		det->recordRef[i*2] = vWrite;
	}
	det->recordRef[NR_FREQ * 2] = 0xFFFFFFFF;
	mb(); /* SRAM writing */
}

static void restore_record(struct eem_det *det)
{
	int i;
	unsigned int vWrite;

	eem_debug("restore_record() called !!\n");
	det->recordRef[NR_FREQ * 2] = 0x00000000;
	mb(); /* SRAM writing */
	for (i = 0; i < NR_FREQ; i++) {
		vWrite = det->recordRef[i*2];
		if (det_to_id(det) == EEM_DET_2L)
			vWrite = (vWrite & (~0x3FFF)) |
				(((*(recordTbl + (i * 8) + 6) & 0x7F) << 7) |
				(*(recordTbl + (i * 8) + 7) & 0x7F));
		else if (det_to_id(det) == EEM_DET_L)
			vWrite = (vWrite & (~0x3FFF)) |
				(((*(recordTbl + (i + 16) * 8 + 6) & 0x7F) << 7) |
				(*(recordTbl + (i + 16) * 8 + 7) & 0x7F));
		else if (det_to_id(det) == EEM_DET_CCI)
			vWrite = (vWrite & (~0x3FFF)) |
				(((*(recordTbl + (i + 32) * 8 + 6) & 0x7F) << 7) |
				(*(recordTbl + (i + 32) * 8 + 7) & 0x7F));
		else if (det_to_id(det) == EEM_DET_BIG)
			vWrite = (vWrite & (~0x3FFF)) |
				(((*(recordTbl + (i + 48) * 8 + 6) & 0x7F) << 7) |
				(*(recordTbl + (i + 48) * 8 + 7) & 0x7F));
		det->recordRef[i*2] = vWrite;
	}
	det->recordRef[NR_FREQ * 2] = 0xFFFFFFFF;
	mb(); /* SRAM writing */
}
#endif

/* Will return 10uV */
static int get_volt_cpu(struct eem_det *det)
{
	int value = 0;

	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);

	/* unit mv * 100 = 10uv */  /* I-Chang */
	#ifdef EARLY_PORTING
		return value;
	#else
		switch (det_to_id(det)) {
		case EEM_DET_BIG:
			value = mt_cpufreq_get_cur_volt(MT_CPU_DVFS_B);
			break;

		case EEM_DET_L:
			value = mt_cpufreq_get_cur_volt(MT_CPU_DVFS_L);
			break;

		case EEM_DET_2L:
			value = mt_cpufreq_get_cur_volt(MT_CPU_DVFS_LL);
			break;

		case EEM_DET_CCI:
			value = mt_cpufreq_get_cur_volt(MT_CPU_DVFS_CCI);
			break;

		default:
			value = 0;
			break;
		}

		return value;
	#endif
}

static void mt_cpufreq_set_ptbl_funcEEM(enum mt_cpu_dvfs_id id, int restore)
{
	struct eem_det *det = NULL;

	switch (id) {
	case MT_CPU_DVFS_B:
		det = id_to_eem_det(EEM_DET_BIG);
		break;

	case MT_CPU_DVFS_L:
		det = id_to_eem_det(EEM_DET_L);
		break;

	case MT_CPU_DVFS_LL:
		det = id_to_eem_det(EEM_DET_2L);
		break;

	case MT_CPU_DVFS_CCI:
		det = id_to_eem_det(EEM_DET_CCI);
		break;

	default:
		break;
	}

	if (det) {
		if (restore)
			restore_record(det);
		else
			record(det);
	}
}

/* volt_tbl_pmic is convert from 10uV */
static int set_volt_cpu(struct eem_det *det)
{
	int value = 0;

	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);

	eem_debug("init02_vop_30 = 0x%x\n", det->vop30[EEM_PHASE_INIT02]);

	#ifdef EARLY_PORTING
		return value;
	#else
		mutex_lock(&record_mutex);
		for (value = 0; value < NR_FREQ; value++)
			record_tbl_locked[value] = det->volt_tbl_pmic[value];

		switch (det_to_id(det)) {
		case EEM_DET_BIG:
			value = mt_cpufreq_update_volt_b(MT_CPU_DVFS_B, record_tbl_locked, det->num_freq_tbl);
			break;

		case EEM_DET_L:
			value = mt_cpufreq_update_volt(MT_CPU_DVFS_L, record_tbl_locked, det->num_freq_tbl);
			break;

		case EEM_DET_2L:
			value = mt_cpufreq_update_volt(MT_CPU_DVFS_LL, record_tbl_locked, det->num_freq_tbl);
			break;

		case EEM_DET_CCI:
			value = mt_cpufreq_update_volt(MT_CPU_DVFS_CCI, record_tbl_locked, det->num_freq_tbl);
			break;

		default:
			value = 0;
			break;
		}
		mutex_unlock(&record_mutex);
		return value;
	#endif
}

static void restore_default_volt_cpu(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	#ifndef EARLY_PORTING
		/* I-Chang */
		switch (det_to_id(det)) {
		case EEM_DET_BIG:
			mt_cpufreq_restore_default_volt(MT_CPU_DVFS_B);
			break;

		case EEM_DET_L:
			mt_cpufreq_restore_default_volt(MT_CPU_DVFS_L);
			break;

		case EEM_DET_2L:
			mt_cpufreq_restore_default_volt(MT_CPU_DVFS_LL);
			break;

		case EEM_DET_CCI:
			mt_cpufreq_restore_default_volt(MT_CPU_DVFS_CCI);
			break;
		}
	#endif

	FUNC_EXIT(FUNC_LV_HELP);
}

static void get_freq_table_cpu(struct eem_det *det)
{
	int i;
	unsigned int binLevel;

	/* Frequency gethering stopped from DVFS
	enum mt_cpu_dvfs_id cpu;

	cpu = (det_to_id(det) == EEM_DET_L) ?
		MT_CPU_DVFS_LITTLE : MT_CPU_DVFS_BIG;

	det->max_freq_khz = mt_cpufreq_get_freq_by_idx(cpu, 0);
	*/

	FUNC_ENTER(FUNC_LV_HELP);
	for (i = 0; i < NR_FREQ; i++) {
		/* det->freq_tbl[i] = PERCENT(mt_cpufreq_get_freq_by_idx(cpu, i), det->max_freq_khz); */
		#ifdef __KERNEL__
			binLevel = GET_BITS_VAL(3:0, get_devinfo_with_index(22));
			if (cpu_speed == 1989) /* M */
				binLevel = 2;
		#else
			binLevel = GET_BITS_VAL(3:0, eem_read(0x1020671C));
		#endif
		if (binLevel == 0) {
			det->freq_tbl[i] =
				PERCENT((det_to_id(det) == EEM_DET_BIG) ? bigFreq_FY[i] :
					(det_to_id(det) == EEM_DET_L) ? L_Freq_FY[i] :
					(det_to_id(det) == EEM_DET_2L) ? LL_Freq_FY[i] :
					cciFreq_FY[i]
					,
					det->max_freq_khz);
		} else if (binLevel == 1) {
			det->freq_tbl[i] =
				PERCENT((det_to_id(det) == EEM_DET_BIG) ? bigFreq_SB[i] :
					(det_to_id(det) == EEM_DET_L) ? L_Freq_SB[i] :
					(det_to_id(det) == EEM_DET_2L) ? LL_Freq_SB[i] :
					cciFreq_SB[i]
					,
					det->max_freq_khz);
		} else {
				det->freq_tbl[i] =
				PERCENT((det_to_id(det) == EEM_DET_BIG) ? bigFreq_MB[i] :
					(det_to_id(det) == EEM_DET_L) ? L_Freq_MB[i] :
					(det_to_id(det) == EEM_DET_2L) ? LL_Freq_MB[i] :
					cciFreq_MB[i]
					,
					det->max_freq_khz);
		}
		/* eem_debug("Timer Bank = %d, freq_procent = (%d)\n", det->ctrl_id, det->freq_tbl[i]); */
		if (0 == det->freq_tbl[i])
			break;
	}

	det->num_freq_tbl = i;

	FUNC_EXIT(FUNC_LV_HELP);
}

static int get_volt_gpu(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);

	/* eem_debug("get_volt_gpu=%d\n",mt_gpufreq_get_cur_volt()); */
	#ifdef EARLY_PORTING
	return 0;
	#else
		#if defined(__MTK_SLT_)
			return gpu_dvfs_get_cur_volt();
		#else
			return mt_gpufreq_get_cur_volt(); /* unit  mv * 100 = 10uv */
		#endif
	#endif
}

static int set_volt_gpu(struct eem_det *det)
{
	int i;

	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);

	/*
	eem_error("set_volt_gpu= ");
	for (i = 0; i < 15; i++)
		eem_error("%x(%d), ", det->volt_tbl_pmic[i], det->ops->pmic_2_volt(det, det->volt_tbl_pmic[i]));
	eem_error("%x\n", det->volt_tbl_pmic[i]);
	*/
	for (i = 0; i < 8; i++)
		gpuOutput[i] = det->volt_tbl_pmic[i * 2];

	#ifdef EARLY_PORTING
		return 0;
	#else
		return mt_gpufreq_update_volt(gpuOutput, 8);
	#endif
}

static void restore_default_volt_gpu(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	#if !defined(EARLY_PORTING)
	mt_gpufreq_restore_default_volt();
	#endif
	FUNC_EXIT(FUNC_LV_HELP);
}

static void get_freq_table_gpu(struct eem_det *det)
{
	int i;
	unsigned int binLevel;

	FUNC_ENTER(FUNC_LV_HELP);

	for (i = 0; i < NR_FREQ; i++) {
		/* det->freq_tbl[i] = PERCENT(mt_gpufreq_get_freq_by_idx(i/2), det->max_freq_khz); */
		#ifdef __KERNEL__
			binLevel = GET_BITS_VAL(3:0, get_devinfo_with_index(22));
		#else
			binLevel = GET_BITS_VAL(3:0, eem_read(0x1020671C));
		#endif
		if (binLevel == 0)
			det->freq_tbl[i] = PERCENT(gpuFreq_FY[i], det->max_freq_khz);
		else if (binLevel == 1)
			det->freq_tbl[i] = PERCENT(gpuFreq_SB[i], det->max_freq_khz);
		else
			det->freq_tbl[i] = PERCENT(gpuFreq_MB[i], det->max_freq_khz);
		if (0 == det->freq_tbl[i])
			break;
	}

	FUNC_EXIT(FUNC_LV_HELP);

	det->num_freq_tbl = i;
}

#if 0
static int set_volt_lte(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
#ifdef __KERNEL__
	return mt_cpufreq_set_lte_volt(det->volt_tbl_init2[0]);
#else
	return 0;
	/* return dvfs_set_vlte(det->volt_tbl_bin[0]); */
	/* return dvfs_set_vlte(det->volt_tbl_init2[0]); */
#endif
	FUNC_EXIT(FUNC_LV_HELP);
}

static int get_volt_lte(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	return mt_get_cur_volt_lte(); /* unit mv * 100 = 10uv */
	FUNC_EXIT(FUNC_LV_HELP);
}

static void restore_default_volt_lte(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
#ifdef __KERNEL__
	if (0x337 == mt_get_chip_hw_code())
		mt_cpufreq_set_lte_volt(det->ops->volt_2_pmic(det, 105000));
#else
	/* dvfs_set_vlte(det->ops->volt_2_pmic(det, 105000)); */
#endif
	FUNC_EXIT(FUNC_LV_HELP);
}


static void switch_to_vcore_ao(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	eem_write_field(PERI_VCORE_PTPOD_CON0, VCORE_PTPODSEL, SEL_VCORE_AO);
	eem_write_field(EEMCORESEL, APBSEL, det->ctrl_id);
	eem_ctrls[PTP_CTRL_VCORE].det_id = det_to_id(det);

	FUNC_EXIT(FUNC_LV_HELP);
}

static void switch_to_vcore_pdn(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	eem_write_field(PERI_VCORE_PTPOD_CON0, VCORE_PTPODSEL, SEL_VCORE_PDN);
	eem_write_field(EEMCORESEL, APBSEL, det->ctrl_id);
	eem_ctrls[EEM_CTRL_SOC].det_id = det_to_id(det);

	FUNC_EXIT(FUNC_LV_HELP);
}

#ifndef __KERNEL__
static int set_volt_vcore(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
	#if defined(__MTK_SLT_)
		#if defined(SLT_VMAX)
			det->volt_tbl_init2[0] = det->ops->volt_2_pmic(det, 131000);
			eem_debug("HV VCORE voltage EEM to 0x%X\n", det->volt_tbl_init2[0]);
		#elif defined(SLT_VMIN)
			det->volt_tbl_init2[0] = clamp(det->volt_tbl_init2[0] - 0xB,
				det->ops->eem_2_pmic(det, det->VMIN), det->ops->eem_2_pmic(det, det->VMAX));
			eem_debug("LV VCORE voltage EEM to 0x%X\n", det->volt_tbl_init2[0]);
		#else
			eem_debug("NV VCORE voltage EEM to 0x%x\n", det->volt_tbl_init2[0]);
		#endif
	#else
		eem_debug("VCORE EEM voltage to 0x%x\n", det->volt_tbl_init2[0]);
	#endif

	/* return mt_set_cur_volt_vcore_pdn(det->volt_tbl_pmic[0]); */  /* unit = 10 uv */
	#ifdef EARLY_PORTING
		return 0;
	#else
		return dvfs_set_vcore(det->volt_tbl_init2[0]);
	#endif
}
#endif

static int get_volt_vcore(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);

	/* eem_debug("get_volt_vcore=%d\n",mt_get_cur_volt_vcore_ao()); */
	#ifdef EARLY_PORTING
	return 0;
	#else
	return mt_get_cur_volt_vcore_ao(); /* unit = 10 uv */
	#endif
}
#endif

/*=============================================================
 * Global function definition
 *=============================================================*/
void mt_ptp_lock(unsigned long *flags)
{
	/* FUNC_ENTER(FUNC_LV_HELP); */
	/* FIXME: lock with MD32 */
	/* get_md32_semaphore(SEMAPHORE_PTP); */
#ifdef __KERNEL__
	spin_lock_irqsave(&eem_spinlock, *flags);
#endif
	/* FUNC_EXIT(FUNC_LV_HELP); */
}
#ifdef __KERNEL__
EXPORT_SYMBOL(mt_ptp_lock);
#endif

void mt_ptp_unlock(unsigned long *flags)
{
	/* FUNC_ENTER(FUNC_LV_HELP); */
#ifdef __KERNEL__
	spin_unlock_irqrestore(&eem_spinlock, *flags);
	/* FIXME: lock with MD32 */
	/* release_md32_semaphore(SEMAPHORE_PTP); */
#endif
	/* FUNC_EXIT(FUNC_LV_HELP); */
}
#ifdef __KERNEL__
EXPORT_SYMBOL(mt_ptp_unlock);
#endif

#if 0
int mt_ptp_idle_can_enter(void)
{
	struct eem_ctrl *ctrl;

	FUNC_ENTER(FUNC_LV_HELP);

	for_each_ctrl(ctrl) {
		if (atomic_read(&ctrl->in_init)) {
			FUNC_EXIT(FUNC_LV_HELP);
			return 0;
		}
	}

	FUNC_EXIT(FUNC_LV_HELP);

	return 1;
}
EXPORT_SYMBOL(mt_ptp_idle_can_enter);
#endif

#ifdef __KERNEL__
/*
 * timer for log
 */
static enum hrtimer_restart eem_log_timer_func(struct hrtimer *timer)
{
	int i;
	struct eem_det *det;

	FUNC_ENTER(FUNC_LV_HELP);

	for_each_det(det) {
		if (EEM_CTRL_SOC ==  det->ctrl_id)
			continue;

		eem_error("Timer Bank = %d (%d) - (", det->ctrl_id, det->ops->get_temp(det));
		for (i = 0; i < det->num_freq_tbl - 1; i++)
			eem_error("%d, ", det->ops->pmic_2_volt(det, det->volt_tbl_pmic[i]));
		eem_error("%d) - (0x%x), sts(%d), It(%d)\n",
			det->ops->pmic_2_volt(det, det->volt_tbl_pmic[i]),
			det->t250,
			det->ops->get_status(det),
			ITurboRun);
		/*
		det->freq_tbl[0],
		det->freq_tbl[1],
		det->freq_tbl[2],
		det->freq_tbl[3],
		det->freq_tbl[4],
		det->freq_tbl[5],
		det->freq_tbl[6],
		det->freq_tbl[7],
		det->dcvalues[3],
		det->freqpct30[3],
		det->eem_26c[3],
		det->vop30[3]
		*/
	}

	hrtimer_forward_now(timer, ns_to_ktime(LOG_INTERVAL));
	FUNC_EXIT(FUNC_LV_HELP);

	return HRTIMER_RESTART;
}
#endif
/*
 * Thread for voltage setting
 */
static int eem_volt_thread_handler(void *data)
{
	struct eem_ctrl *ctrl = (struct eem_ctrl *)data;
	struct eem_det *det = id_to_eem_det(ctrl->det_id);

	FUNC_ENTER(FUNC_LV_HELP);
	#ifdef __KERNEL__
	do {
		wait_event_interruptible(ctrl->wq, ctrl->volt_update);

		if ((ctrl->volt_update & EEM_VOLT_UPDATE) && det->ops->set_volt) {
		#if (defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING))
			int temp = -1;

			switch (det->ctrl_id) {
			case EEM_CTRL_BIG:
				aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() |
					(1 << EEM_CPU_BIG_IS_SET_VOLT));
				temp = EEM_CPU_BIG_IS_SET_VOLT;
				break;

			case EEM_CTRL_GPU:
				aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() |
					(1 << EEM_GPU_IS_SET_VOLT));
				temp = EEM_GPU_IS_SET_VOLT;
				break;

			case EEM_CTRL_L:
				aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() |
					(1 << EEM_CPU_LITTLE_IS_SET_VOLT));
				temp = EEM_CPU_LITTLE_IS_SET_VOLT;
				break;

			case EEM_CTRL_2L:
				aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() |
					(1 << EEM_CPU_2_LITTLE_IS_SET_VOLT));
				temp = EEM_CPU_2_LITTLE_IS_SET_VOLT;
				break;

			case EEM_CTRL_CCI:
				aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() |
					(1 << EEM_CPU_CCI_IS_SET_VOLT));
				temp = EEM_CPU_CCI_IS_SET_VOLT;
				break;

			default:
				break;
			}
		#endif
		det->ops->set_volt(det);

		#if (defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING))
			if (temp >= EEM_CPU_BIG_IS_SET_VOLT)
				aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() & ~(1 << temp));
		#endif
		}
		if ((ctrl->volt_update & EEM_VOLT_RESTORE) && det->ops->restore_default_volt)
			det->ops->restore_default_volt(det);

		ctrl->volt_update = EEM_VOLT_NONE;

	} while (!kthread_should_stop());
	#endif
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static void inherit_base_det(struct eem_det *det)
{
	/*
	 * Inherit ops from eem_det_base_ops if ops in det is NULL
	 */
	FUNC_ENTER(FUNC_LV_HELP);

	#define INIT_OP(ops, func)					\
		do {							\
			if (ops->func == NULL)				\
				ops->func = eem_det_base_ops.func;	\
		} while (0)

	INIT_OP(det->ops, disable);
	INIT_OP(det->ops, disable_locked);
	INIT_OP(det->ops, switch_bank);
	INIT_OP(det->ops, init01);
	INIT_OP(det->ops, init02);
	INIT_OP(det->ops, mon_mode);
	INIT_OP(det->ops, get_status);
	INIT_OP(det->ops, dump_status);
	INIT_OP(det->ops, set_phase);
	INIT_OP(det->ops, get_temp);
	INIT_OP(det->ops, get_volt);
	INIT_OP(det->ops, set_volt);
	INIT_OP(det->ops, restore_default_volt);
	INIT_OP(det->ops, get_freq_table);
	INIT_OP(det->ops, volt_2_pmic);
	INIT_OP(det->ops, volt_2_eem);
	INIT_OP(det->ops, pmic_2_volt);
	INIT_OP(det->ops, eem_2_pmic);

	FUNC_EXIT(FUNC_LV_HELP);
}

static void eem_init_ctrl(struct eem_ctrl *ctrl)
{
	FUNC_ENTER(FUNC_LV_HELP);
	/* init_completion(&ctrl->init_done); */
	/* atomic_set(&ctrl->in_init, 0); */
#ifdef __KERNEL__
	/* if(HAS_FEATURE(id_to_eem_det(ctrl->det_id), FEA_MON)) { */
	/* TODO: FIXME, why doesn't work <-XXX */
	if (1) {
		init_waitqueue_head(&ctrl->wq);
		ctrl->thread = kthread_run(eem_volt_thread_handler, ctrl, ctrl->name);

		if (IS_ERR(ctrl->thread))
			eem_error("Create %s thread failed: %ld\n", ctrl->name, PTR_ERR(ctrl->thread));
	}
#endif
	FUNC_EXIT(FUNC_LV_HELP);
}

static void eem_init_det(struct eem_det *det, struct eem_devinfo *devinfo)
{
	enum eem_det_id det_id = det_to_id(det);
	unsigned int binLevel, ateVer;

	FUNC_ENTER(FUNC_LV_HELP);
	eem_debug("det=%s, id=%d\n", ((char *)(det->name) + 8), det_id);

	inherit_base_det(det);

	/* init with devinfo, init with constant */
	det->DETWINDOW = DETWINDOW_VAL;

	det->DTHI = DTHI_VAL;
	det->DTLO = DTLO_VAL;
	det->DETMAX = DETMAX_VAL;

	det->AGECONFIG = AGECONFIG_VAL;
	det->AGEM = AGEM_VAL;
	det->DVTFIXED = DVTFIXED_VAL;

	#ifdef __KERNEL__
		ateVer = GET_BITS_VAL(7:4, get_devinfo_with_index(61));
	#else
		ateVer = GET_BITS_VAL(7:4, eem_read(0x1020698C));
	#endif
	if (ateVer > 5)
		det->VCO = VCO_VAL_AFTER_5;
	else
		det->VCO = VCO_VAL;

	det->DCCONFIG = DCCONFIG_VAL;

	#ifdef __KERNEL__
		binLevel = GET_BITS_VAL(3:0, get_devinfo_with_index(22));
	#else
		binLevel = GET_BITS_VAL(3:0, eem_read(0x1020671C));
	#endif
	if (binLevel == 0)
		det->VMIN       = det->ops->volt_2_eem(det, VMIN_VAL_LITTLE);
	else if (binLevel == 1)
		det->VMIN       = det->ops->volt_2_eem(det, VMIN_VAL_LITTLE_SB);
	else
		det->VMIN       = det->ops->volt_2_eem(det, VMIN_VAL_LITTLE);
	/*
	if (NULL != det->ops->get_volt) {
		det->VBOOT = det->ops->get_volt(det);
		eem_debug("@%s, %s-VBOOT = %d\n",
			__func__, ((char *)(det->name) + 8), det->VBOOT);
	}
	*/

	switch (det_id) {
	case EEM_DET_BIG:
		det->MDES	= devinfo->BIG_9214_MDES; /* devinfo->BIG_MDES; for MT6313  */
		det->BDES	= devinfo->BIG_9214_BDES; /* devinfo->BIG_BDES; for MT6313 */
		det->DCMDET	= devinfo->BIG_9214_DCMDET; /* devinfo->BIG_DCMDET; for MT6313 */
		det->DCBDET	= devinfo->BIG_9214_DCBDET; /* devinfo->BIG_DCBDET; for MT6313 */
		det->EEMINITEN	= devinfo->BIG_INITEN;
		det->EEMMONEN	= devinfo->BIG_MONEN;
		det->VBOOT	= det->ops->volt_2_pmic(det, det->VBOOT);
		det->VMAX	= det->ops->volt_2_pmic(det, VMAX_VAL_BIG);
		det->VMIN	= det->ops->volt_2_pmic(det, VMIN_VAL_BIG);
		det->DVTFIXED	= DVTFIXED_VAL_BIG;
		if (ateVer > 5)
			det->VCO = VCO_VAL_AFTER_5_BIG;
		else
			det->VCO = VCO_VAL_BIG;
		det->recordRef	= recordRef + 108;
		#if 0
			int i;

			for (int i = 0; i < NR_FREQ; i++)
				eem_debug("@(SRAM)%s----->(%s), = 0x%08x\n",
						__func__,
						det->name,
						*(det->recordRef + (i * 2)));
		#endif
		break;

	case EEM_DET_GPU:
		det->MDES	= devinfo->GPU_MDES;
		det->BDES	= devinfo->GPU_BDES;
		det->DCMDET	= devinfo->GPU_DCMDET;
		det->DCBDET	= devinfo->GPU_DCBDET;
		det->EEMINITEN	= devinfo->GPU_INITEN;
		det->EEMMONEN	= devinfo->GPU_MONEN;
		/* override default setting */
		det->VBOOT	= det->ops->volt_2_eem(det, det->VBOOT);
		det->VMAX	= det->ops->volt_2_eem(det, VMAX_VAL_GPU);
		det->VMIN	= det->ops->volt_2_eem(det, VMIN_VAL_GPU);
		det->DVTFIXED   = DVTFIXED_VAL_GPU;
		det->VCO	= VCO_VAL_GPU;
		break;

	case EEM_DET_L:
		det->MDES	= devinfo->CPU_L_MDES;
		det->BDES	= devinfo->CPU_L_BDES;
		det->DCMDET	= devinfo->CPU_L_DCMDET;
		det->DCBDET	= devinfo->CPU_L_DCBDET;
		det->EEMINITEN	= devinfo->CPU_L_INITEN;
		det->EEMMONEN	= devinfo->CPU_L_MONEN;
		det->VBOOT      = det->ops->volt_2_eem(det, det->VBOOT);
		det->VMAX	= det->ops->volt_2_eem(det, VMAX_VAL_LITTLE);
		det->recordRef	= recordRef + 36;
		#if 0
			int i;

			for (int i = 0; i < NR_FREQ; i++)
				eem_debug("@(Record)%s----->(%s), = 0x%08x\n",
						__func__,
						det->name,
						*(det->recordRef + (i * 2)));
		#endif
		break;

	case EEM_DET_2L:
		det->MDES	= devinfo->CPU_2L_MDES;
		det->BDES	= devinfo->CPU_2L_BDES;
		det->DCMDET	= devinfo->CPU_2L_DCMDET;
		det->DCBDET	= devinfo->CPU_2L_DCBDET;
		det->EEMINITEN	= devinfo->CPU_2L_INITEN;
		det->EEMMONEN	= devinfo->CPU_2L_MONEN;
		det->VBOOT      = det->ops->volt_2_eem(det, det->VBOOT);
		det->VMAX	= det->ops->volt_2_eem(det, VMAX_VAL_LITTLE);
		det->recordRef	= recordRef;
		#if 0
			int i;

			for (int i = 0; i < NR_FREQ; i++)
				eem_debug("@(Record)%s----->(%s), = 0x%08x\n",
						__func__,
						det->name,
						*(det->recordRef + (i * 2)));
		#endif
		break;

	case EEM_DET_CCI:
		det->MDES	= devinfo->CCI_MDES;
		det->BDES	= devinfo->CCI_BDES;
		det->DCMDET	= devinfo->CCI_DCMDET;
		det->DCBDET	= devinfo->CCI_DCBDET;
		det->EEMINITEN	= devinfo->CCI_INITEN;
		det->EEMMONEN	= devinfo->CCI_MONEN;
		det->VBOOT      = det->ops->volt_2_eem(det, det->VBOOT);
		det->VMAX	= det->ops->volt_2_eem(det, VMAX_VAL_LITTLE);
		det->recordRef	= recordRef + 72;
		#if 0
			int i;

			for (int i = 0; i < NR_FREQ; i++)
				eem_debug("@(Record)%s----->(%s), = 0x%08x\n",
						__func__,
						det->name,
						*(det->recordRef + (i * 2)));
		#endif
		break;

	default:
		eem_debug("[%s]: Unknown det_id %d\n", __func__, det_id);
		break;
	}

	switch (det->ctrl_id) {
	case EEM_CTRL_BIG:
		det->AGEDELTA	= devinfo->BIG_9214_AGEDELTA; /* devinfo->BIG_AGEDELTA; for MT6313 */
		det->MTDES	= devinfo->BIG_9214_MTDES; /* devinfo->BIG_MTDES; for MT6313 */
		break;

	case EEM_CTRL_GPU:
		det->AGEDELTA	= devinfo->GPU_AGEDELTA;
		det->MTDES	= devinfo->GPU_MTDES;
		break;

	case EEM_CTRL_L:
		det->AGEDELTA	= devinfo->CPU_L_AGEDELTA;
		det->MTDES	= devinfo->CPU_L_MTDES;
		break;

	case EEM_CTRL_2L:
		det->AGEDELTA	= devinfo->CPU_2L_AGEDELTA;
		det->MTDES	= devinfo->CPU_2L_MTDES;
		break;

	case EEM_CTRL_CCI:
		det->AGEDELTA	= devinfo->CCI_AGEDELTA;
		det->MTDES	= devinfo->CCI_MTDES;
		break;

	default:
		eem_debug("[%s]: Unknown ctrl_id %d\n", __func__, det->ctrl_id);
		break;
	}

	/* get DVFS frequency table */
	det->ops->get_freq_table(det);

	FUNC_EXIT(FUNC_LV_HELP);
}

#if defined(__MTK_SLT_) && (defined(SLT_VMAX) || defined(SLT_VMIN))
	#define EEM_PMIC_LV_HV_OFFSET (0x3)
	#define EEM_PMIC_NV_OFFSET (0xB)
#endif
static void eem_set_eem_volt(struct eem_det *det)
{
#if SET_PMIC_VOLT
	unsigned i;
	int cur_temp, low_temp_offset;
	struct eem_ctrl *ctrl = id_to_eem_ctrl(det->ctrl_id);

	cur_temp = det->ops->get_temp(det);
	/* eem_debug("eem_set_eem_volt cur_temp = %d\n", cur_temp); */
	/* 6250 * 10uV = 62.5mv */
	if (cur_temp <= 33000) {
		if (EEM_CTRL_BIG == det->ctrl_id)
			low_temp_offset = det->ops->volt_2_pmic(det, 6250 + det->pmic_base);
		else
			low_temp_offset = det->ops->volt_2_eem(det, 6250 + det->eem_v_base);

		ctrl->volt_update |= EEM_VOLT_UPDATE;
	} else {
		low_temp_offset = 0;
		ctrl->volt_update |= EEM_VOLT_UPDATE;
	}
	/* eem_debug("ctrl->volt_update |= EEM_VOLT_UPDATE\n"); */

	/* scale of det->volt_offset must equal 10uV */
	for (i = 0; i < det->num_freq_tbl; i++) {
		switch (det->ctrl_id) {
		case EEM_CTRL_BIG:
			det->volt_tbl_pmic[i] =
			min(
			(unsigned int)(clamp(
					(det->volt_tbl[i] + det->volt_offset + low_temp_offset + det->pi_offset),
					det->VMIN, det->VMAX)),
			(unsigned int)(*(recordTbl + ((i + 48) * 8) + 7) & 0x7F)
			);
			break;

		case EEM_CTRL_GPU:
			det->volt_tbl_pmic[i] =
			min(
			(unsigned int)(clamp(
				det->ops->eem_2_pmic(det, (det->volt_tbl[i] + det->volt_offset + low_temp_offset)),
				det->ops->eem_2_pmic(det, det->VMIN),
				det->ops->eem_2_pmic(det, det->VMAX))),
			(unsigned int)(*(gpuTbl + i))
			);
			break;

		case EEM_CTRL_L:
			det->volt_tbl_pmic[i] =
			min(
			(unsigned int)(clamp(
				det->ops->eem_2_pmic(det,
					(det->volt_tbl[i] + det->volt_offset + low_temp_offset)) + det->pi_offset,
				det->ops->eem_2_pmic(det, det->VMIN),
				det->ops->eem_2_pmic(det, det->VMAX))),
			(unsigned int)(*(recordTbl + ((i + 16) * 8) + 7) & 0x7F)
			);
			break;

		case EEM_CTRL_2L:
			det->volt_tbl_pmic[i] =
			min(
			(unsigned int)(clamp(
				det->ops->eem_2_pmic(det,
					(det->volt_tbl[i] + det->volt_offset + low_temp_offset)) + det->pi_offset,
				det->ops->eem_2_pmic(det, det->VMIN),
				det->ops->eem_2_pmic(det, det->VMAX))),
			(unsigned int)(*(recordTbl + (i * 8) + 7) & 0x7F)
			);
			break;

		case EEM_CTRL_CCI:
			det->volt_tbl_pmic[i] =
			min(
			(unsigned int)(clamp(
				det->ops->eem_2_pmic(det, (det->volt_tbl[i] + det->volt_offset + low_temp_offset)),
				det->ops->eem_2_pmic(det, det->VMIN),
				det->ops->eem_2_pmic(det, det->VMAX))),
			(unsigned int)(*(recordTbl + ((i + 32) * 8) + 7) & 0x7F)
			);
			break;

		default:
			break;
		}
		/*
		eem_debug("[%s].volt_tbl[%d] = 0x%X ----- volt_tbl_pmic[%d] = 0x%X (%d)\n",
			det->name,
			i, det->volt_tbl[i],
			i, det->volt_tbl_pmic[i], det->ops->pmic_2_volt(det, det->volt_tbl_pmic[i]));
		*/
	}

	#if defined(__MTK_SLT_)
		#if defined(SLT_VMAX)
		det->volt_tbl_pmic[0] =
			clamp(
				det->ops->eem_2_pmic(det,
					((det->volt_tbl[0] + det->volt_offset + low_temp_offset) -
						EEM_PMIC_NV_OFFSET +
						EEM_PMIC_LV_HV_OFFSET)),
				det->ops->eem_2_pmic(det, det->VMIN),
				det->ops->eem_2_pmic(det, det->VMAX)
			);
		eem_debug("VPROC voltage EEM to 0x%X\n", det->volt_tbl_pmic[0]);
		#elif defined(SLT_VMIN)
		det->volt_tbl_pmic[0] =
			clamp(
				det->ops->eem_2_pmic(det,
					((det->volt_tbl[0] + det->volt_offset + low_temp_offset) -
						EEM_PMIC_NV_OFFSET -
						EEM_PMIC_LV_HV_OFFSET)),
				det->ops->eem_2_pmic(det, det->VMIN),
				det->ops->eem_2_pmic(det, det->VMAX)
			);
		eem_debug("VPROC voltage EEM to 0x%X\n", det->volt_tbl_pmic[0]);
		#else
		/* det->volt_tbl_pmic[0] = det->volt_tbl_pmic[0] - EEM_PMIC_NV_OFFSET; */
		eem_debug("NV VPROC voltage EEM to 0x%X\n", det->volt_tbl_pmic[0]);
		#endif
	#endif

	#ifdef __KERNEL__
		if ((0 == (det->disabled % 2)) && (0 == (det->disabled & BY_PROCFS_INIT2)))
			wake_up_interruptible(&ctrl->wq);
		else
			eem_error("Disabled by [%d]\n", det->disabled);
	#else
		#if defined(__MTK_SLT_)
			if ((ctrl->volt_update & EEM_VOLT_UPDATE) && det->ops->set_volt)
				det->ops->set_volt(det);

			if ((ctrl->volt_update & EEM_VOLT_RESTORE) && det->ops->restore_default_volt)
				det->ops->restore_default_volt(det);

			ctrl->volt_update = EEM_VOLT_NONE;
		#endif
	#endif
#endif

	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
}

static void eem_restore_eem_volt(struct eem_det *det)
{
	#if SET_PMIC_VOLT
		struct eem_ctrl *ctrl = id_to_eem_ctrl(det->ctrl_id);

		ctrl->volt_update |= EEM_VOLT_RESTORE;
		#ifdef __KERNEL__
			wake_up_interruptible(&ctrl->wq);
		#else
			#if defined(__MTK_SLT_)
			if ((ctrl->volt_update & EEM_VOLT_UPDATE) && det->ops->set_volt)
				det->ops->set_volt(det);

			if ((ctrl->volt_update & EEM_VOLT_RESTORE) && det->ops->restore_default_volt)
				det->ops->restore_default_volt(det);

			ctrl->volt_update = EEM_VOLT_NONE;
			#endif
		#endif
	#endif

	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
}

#if 0
static void mt_eem_reg_dump_locked(void)
{
#ifndef CONFIG_ARM64
	unsigned int addr;

	for (addr = (unsigned int)EEM_DESCHAR; addr <= (unsigned int)EEM_SMSTATE1; addr += 4)
		eem_isr_info("0x%08X = 0x%08X\n", addr, *(volatile unsigned int *)addr);

	addr = (unsigned int)EEMCORESEL;
	eem_isr_info("0x%08X = 0x%08X\n", addr, *(volatile unsigned int *)addr);
#else
	unsigned long addr;

	for (addr = (unsigned long)EEM_DESCHAR; addr <= (unsigned long)EEM_SMSTATE1; addr += 4)
		eem_isr_info("0x%lu = 0x%lu\n", addr, *(volatile unsigned long *)addr);

	addr = (unsigned long)EEMCORESEL;
	eem_isr_info("0x%lu = 0x%lu\n", addr, *(volatile unsigned long *)addr);
#endif
}
#endif

static inline void handle_init01_isr(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_LOCAL);

	eem_isr_info("mode = init1 %s-isr\n", ((char *)(det->name) + 8));

	det->dcvalues[EEM_PHASE_INIT01]		= eem_read(EEM_DCVALUES);
	det->freqpct30[EEM_PHASE_INIT01]	= eem_read(EEM_FREQPCT30);
	det->eem_26c[EEM_PHASE_INIT01]		= eem_read(EEMINTEN + 0x10);
	det->vop30[EEM_PHASE_INIT01]		= eem_read(EEM_VOP30);
	det->eem_eemEn[EEM_PHASE_INIT01]	= eem_read(EEMEN);

	/*
	#if defined(__MTK_SLT_)
	eem_debug("CTP - dcvalues 240 = 0x%x\n", det->dcvalues[EEM_PHASE_INIT01]);
	eem_debug("CTP - freqpct30 218 = 0x%x\n", det->freqpct30[EEM_PHASE_INIT01]);
	eem_debug("CTP - eem_26c 26c = 0x%x\n", det->eem_26c[EEM_PHASE_INIT01]);
	eem_debug("CTP - vop30 248 = 0x%x\n", det->vop30[EEM_PHASE_INIT01]);
	eem_debug("CTP - eem_eemEn 238 = 0x%x\n", det->eem_eemEn[EEM_PHASE_INIT01]);i
	#endif
	*/
#if DUMP_DATA_TO_DE
	{
		unsigned int i;

		for (i = 0; i < ARRAY_SIZE(reg_dump_addr_off); i++) {
			det->reg_dump_data[i][EEM_PHASE_INIT01] = eem_read(EEM_BASEADDR + reg_dump_addr_off[i]);
			#ifdef __KERNEL__
			eem_isr_info("0x%lx = 0x%08x\n",
			#else
			eem_isr_info("0x%X = 0x%X\n",
			#endif
				(unsigned long)EEM_BASEADDR + reg_dump_addr_off[i],
				det->reg_dump_data[i][EEM_PHASE_INIT01]
				);
		}
	}
#endif
	/*
	 * Read & store 16 bit values EEM_DCVALUES.DCVOFFSET and
	 * EEM_AGEVALUES.AGEVOFFSET for later use in INIT2 procedure
	 */
	det->DCVOFFSETIN = ~(eem_read(EEM_DCVALUES) & 0xffff) + 1; /* hw bug, workaround */
	det->AGEVOFFSETIN = eem_read(EEM_AGEVALUES) & 0xffff;

	/*
	 * Set EEMEN.EEMINITEN/EEMEN.EEMINIT2EN = 0x0 &
	 * Clear EEM INIT interrupt EEMINTSTS = 0x00000001
	 */
	eem_write(EEMEN, 0x0);
	eem_write(EEMINTSTS, 0x1);
	/* det->ops->init02(det); */

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static unsigned int interpolate(unsigned int y1, unsigned int y0,
	unsigned int x1, unsigned int x0, unsigned int ym)
{
	unsigned int ratio, result;

	ratio = (((y1 - y0) * 100) + (x1 - x0 - 1)) / (x1 - x0);
	result =  (x1 - ((((y1 - ym) * 10000) + ratio - 1) / ratio) / 100);
	/*
	eem_debug("y1(%d), y0(%d), x1(%d), x0(%d), ym(%d), ratio(%d), rtn(%d)\n",
		y1, y0, x1, x0, ym, ratio, result);
	*/

	return result;
}

#if defined(__MTK_SLT_)
int cpu_in_mon;
#endif
static inline void handle_init02_isr(struct eem_det *det)
{
	unsigned int temp, i, j;
	/* struct eem_ctrl *ctrl = id_to_eem_ctrl(det->ctrl_id); */

	FUNC_ENTER(FUNC_LV_LOCAL);

	eem_isr_info("mode = init2 %s-isr\n", ((char *)(det->name) + 8));

	det->dcvalues[EEM_PHASE_INIT02]		= eem_read(EEM_DCVALUES);
	det->freqpct30[EEM_PHASE_INIT02]	= eem_read(EEM_FREQPCT30);
	det->eem_26c[EEM_PHASE_INIT02]		= eem_read(EEMINTEN + 0x10);
	det->vop30[EEM_PHASE_INIT02]	= eem_read(EEM_VOP30);
	det->eem_eemEn[EEM_PHASE_INIT02]	= eem_read(EEMEN);

	/*
	#if defined(__MTK_SLT_)
	eem_debug("CTP - dcvalues 240 = 0x%x\n", det->dcvalues[EEM_PHASE_INIT02]);
	eem_debug("CTP - freqpct30 218 = 0x%x\n", det->freqpct30[EEM_PHASE_INIT02]);
	eem_debug("CTP - eem_26c 26c = 0x%x\n", det->eem_26c[EEM_PHASE_INIT02]);
	eem_debug("CTP - vop30 248 = 0x%x\n", det->vop30[EEM_PHASE_INIT02]);
	eem_debug("CTP - eem_eemEn 238 = 0x%x\n", det->eem_eemEn[EEM_PHASE_INIT02]);
	#endif
	*/

#if DUMP_DATA_TO_DE
	for (i = 0; i < ARRAY_SIZE(reg_dump_addr_off); i++) {
		det->reg_dump_data[i][EEM_PHASE_INIT02] = eem_read(EEM_BASEADDR + reg_dump_addr_off[i]);
		#ifdef __KERNEL__
		eem_isr_info("0x%lx = 0x%08x\n",
		#else
		eem_isr_info("0x%X = 0x%X\n",
		#endif
			(unsigned long)EEM_BASEADDR + reg_dump_addr_off[i],
			det->reg_dump_data[i][EEM_PHASE_INIT02]
			);
	}
#endif
	temp = eem_read(EEM_VOP30);
	/* eem_debug("init02 read(EEM_VOP30) = 0x%08X\n", temp); */
	/* EEM_VOP30=>pmic value */
	det->volt_tbl[0] = (temp & 0xff);
	det->volt_tbl[1 * (NR_FREQ / 8)] = (temp >> 8)  & 0xff;
	det->volt_tbl[2 * (NR_FREQ / 8)] = (temp >> 16) & 0xff;
	det->volt_tbl[3 * (NR_FREQ / 8)] = (temp >> 24) & 0xff;

	temp = eem_read(EEM_VOP74);
	/* eem_debug("init02 read(EEM_VOP74) = 0x%08X\n", temp); */
	/* EEM_VOP74=>pmic value */
	det->volt_tbl[4 * (NR_FREQ / 8)] = (temp & 0xff);
	det->volt_tbl[5 * (NR_FREQ / 8)] = (temp >> 8)  & 0xff;
	det->volt_tbl[6 * (NR_FREQ / 8)] = (temp >> 16) & 0xff;
	det->volt_tbl[7 * (NR_FREQ / 8)] = (temp >> 24) & 0xff;

	if (NR_FREQ > 8) {
		for (i = 0; i < 8; i++) {
			for (j = 1; j < (NR_FREQ / 8); j++) {
				if (i < 7) {
					det->volt_tbl[(i * (NR_FREQ / 8)) + j] =
						interpolate(
							det->freq_tbl[(i * (NR_FREQ / 8))],
							det->freq_tbl[((1 + i) * (NR_FREQ / 8))],
							det->volt_tbl[(i * (NR_FREQ / 8))],
							det->volt_tbl[((1 + i) * (NR_FREQ / 8))],
							det->freq_tbl[(i * (NR_FREQ / 8)) + j]
						);
				} else {
					det->volt_tbl[(i * (NR_FREQ / 8)) + j] =
					clamp(
						interpolate(
							det->freq_tbl[((i - 1) * (NR_FREQ / 8))],
							det->freq_tbl[((i) * (NR_FREQ / 8))],
							det->volt_tbl[((i - 1) * (NR_FREQ / 8))],
							det->volt_tbl[((i) * (NR_FREQ / 8))],
							det->freq_tbl[(i * (NR_FREQ / 8)) + j]
						),
						det->VMIN,
						det->VMAX
					);
				}
			}
		}
	}

	/* backup to volt_tbl_init2 */
	memcpy(det->volt_tbl_init2, det->volt_tbl, sizeof(det->volt_tbl_init2));

	for (i = 0; i < NR_FREQ; i++) {
		#if defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING) /* I-Chang */
		switch (det->ctrl_id) {
		case EEM_CTRL_BIG:
			if (i < 8) {
				aee_rr_rec_ptp_cpu_big_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_cpu_big_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_cpu_big_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_cpu_big_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		case EEM_CTRL_GPU:
			if (i < 8) {
				aee_rr_rec_ptp_gpu_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_gpu_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_gpu_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_gpu_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		case EEM_CTRL_L:
			if (i < 8) {
				aee_rr_rec_ptp_cpu_little_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_cpu_little_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_cpu_little_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_cpu_little_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		case EEM_CTRL_2L:
			if (i < 8) {
				aee_rr_rec_ptp_cpu_2_little_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_cpu_2_little_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_cpu_2_little_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_cpu_2_little_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		case EEM_CTRL_CCI:
			if (i < 8) {
				aee_rr_rec_ptp_cpu_cci_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_cpu_cci_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_cpu_cci_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_cpu_cci_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		default:
			break;
		}
		#endif
		/*
		eem_error("init02_[%s].volt_tbl[%d] = 0x%X (%d)\n",
			det->name, i, det->volt_tbl[i], det->ops->pmic_2_volt(det, det->volt_tbl[i]));

		if (NR_FREQ > 8) {
			eem_error("init02_[%s].volt_tbl[%d] = 0x%X (%d)\n",
			det->name, i+1, det->volt_tbl[i+1], det->ops->pmic_2_volt(det, det->volt_tbl[i+1]));
		}
		*/
	}
	#if defined(__MTK_SLT_)
	if (det->ctrl_id <= EEM_CTRL_BIG)
		cpu_in_mon = 0;
	#endif
	eem_set_eem_volt(det);

	/*
	 * Set EEMEN.EEMINITEN/EEMEN.EEMINIT2EN = 0x0 &
	 * Clear EEM INIT interrupt EEMINTSTS = 0x00000001
	 */
	eem_write(EEMEN, 0x0);
	eem_write(EEMINTSTS, 0x1);

	/* atomic_dec(&ctrl->in_init); */
	/* complete(&ctrl->init_done); */
	det->ops->mon_mode(det);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void handle_init_err_isr(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_LOCAL);
	eem_error("====================================================\n");
	eem_error("EEM init err: EEMEN(%p) = 0x%X, EEMINTSTS(%p) = 0x%X\n",
		     EEMEN, eem_read(EEMEN),
		     EEMINTSTS, eem_read(EEMINTSTS));
	eem_error("EEM_SMSTATE0 (%p) = 0x%X\n",
		     EEM_SMSTATE0, eem_read(EEM_SMSTATE0));
	eem_error("EEM_SMSTATE1 (%p) = 0x%X\n",
		     EEM_SMSTATE1, eem_read(EEM_SMSTATE1));
	eem_error("====================================================\n");

	/*
	TODO: FIXME
	{
		struct eem_ctrl *ctrl = id_to_eem_ctrl(det->ctrl_id);
		atomic_dec(&ctrl->in_init);
		complete(&ctrl->init_done);
	}
	TODO: FIXME
	*/

#ifdef __KERNEL__
	aee_kernel_warning("mt_eem", "@%s():%d, get_volt(%s) = 0x%08X\n",
		__func__,
		__LINE__,
		det->name,
		det->VBOOT);
#endif
	det->ops->disable_locked(det, BY_INIT_ERROR);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void handle_mon_mode_isr(struct eem_det *det)
{
	unsigned int temp, i, j;

	FUNC_ENTER(FUNC_LV_LOCAL);

	eem_isr_info("mode = mon %s-isr\n", ((char *)(det->name) + 8));

	#if defined(CONFIG_THERMAL) && !defined(EARLY_PORTING)
	eem_isr_info("B_temp=%d, G_temp=%d, L_temp=%d, 2L_temp=%d CCI_temp=%d\n",
		tscpu_get_temp_by_bank(THERMAL_BANK0),
		tscpu_get_temp_by_bank(THERMAL_BANK1),
		tscpu_get_temp_by_bank(THERMAL_BANK3),
		tscpu_get_temp_by_bank(THERMAL_BANK4),
		tscpu_get_temp_by_bank(THERMAL_BANK5)
		);
	#endif

	#if defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING)
	switch (det->ctrl_id) {
	case EEM_CTRL_BIG:
		#ifdef CONFIG_THERMAL
		if (tscpu_get_temp_by_bank(THERMAL_BANK0) != 0) {
			aee_rr_rec_ptp_temp(
			(tscpu_get_temp_by_bank(THERMAL_BANK0) / 1000) << (8 * EEM_CPU_BIG_IS_SET_VOLT) |
			(aee_rr_curr_ptp_temp() & ~(0xFF << (8 * EEM_CPU_BIG_IS_SET_VOLT))));
		}
		#endif
		break;

	case EEM_CTRL_GPU:
		#ifdef CONFIG_THERMAL
		if (tscpu_get_temp_by_bank(THERMAL_BANK1) != 0) {
			aee_rr_rec_ptp_temp(
			(tscpu_get_temp_by_bank(THERMAL_BANK1) / 1000) << (8 * EEM_GPU_IS_SET_VOLT) |
			(aee_rr_curr_ptp_temp() & ~(0xFF << (8 * EEM_GPU_IS_SET_VOLT))));
		}
		#endif
		break;

	case EEM_CTRL_L:
	case EEM_CTRL_2L:
	case EEM_CTRL_CCI:
		#ifdef CONFIG_THERMAL
		if (tscpu_get_temp_by_bank(THERMAL_BANK3) != 0) {
			aee_rr_rec_ptp_temp(
			(tscpu_get_temp_by_bank(THERMAL_BANK3) / 1000) << (8 * EEM_CPU_LITTLE_IS_SET_VOLT) |
			(aee_rr_curr_ptp_temp() & ~(0xFF << (8 * EEM_CPU_LITTLE_IS_SET_VOLT))));
		}
		#endif
		break;

	default:
		break;
	}
	#endif

	det->dcvalues[EEM_PHASE_MON]	= eem_read(EEM_DCVALUES);
	det->freqpct30[EEM_PHASE_MON]	= eem_read(EEM_FREQPCT30);
	det->eem_26c[EEM_PHASE_MON]	= eem_read(EEMINTEN + 0x10);
	det->vop30[EEM_PHASE_MON]	= eem_read(EEM_VOP30);
	det->eem_eemEn[EEM_PHASE_MON]	= eem_read(EEMEN);

	/*
	#if defined(__MTK_SLT_)
	eem_debug("CTP - dcvalues 240 = 0x%x\n", det->dcvalues[EEM_PHASE_MON]);
	eem_debug("CTP - freqpct30 218 = 0x%x\n", det->freqpct30[EEM_PHASE_MON]);
	eem_debug("CTP - eem_26c 26c = 0x%x\n", det->eem_26c[EEM_PHASE_MON]);
	eem_debug("CTP - vop30 248 = 0x%x\n", det->vop30[EEM_PHASE_MON]);
	eem_debug("CTP - eem_eemEn 238 = 0x%x\n", det->eem_eemEn[EEM_PHASE_MON]);
	#endif
	*/

	#if DUMP_DATA_TO_DE
	for (i = 0; i < ARRAY_SIZE(reg_dump_addr_off); i++) {
		det->reg_dump_data[i][EEM_PHASE_MON] = eem_read(EEM_BASEADDR + reg_dump_addr_off[i]);

		#ifdef __KERNEL__
		eem_isr_info("0x%lx = 0x%08x\n",
		#else
		eem_isr_info("0x%X = 0x%X\n",
		#endif
			(unsigned long)EEM_BASEADDR + reg_dump_addr_off[i],
			det->reg_dump_data[i][EEM_PHASE_MON]
			);
	}
	#endif
	/* check if thermal sensor init completed? */
	det->t250 = eem_read(TEMP);

	/* 0x64 mappint to 100 + 25 = 125C,
	   0xB2 mapping to 178 - 128 = 50, -50 + 25 = -25C */
	if (((det->t250 & 0xff)  > 0x64) && ((det->t250  & 0xff) < 0xB2)) {
		eem_error("Temperature to high ----- (%d) degree !!\n", det->t250 + 25);
		goto out;
	}

	temp = eem_read(EEM_VOP30);
	/* eem_debug("mon eem_read(EEM_VOP30) = 0x%08X\n", temp); */
	/* EEM_VOP30=>pmic value */
	det->volt_tbl[0] = (temp & 0xff);
	det->volt_tbl[1 * (NR_FREQ / 8)] = (temp >> 8)  & 0xff;
	det->volt_tbl[2 * (NR_FREQ / 8)] = (temp >> 16) & 0xff;
	det->volt_tbl[3 * (NR_FREQ / 8)] = (temp >> 24) & 0xff;

	temp = eem_read(EEM_VOP74);
	/* eem_debug("mon eem_read(EEM_VOP74) = 0x%08X\n", temp); */
	/* EEM_VOP74=>pmic value */
	det->volt_tbl[4 * (NR_FREQ / 8)] = (temp & 0xff);
	det->volt_tbl[5 * (NR_FREQ / 8)] = (temp >> 8)  & 0xff;
	det->volt_tbl[6 * (NR_FREQ / 8)] = (temp >> 16) & 0xff;
	det->volt_tbl[7 * (NR_FREQ / 8)] = (temp >> 24) & 0xff;

	if (NR_FREQ > 8) {
		for (i = 0; i < 8; i++) {
			for (j = 1; j < (NR_FREQ / 8); j++) {
				if (i < 7) {
					det->volt_tbl[(i * (NR_FREQ / 8)) + j] =
						interpolate(
							det->freq_tbl[(i * (NR_FREQ / 8))],
							det->freq_tbl[((1 + i) * (NR_FREQ / 8))],
							det->volt_tbl[(i * (NR_FREQ / 8))],
							det->volt_tbl[((1 + i) * (NR_FREQ / 8))],
							det->freq_tbl[(i * (NR_FREQ / 8)) + j]
						);
				} else {
					det->volt_tbl[(i * (NR_FREQ / 8)) + j] =
						clamp(
							interpolate(
								det->freq_tbl[((i - 1) * (NR_FREQ / 8))],
								det->freq_tbl[((i) * (NR_FREQ / 8))],
								det->volt_tbl[((i - 1) * (NR_FREQ / 8))],
								det->volt_tbl[((i) * (NR_FREQ / 8))],
								det->freq_tbl[(i * (NR_FREQ / 8)) + j]
							),
							det->VMIN,
							det->VMAX
						);
				}
			}
		}
	}

	for (i = 0; i < NR_FREQ; i++) {
		#if defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING) /* I-Chang */
		switch (det->ctrl_id) {
		case EEM_CTRL_BIG:
			if (i < 8) {
				aee_rr_rec_ptp_cpu_big_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_cpu_big_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_cpu_big_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_cpu_big_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		case EEM_CTRL_GPU:
			if (i < 8) {
				aee_rr_rec_ptp_gpu_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_gpu_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_gpu_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_gpu_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		case EEM_CTRL_L:
			if (i < 8) {
				aee_rr_rec_ptp_cpu_little_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_cpu_little_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_cpu_little_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_cpu_little_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		case EEM_CTRL_2L:
			if (i < 8) {
				aee_rr_rec_ptp_cpu_2_little_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_cpu_2_little_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_cpu_2_little_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_cpu_2_little_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		case EEM_CTRL_CCI:
			if (i < 8) {
				aee_rr_rec_ptp_cpu_cci_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_cpu_cci_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_cpu_cci_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_cpu_cci_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		default:
			break;
		}
		#endif

		eem_isr_info("mon_[%s].volt_tbl[%d] = 0x%X (%d)\n",
			det->name, i, det->volt_tbl[i], det->ops->pmic_2_volt(det, det->volt_tbl[i]));

		if (NR_FREQ > 8) {
			eem_isr_info("mon_[%s].volt_tbl[%d] = 0x%X (%d)\n",
			det->name, i+1, det->volt_tbl[i+1], det->ops->pmic_2_volt(det, det->volt_tbl[i+1]));
		}
	}
	eem_isr_info("ISR : EEM_TEMPSPARE1 = 0x%08X\n", eem_read(EEM_TEMPSPARE1));

	#if defined(__MTK_SLT_)
		if ((cpu_in_mon == 1) && (det->ctrl_id <= EEM_CTRL_BIG))
			eem_isr_info("Won't do CPU eem_set_eem_volt\n");
		else{
			if (det->ctrl_id <= EEM_CTRL_BIG) {
				eem_isr_info("Do CPU eem_set_eem_volt\n");
				cpu_in_mon = 1;
			}
			eem_set_eem_volt(det);
		}
	#else
		eem_set_eem_volt(det);
		if (EEM_CTRL_BIG == det->ctrl_id)
			infoIdvfs = 0xff;
	#endif

out:
	/* Clear EEM INIT interrupt EEMINTSTS = 0x00ff0000 */
	eem_write(EEMINTSTS, 0x00ff0000);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void handle_mon_err_isr(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_LOCAL);

	/* EEM Monitor mode error handler */
	eem_error("====================================================\n");
	eem_error("EEM mon err: EEMCORESEL(%p) = 0x%08X, EEM_THERMINTST(%p) = 0x%08X, EEMODINTST(%p) = 0x%08X",
		     EEMCORESEL, eem_read(EEMCORESEL),
		     EEM_THERMINTST, eem_read(EEM_THERMINTST),
		     EEMODINTST, eem_read(EEMODINTST));
	eem_error(" EEMINTSTSRAW(%p) = 0x%08X, EEMINTEN(%p) = 0x%08X\n",
		     EEMINTSTSRAW, eem_read(EEMINTSTSRAW),
		     EEMINTEN, eem_read(EEMINTEN));
	eem_error("====================================================\n");
	eem_error("EEM mon err: EEMEN(%p) = 0x%08X, EEMINTSTS(%p) = 0x%08X\n",
		     EEMEN, eem_read(EEMEN),
		     EEMINTSTS, eem_read(EEMINTSTS));
	eem_error("EEM_SMSTATE0 (%p) = 0x%08X\n",
		     EEM_SMSTATE0, eem_read(EEM_SMSTATE0));
	eem_error("EEM_SMSTATE1 (%p) = 0x%08X\n",
		     EEM_SMSTATE1, eem_read(EEM_SMSTATE1));
	eem_error("TEMP (%p) = 0x%08X\n",
		     TEMP, eem_read(TEMP));
	eem_error("EEM_TEMPMSR0 (%p) = 0x%08X\n",
		     EEM_TEMPMSR0, eem_read(EEM_TEMPMSR0));
	eem_error("EEM_TEMPMSR1 (%p) = 0x%08X\n",
		     EEM_TEMPMSR1, eem_read(EEM_TEMPMSR1));
	eem_error("EEM_TEMPMSR2 (%p) = 0x%08X\n",
		     EEM_TEMPMSR2, eem_read(EEM_TEMPMSR2));
	eem_error("EEM_TEMPMONCTL0 (%p) = 0x%08X\n",
		     EEM_TEMPMONCTL0, eem_read(EEM_TEMPMONCTL0));
	eem_error("EEM_TEMPMSRCTL1 (%p) = 0x%08X\n",
		     EEM_TEMPMSRCTL1, eem_read(EEM_TEMPMSRCTL1));
	eem_error("====================================================\n");

	{
		unsigned int i;

		for (i = 0; i < ARRAY_SIZE(reg_dump_addr_off); i++) {
			det->reg_dump_data[i][EEM_PHASE_MON] = eem_read(EEM_BASEADDR + reg_dump_addr_off[i]);
			eem_error("0x%lx = 0x%08x\n",
				(unsigned long)EEM_BASEADDR + reg_dump_addr_off[i],
				det->reg_dump_data[i][EEM_PHASE_MON]
				);
		}
	}
	eem_error("====================================================\n");
	eem_error("EEM mon err: EEMCORESEL(%p) = 0x%08X, EEM_THERMINTST(%p) = 0x%08X, EEMODINTST(%p) = 0x%08X",
		     EEMCORESEL, eem_read(EEMCORESEL),
		     EEM_THERMINTST, eem_read(EEM_THERMINTST),
		     EEMODINTST, eem_read(EEMODINTST));
	eem_error(" EEMINTSTSRAW(%p) = 0x%08X, EEMINTEN(%p) = 0x%08X\n",
		     EEMINTSTSRAW, eem_read(EEMINTSTSRAW),
		     EEMINTEN, eem_read(EEMINTEN));
	eem_error("====================================================\n");

#ifdef __KERNEL__
	aee_kernel_warning("mt_eem", "@%s():%d, get_volt(%s) = 0x%08X\n", __func__, __LINE__, det->name, det->VBOOT);
	/* BUG(); */
#endif
	det->ops->disable_locked(det, BY_MON_ERROR);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void eem_isr_handler(struct eem_det *det)
{
	unsigned int eemintsts, eemen;

	FUNC_ENTER(FUNC_LV_LOCAL);

	eemintsts = eem_read(EEMINTSTS);
	eemen = eem_read(EEMEN);

	eem_debug("Bk_No = %d %s-isr, 0x%X, 0x%X\n",
		det->ctrl_id, ((char *)(det->name) + 8), eemintsts, eemen);

	if (eemintsts == 0x1) { /* EEM init1 or init2 */
		if ((eemen & 0x7) == 0x1) /* EEM init1 */
			handle_init01_isr(det);
		else if ((eemen & 0x7) == 0x5) /* EEM init2 */
			handle_init02_isr(det);
		else {
			/* error : init1 or init2 */
			handle_init_err_isr(det);
		}
	} else if ((eemintsts & 0x00ff0000) != 0x0)
		handle_mon_mode_isr(det);
	else { /* EEM error handler */
		/* init 1  || init 2 error handler */
		if (((eemen & 0x7) == 0x1) || ((eemen & 0x7) == 0x5))
			handle_init_err_isr(det);
		else /* EEM Monitor mode error handler */
			handle_mon_err_isr(det);
	}

	FUNC_EXIT(FUNC_LV_LOCAL);
}
#ifdef __KERNEL__
static irqreturn_t eem_isr(int irq, void *dev_id)
#else
int ptp_isr(void)
#endif
{
	unsigned long flags;
	struct eem_det *det = NULL;
	int i;

	FUNC_ENTER(FUNC_LV_MODULE);

	mt_ptp_lock(&flags);

#if 0
	if (!(BIT(PTP_CTRL_VCORE) & eem_read(EEMODINTST))) {
		switch (eem_read_field(PERI_VCORE_PTPOD_CON0, VCORE_PTPODSEL)) {
		case SEL_VCORE_AO:
			det = &eem_detectors[PTP_DET_VCORE_AO];
			break;

		case SEL_VCORE_PDN:
			det = &eem_detectors[PTP_DET_VCORE_PDN];
			break;
		}

		if (likely(det)) {
			det->ops->switch_bank(det);
			eem_isr_handler(det);
		}
	}
#endif

	for (i = 0; i < NR_EEM_CTRL; i++) {

		if (i == EEM_CTRL_SOC)
			continue;

		/* TODO: FIXME, it is better to link i @ struct eem_det */
		if ((BIT(i) & eem_read(EEMODINTST)))
			continue;

		det = &eem_detectors[i];

		det->ops->switch_bank(det);

		/*mt_eem_reg_dump_locked(); */

		eem_isr_handler(det);
	}

	mt_ptp_unlock(&flags);

	FUNC_EXIT(FUNC_LV_MODULE);
#ifdef __KERNEL__
	return IRQ_HANDLED;
#else
	return 0;
#endif
}

#define SINGLE_CPU_NUM 1
static int __cpuinit _mt_eem_cpu_CB(struct notifier_block *nfb,
					unsigned long action, void *hcpu)
{
	unsigned int cpu = (unsigned long)hcpu;
	unsigned int online_cpus = num_online_cpus();
	struct device *dev;
	enum mt_eem_cpu_id cluster_id;

	/* CPU mask - Get on-line cpus per-cluster */
	struct cpumask eem_cpumask;
	struct cpumask cpu_online_cpumask;
	unsigned int cpus;

	if (0 == ctrl_ITurbo) {
		eem_debug("Default I-Turbo off !!");
		return NOTIFY_OK;
	}

	/* Current active CPU is belong which cluster */
	cluster_id = arch_get_cluster_id(cpu);

	/* How many CPU in this cluster, present by bit mask
		ex:	B	L	LL
			00	1111	0000 */
	arch_get_cluster_cpus(&eem_cpumask, cluster_id);

	/* How many CPU online in this cluster */
	cpumask_and(&cpu_online_cpumask, &eem_cpumask, cpu_online_mask);
	cpus = cpumask_weight(&cpu_online_cpumask);

	if (eem_log_en)
		eem_error("@%s():%d, cpu = %d, act = %lu, on_cpus = %d, clst = %d, clst_cpu = %d\n"
		, __func__, __LINE__, cpu, action, online_cpus, cluster_id, cpus);

	dev = get_cpu_device(cpu);

	if (dev) {
		switch (action) {
		case CPU_POST_DEAD:
			if (SINGLE_CPU_NUM == online_cpus) { /* Online cpu is single */
				arch_get_cluster_cpus(&eem_cpumask, MT_EEM_CPU_L);
				cpumask_and(&cpu_online_cpumask, &eem_cpumask, cpu_online_mask);
				cpus = cpumask_weight(&cpu_online_cpumask);
				if (eem_log_en)
					eem_error("L_cluster_cpus = (%d)\n", cpus);
				if (SINGLE_CPU_NUM == cpus) { /* The single cpu located at L cluster */
					if (0 == ITurboRun) {
						ITurboRun = 1;
						eem_error("ITurbo(1) L_cc(%d)\n", cpus);
					} else {
						eem_debug("ITurbo(1)ed L_cc(%d)\n", cpus);
					}
				}
			}
		break;
		case CPU_UP_PREPARE:
			if (1 == ITurboRun) {
				ITurboRun = 0;
				if (eem_log_en)
					eem_error("ITurbo(0) ->(%d), c(%d), cc(%d)\n", online_cpus, cluster_id, cpus);
			} else {
				if (eem_log_en)
					eem_error("ITurbo(0)ed ->(%d), c(%d), cc(%d)\n", online_cpus, cluster_id, cpus);
			}
		break;
		}
	}
	return NOTIFY_OK;
}

void eem_init02(void)
{
	struct eem_det *det;
	struct eem_ctrl *ctrl;

	FUNC_ENTER(FUNC_LV_LOCAL);

	for_each_det_ctrl(det, ctrl) {
		if (HAS_FEATURE(det, FEA_MON)) {
			unsigned long flag;

			mt_ptp_lock(&flag);
			det->ops->init02(det);
			mt_ptp_unlock(&flag);
		}
	}

	FUNC_EXIT(FUNC_LV_LOCAL);
}

void eem_init01(void)
{
	struct eem_det *det;
	struct eem_ctrl *ctrl;
	unsigned int out = 0, timeout = 0;

	FUNC_ENTER(FUNC_LV_LOCAL);

	for_each_det_ctrl(det, ctrl) {
		unsigned long flag;
		unsigned int vboot = 0;

		if (HAS_FEATURE(det, FEA_INIT01)) {
			if (NULL != det->ops->get_volt) {
				if (EEM_CTRL_BIG == det->ctrl_id)
					vboot = det->ops->volt_2_pmic(det, det->ops->get_volt(det));
				else
					vboot = det->ops->volt_2_eem(det, det->ops->get_volt(det));

				#if defined(CONFIG_EEM_AEE_RR_REC)
					aee_rr_rec_ptp_vboot(
						((unsigned long long)(vboot) << (8 * det->ctrl_id)) |
						(aee_rr_curr_ptp_vboot() & ~
							((unsigned long long)(0xFF) << (8 * det->ctrl_id))
						)
					);
				#endif
			}

			eem_error("%s, vboot = %d, VBOOT = %d\n", ((char *)(det->name) + 8), vboot, det->VBOOT);
			#ifdef __KERNEL__
				if (vboot != det->VBOOT) {
					aee_kernel_warning("mt_eem",
						"@%s():%d, get_volt(%s) = 0x%08X, VBOOT = 0x%08X\n",
						__func__, __LINE__, det->name, vboot, det->VBOOT);
				}
				BUG_ON(vboot != det->VBOOT);
			#endif
			mt_ptp_lock(&flag); /* <-XXX */
			det->ops->init01(det);
			mt_ptp_unlock(&flag); /* <-XXX */

			/*
			 * VCORE_AO and VCORE_PDN use the same controller.
			 * Wait until VCORE_AO init01 and init02 done
			 */

			/*
			if (atomic_read(&ctrl->in_init)) {
				TODO: Use workqueue to avoid blocking
				wait_for_completion(&ctrl->init_done);
			}
			*/
		}
	}

	/* set non-PWM mode for DA9214 */
	da9214_config_interface(0x0, 0x1, 0xF, 0);  /* select to page 2,3 */
	da9214_config_interface(0xD1, 0x1, 0x3, 0); /* Disable Buck A PWM Mode for LL/L/CCI */
	da9214_config_interface(0x0, 0x1, 0xF, 0);  /* select to page 2,3 */
	da9214_config_interface(0xD2, 0x1, 0x3, 0); /* Disable Buck B PWM Mode for BIG */
	/* set non_PWM mode for FAN5355 */
	fan53555_config_interface(0x0, 0x0, 0x1, 6); /* Disable PWM mode for GPU */

	#ifdef __KERNEL__
		#ifndef EARLY_PORTING
			#if !defined(CONFIG_MTK_CLKMGR)
				clk_disable_unprepare(clk_mfg); /* Disable GPU clock */
				clk_disable_unprepare(clk_mfg_scp); /* Disable GPU MTCMOSE */
				clk_disable_unprepare(clk_thermal); /* Disable Thermal clock */
			#endif
			mt_gpufreq_enable_by_ptpod(); /* enable gpu DVFS */
			mt_ppm_ptpod_policy_deactivate();
			if (cpu_online(8))
				cpu_down(8);
			/* enable frequency hopping (main PLL) */
			/* mt_fh_popod_restore(); */
		#endif
	#endif

	informGpuEEMisReady = 1;

	/* This patch is waiting for whole bank finish the init01 then go
	 * next. Due to LL/L use same bulk PMIC, LL voltage table change
	 * will impact L to process init01 stage, because L require a
	 * stable 1V for init01.
	*/
	while (1) {
		for_each_det_ctrl(det, ctrl) {
			if ((EEM_CTRL_BIG == det->ctrl_id) && (1 == det->eem_eemEn[EEM_PHASE_INIT01]))
				out |= BIT(EEM_CTRL_BIG);
			else if ((EEM_CTRL_GPU == det->ctrl_id) && (1 == det->eem_eemEn[EEM_PHASE_INIT01]))
				out |= BIT(EEM_CTRL_GPU);
			else if ((EEM_CTRL_L == det->ctrl_id) && (1 == det->eem_eemEn[EEM_PHASE_INIT01]))
				out |= BIT(EEM_CTRL_L);
			else if ((EEM_CTRL_2L == det->ctrl_id) && (1 == det->eem_eemEn[EEM_PHASE_INIT01]))
				out |= BIT(EEM_CTRL_2L);
			else if ((EEM_CTRL_CCI == det->ctrl_id) && (1 == det->eem_eemEn[EEM_PHASE_INIT01]))
				out |= BIT(EEM_CTRL_CCI);
		}
		if ((0x3B == out) || (30 == timeout)) {
			eem_debug("init01 finish time is %d\n", timeout);
			break;
		}
		udelay(100);
		timeout++;
	}
	eem_init02();
	FUNC_EXIT(FUNC_LV_LOCAL);
}

#if EN_EEM
#if 0
static char *readline(struct file *fp)
{
#define BUFSIZE 1024
	static char buf[BUFSIZE];
	static int buf_end;
	static int line_start;
	static int line_end;
	char *ret;

	FUNC_ENTER(FUNC_LV_HELP);
empty:

	if (line_start >= buf_end) {
		line_start = 0;
		buf_end = fp->f_op->read(fp, &buf[line_end], sizeof(buf) - line_end, &fp->f_pos);

		if (0 == buf_end) {
			line_end = 0;
			return NULL;
		}

		buf_end += line_end;
	}

	while (buf[line_end] != '\n') {
		line_end++;

		if (line_end >= buf_end) {
			memcpy(&buf[0], &buf[line_start], buf_end - line_start);
			line_end = buf_end - line_start;
			line_start = buf_end;
			goto empty;
		}
	}

	buf[line_end] = '\0';
	ret = &buf[line_start];
	line_start = line_end + 1;

	FUNC_EXIT(FUNC_LV_HELP);

	return ret;
}
#endif
/* leakage */
unsigned int leakage_core;
unsigned int leakage_gpu;
unsigned int leakage_sram2;
unsigned int leakage_sram1;


void get_devinfo(struct eem_devinfo *p)
{
	int *val = (int *)p;
	int i;

	FUNC_ENTER(FUNC_LV_HELP);
	checkEfuse = 1;

#ifndef EARLY_PORTING
	#if defined(__KERNEL__)
		val[0] = get_devinfo_with_index(50);
		val[1] = get_devinfo_with_index(51);
		val[2] = get_devinfo_with_index(52);
		val[3] = get_devinfo_with_index(53);
		val[4] = get_devinfo_with_index(56);
		val[5] = get_devinfo_with_index(57);
		val[6] = get_devinfo_with_index(58);
		val[7] = get_devinfo_with_index(59);
		val[8] = get_devinfo_with_index(60);
		val[9] = get_devinfo_with_index(61);
		val[10] = get_devinfo_with_index(65);
		val[11] = get_devinfo_with_index(66);
		#if defined(CONFIG_EEM_AEE_RR_REC)
			aee_rr_rec_ptp_60((unsigned int)val[0]);
			aee_rr_rec_ptp_64((unsigned int)val[1]);
			aee_rr_rec_ptp_68((unsigned int)val[2]);
			aee_rr_rec_ptp_6C((unsigned int)val[3]);
			aee_rr_rec_ptp_78((unsigned int)val[4]);
			aee_rr_rec_ptp_7C((unsigned int)val[5]);
			aee_rr_rec_ptp_80((unsigned int)val[6]);
			aee_rr_rec_ptp_84((unsigned int)val[7]);
			aee_rr_rec_ptp_88((unsigned int)val[8]);
			aee_rr_rec_ptp_8C((unsigned int)val[9]);
			aee_rr_rec_ptp_9C((unsigned int)val[10]);
			aee_rr_rec_ptp_A0((unsigned int)val[11]);
		#endif
	#else
		val[0] = eem_read(0x10206960); /* M_HW_RES0 */
		val[1] = eem_read(0x10206964); /* M_HW_RES1 */
		val[2] = eem_read(0x10206968); /* M_HW_RES2 */
		val[3] = eem_read(0x1020696C); /* M_HW_RES3 */
		val[4] = eem_read(0x10206978); /* M_HW_RES6 */
		val[5] = eem_read(0x1020697C); /* M_HW_RES7 */
		val[6] = eem_read(0x10206980); /* M_HW_RES8 */
		val[7] = eem_read(0x10206984); /* M_HW_RES9 */
		val[8] = eem_read(0x10206988); /* M_HW_RES10 */
		val[9] = eem_read(0x1020698C); /* M_HW_RES11 */
		val[10] = eem_read(0x1020699C); /* M_HW_RES15 */
		val[11] = eem_read(0x102069A0); /* M_HW_RES16 */

	#endif
#else
	/* test pattern */
	val[0] = 0x17F75060;
	val[1] = 0x00540003;
	val[2] = 0x187E3D12; /* 0x18A73D12 */
	val[3] = 0x00560003;
	val[4] = 0x187E6103; /* 0x18A46103 */
	val[5] = 0x00450003;
	val[6] = 0x187E5E06; /* 0x18A25E06 */
	val[7] = 0x00450003;
	val[8] = 0x187E6004; /* 0x18A56004 */
	val[9] = 0x00450010;
	val[10] = 0x0070004E;
	val[11] = 0x0F18315B;
#endif
	eem_debug("M_HW_RES0 = 0x%X\n", val[0]);
	eem_debug("M_HW_RES1 = 0x%X\n", val[1]);
	eem_debug("M_HW_RES2 = 0x%X\n", val[2]);
	eem_debug("M_HW_RES3 = 0x%X\n", val[3]);
	eem_debug("M_HW_RES6 = 0x%X\n", val[4]);
	eem_debug("M_HW_RES7 = 0x%X\n", val[5]);
	eem_debug("M_HW_RES8 = 0x%X\n", val[6]);
	eem_debug("M_HW_RES9 = 0x%X\n", val[7]);
	eem_debug("M_HW_RES10 = 0x%X\n", val[8]);
	eem_debug("M_HW_RES11 = 0x%X\n", val[9]);
	eem_debug("M_HW_RES15 = 0x%X\n", val[10]);
	eem_debug("M_HW_RES16 = 0x%X\n", val[11]);

	for (i = 0; i < 12; i++) {
		if (0 == val[i]) {
			ctrl_EEM_Enable = 0;
			infoIdvfs = 0x55;
			checkEfuse = 0;
			eem_error("No EEM EFUSE available, will turn off EEM !!\n");
			break;
		}
	}
	/*
	if (i < 12) {
		val[0] = 0x1410595E;
		val[1] = 0x004C0003;
		val[2] = 0x14C0570C;
		val[3] = 0x004B0003;
		val[4] = 0x14C06B01;
		val[5] = 0x003F0003;
		val[6] = 0x14C06B05;
		val[7] = 0x003F0303;
		val[8] = 0x14C06804;
		val[9] = 0x003F0000;
		val[10] = 0x00000047;
		val[11] = 0x0D2B365A;
	}
	*/
	FUNC_EXIT(FUNC_LV_HELP);
}

static int eem_probe(struct platform_device *pdev)
{
	int ret;
	struct eem_det *det;
	struct eem_ctrl *ctrl;
	/* unsigned int code = mt_get_chip_hw_code(); */

	FUNC_ENTER(FUNC_LV_MODULE);

	#ifdef __KERNEL__
		#if !defined(CONFIG_MTK_CLKMGR) && !defined(EARLY_PORTING)
			/* enable thermal CG */
			clk_thermal = devm_clk_get(&pdev->dev, "therm-eem");
			if (IS_ERR(clk_thermal)) {
				eem_error("cannot get thermal clock\n");
				return PTR_ERR(clk_thermal);
			}
			/* get GPU clock */
			clk_mfg = devm_clk_get(&pdev->dev, "mfg-main");
			if (IS_ERR(clk_mfg)) {
				eem_error("cannot get mfg main clock\n");
				return PTR_ERR(clk_mfg);
			}
			/* get GPU mtcomose */
			clk_mfg_scp = devm_clk_get(&pdev->dev, "mtcmos-mfg");
			if (IS_ERR(clk_mfg_scp)) {
				eem_error("cannot get mtcmos mfg\n");
				return PTR_ERR(clk_mfg_scp);
			}
			eem_debug("thmal=%p, gpu_clk=%p, gpu_mtcmos=%p",
				clk_thermal,
				clk_mfg,
				clk_mfg_scp);
		#endif

		/* set EEM IRQ */
		ret = request_irq(eem_irq_number, eem_isr, IRQF_TRIGGER_LOW, "eem", NULL);
		if (ret) {
			eem_debug("EEM IRQ register failed (%d)\n", ret);
			WARN_ON(1);
		}
		eem_debug("Set EEM IRQ OK.\n");
	#endif
	eem_debug("In eem_probe\n");

	#if (defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING))
		_mt_eem_aee_init();
	#endif

	for_each_ctrl(ctrl) {
		if (EEM_DET_SOC != ctrl->det_id)
			eem_init_ctrl(ctrl);
	}
	#ifdef __KERNEL__
		#ifndef EARLY_PORTING
			/* disable frequency hopping (main PLL) */
			/* mt_fh_popod_save(); */

			set_cpu_present(8, true); /* enable big cluster */
			if (!cpu_online(8))
				cpu_up(8);
			/* disable DVFS and set vproc = 1v (LITTLE = 689 MHz)(BIG = 1196 MHz) */
			mt_ppm_ptpod_policy_activate();
			mt_gpufreq_disable_by_ptpod();
			#if !defined(CONFIG_MTK_CLKMGR)
				ret = clk_prepare_enable(clk_thermal); /* Thermal clock enable */
				if (ret)
					eem_error("clk_prepare_enable failed when enabling THERMAL\n");

				ret = clk_prepare_enable(clk_mfg_scp); /* GPU MTCMOS enable*/
				if (ret)
					eem_error("clk_prepare_enable failed when enabling mfg MTCMOS\n");

				ret = clk_prepare_enable(clk_mfg); /* GPU CLOCK */
				if (ret)
					eem_error("clk_prepare_enable failed when enabling mfg clock\n");
			#endif
		#endif
	#endif

	#if (defined(__KERNEL__) && !defined(EARLY_PORTING))
		/*
		extern unsigned int ckgen_meter(int val);
		eem_debug("@%s(), hf_faxi_ck = %d, hd_faxi_ck = %d\n",
			__func__, ckgen_meter(1), ckgen_meter(2));
		*/
	#endif
	/* set PWM mode for DA9214 */
	da9214_config_interface(0x0, 0x1, 0xF, 0);  /* select to page 2,3 */
	da9214_config_interface(0xD1, 0x2, 0x3, 0); /* Enable Buck A PWM Mode for L/LL/CCI */
	da9214_config_interface(0x0, 0x1, 0xF, 0);  /* select to page 2,3 */
	da9214_config_interface(0xD2, 0x2, 0x3, 0); /* Enable Buck B PWM Mode for BIG */
	/* set PWM mode for FAN5355 */
	fan53555_config_interface(0x00, 0x01, 0x01, 6); /* Set PWM mode for GPU */

	/* for slow idle */
	ptp_data[0] = 0xffffffff;

	for_each_det(det)
		if (EEM_CTRL_SOC != det->ctrl_id)
			eem_init_det(det, &eem_devinfo);

	#ifdef __KERNEL__
		mt_cpufreq_set_ptbl_registerCB(mt_cpufreq_set_ptbl_funcEEM);
		eem_init01();
	#endif
	ptp_data[0] = 0;

	#if (defined(__KERNEL__) && !defined(EARLY_PORTING))
		/*
		unsigned int ckgen_meter(int val);
		eem_debug("@%s(), hf_faxi_ck = %d, hd_faxi_ck = %d\n",
			__func__,
			ckgen_meter(1),
			ckgen_meter(2));
		*/
	register_hotcpu_notifier(&_mt_eem_cpu_notifier);
	#endif
	eem_debug("eem_probe ok\n");
	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static int eem_suspend(struct platform_device *pdev, pm_message_t state)
{
	/*
	kthread_stop(eem_volt_thread);
	*/
	FUNC_ENTER(FUNC_LV_MODULE);
	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static int eem_resume(struct platform_device *pdev)
{
	/*
	eem_volt_thread = kthread_run(eem_volt_thread_handler, 0, "eem volt");
	if (IS_ERR(eem_volt_thread))
	{
	    eem_debug("[%s]: failed to create eem volt thread\n", __func__);
	}
	*/
	FUNC_ENTER(FUNC_LV_MODULE);
	mt_cpufreq_eem_resume();
	eem_init02();
	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mt_eem_of_match[] = {
	{ .compatible = "mediatek,mt6797-eem_fsm", },
	{},
};
#endif

static struct platform_driver eem_driver = {
	.remove     = NULL,
	.shutdown   = NULL,
	.probe      = eem_probe,
	.suspend    = eem_suspend,
	.resume     = eem_resume,
	.driver     = {
		.name   = "mt-eem",
#ifdef CONFIG_OF
		.of_match_table = mt_eem_of_match,
#endif
	},
};

#ifndef __KERNEL__
/*
 * For CTP
 */
int no_efuse;

int can_not_read_efuse(void)
{
	return no_efuse;
}

int mt_ptp_probe(void)
{
	eem_debug("CTP - In mt_eem_probe\n");
	no_efuse = eem_init();

#if defined(__MTK_SLT_)
	if (no_efuse == -1)
		return 0;
#endif
	eem_probe(NULL);
	return 0;
}

void eem_init01_ctp(unsigned int id)
{
	struct eem_det *det;
	struct eem_ctrl *ctrl;
	unsigned int out = 0, timeout = 0;

	FUNC_ENTER(FUNC_LV_LOCAL);

	for_each_det_ctrl(det, ctrl) {
		if (HAS_FEATURE(det, FEA_INIT01)) {
			unsigned long flag;

			eem_debug("CTP - (%d:%d)\n",
				det->ops->volt_2_eem(det, det->ops->get_volt(det)),
				det->VBOOT);

			mt_ptp_lock(&flag);
			det->ops->init01(det);
			mt_ptp_unlock(&flag);
		}
	}

	while (1) {
		for_each_det_ctrl(det, ctrl) {
			if ((EEM_CTRL_BIG == det->ctrl_id) && (1 == det->eem_eemEn[EEM_PHASE_INIT01]))
				out |= BIT(EEM_CTRL_BIG);
			else if ((EEM_CTRL_GPU == det->ctrl_id) && (1 == det->eem_eemEn[EEM_PHASE_INIT01]))
				out |= BIT(EEM_CTRL_GPU);
			else if ((EEM_CTRL_L == det->ctrl_id) && (1 == det->eem_eemEn[EEM_PHASE_INIT01]))
				out |= BIT(EEM_CTRL_L);
			else if ((EEM_CTRL_2L == det->ctrl_id) && (1 == det->eem_eemEn[EEM_PHASE_INIT01]))
				out |= BIT(EEM_CTRL_2L);
			else if ((EEM_CTRL_CCI == det->ctrl_id) && (1 == det->eem_eemEn[EEM_PHASE_INIT01]))
				out |= BIT(EEM_CTRL_CCI);
		}
		if ((0x3B == out) || (1000 == timeout)) {
			eem_error("init01 finish time is %d\n", timeout);
			break;
		}
		udelay(100);
		timeout++;
	}

	thermal_init();
	udelay(500);
	eem_init02();

	FUNC_EXIT(FUNC_LV_LOCAL);
}

void ptp_init01_ptp(int id)
{
	eem_init01_ctp(id);
}

#endif

#ifdef CONFIG_PROC_FS
int mt_eem_opp_num(enum eem_det_id id)
{
	struct eem_det *det = id_to_eem_det(id);

	FUNC_ENTER(FUNC_LV_API);
	FUNC_EXIT(FUNC_LV_API);

	return det->num_freq_tbl;
}
EXPORT_SYMBOL(mt_eem_opp_num);

void mt_eem_opp_freq(enum eem_det_id id, unsigned int *freq)
{
	struct eem_det *det = id_to_eem_det(id);
	int i = 0;

	FUNC_ENTER(FUNC_LV_API);

	for (i = 0; i < det->num_freq_tbl; i++)
		freq[i] = det->freq_tbl[i];

	FUNC_EXIT(FUNC_LV_API);
}
EXPORT_SYMBOL(mt_eem_opp_freq);

void mt_eem_opp_status(enum eem_det_id id, unsigned int *temp, unsigned int *volt)
{
	struct eem_det *det = id_to_eem_det(id);
	int i = 0;

	FUNC_ENTER(FUNC_LV_API);

#if defined(__KERNEL__) && defined(CONFIG_THERMAL) && !defined(EARLY_PORTING)
	*temp = tscpu_get_temp_by_bank(id);
#else
	*temp = 0;
#endif
	for (i = 0; i < det->num_freq_tbl; i++)
		volt[i] = det->ops->pmic_2_volt(det, det->volt_tbl_pmic[i]);

	FUNC_EXIT(FUNC_LV_API);
}
EXPORT_SYMBOL(mt_eem_opp_status);

/***************************
* return current EEM stauts
****************************/
int mt_eem_status(enum eem_det_id id)
{
	struct eem_det *det = id_to_eem_det(id);

	FUNC_ENTER(FUNC_LV_API);

	BUG_ON(!det);
	BUG_ON(!det->ops);
	BUG_ON(!det->ops->get_status);

	FUNC_EXIT(FUNC_LV_API);

	return det->ops->get_status(det);
}

/**
 * ===============================================
 * PROCFS interface for debugging
 * ===============================================
 */

/*
 * show current EEM stauts
 */
static int eem_debug_proc_show(struct seq_file *m, void *v)
{
	struct eem_det *det = (struct eem_det *)m->private;

	FUNC_ENTER(FUNC_LV_HELP);

	/* FIXME: EEMEN sometimes is disabled temp */
	seq_printf(m, "[%s] %s (%d)\n",
		   ((char *)(det->name) + 8),
		   det->disabled ? "disabled" : "enable",
		   det->ops->get_status(det)
		   );

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

/*
 * set EEM status by procfs interface
 */
static ssize_t eem_debug_proc_write(struct file *file,
				    const char __user *buffer, size_t count, loff_t *pos)
{
	int ret;
	int enabled = 0;
	char *buf = (char *) __get_free_page(GFP_USER);
	struct eem_det *det = (struct eem_det *)PDE_DATA(file_inode(file));

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (!kstrtoint(buf, 10, &enabled)) {
		ret = 0;

		if (0 == enabled)
			/* det->ops->enable(det, BY_PROCFS); */
			det->ops->disable(det, 0);
		else if (1 == enabled)
			det->ops->disable(det, BY_PROCFS);
		else if (2 == enabled)
			det->ops->disable(det, BY_PROCFS_INIT2);
	} else
		ret = -EINVAL;

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}

/*
 * show current EEM data
 */
static int eem_dump_proc_show(struct seq_file *m, void *v)
{
	struct eem_det *det;
	int *val = (int *)&eem_devinfo;
	int i, k;
	#if DUMP_DATA_TO_DE
		int j;
	#endif

	FUNC_ENTER(FUNC_LV_HELP);
	/*
	eem_detectors[EEM_DET_BIG].ops->dump_status(&eem_detectors[EEM_DET_BIG]);
	eem_detectors[EEM_DET_L].ops->dump_status(&eem_detectors[EEM_DET_L]);
	seq_printf(m, "det->EEMMONEN= 0x%08X,det->EEMINITEN= 0x%08X\n", det->EEMMONEN, det->EEMINITEN);
	seq_printf(m, "leakage_core\t= %d\n"
			"leakage_gpu\t= %d\n"
			"leakage_little\t= %d\n"
			"leakage_big\t= %d\n",
			leakage_core,
			leakage_gpu,
			leakage_sram2,
			leakage_sram1
			);
	*/


	for (i = 0; i < sizeof(struct eem_devinfo)/sizeof(unsigned int); i++)
		seq_printf(m, "M_HW_RES%d\t= 0x%08X\n", i, val[i]);

	for_each_det(det) {
		if (EEM_CTRL_SOC ==  det->ctrl_id)
			continue;
		for (i = EEM_PHASE_INIT01; i < NR_EEM_PHASE; i++) {
			seq_printf(m, "Bank_number = %d\n", det->ctrl_id);
			if (i < EEM_PHASE_MON)
				seq_printf(m, "mode = init%d\n", i+1);
			else
				seq_puts(m, "mode = mon\n");
			if (eem_log_en) {
				seq_printf(m, "0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
					det->dcvalues[i],
					det->freqpct30[i],
					det->eem_26c[i],
					det->vop30[i],
					det->eem_eemEn[i]
				);

				if (det->eem_eemEn[i] == 0x5) {
					seq_printf(m, "EEM_LOG: Bank_number = [%d] (%d) - (",
					det->ctrl_id, det->ops->get_temp(det));

					for (k = 0; k < det->num_freq_tbl - 1; k++)
						seq_printf(m, "%d, ",
						det->ops->pmic_2_volt(det, det->volt_tbl_pmic[k]));
					seq_printf(m, "%d) - (", det->ops->pmic_2_volt(det, det->volt_tbl_pmic[k]));

					for (k = 0; k < det->num_freq_tbl - 1; k++)
						seq_printf(m, "%d, ", det->freq_tbl[k]);
					seq_printf(m, "%d)\n", det->freq_tbl[k]);
				}
			}
			#if DUMP_DATA_TO_DE
			for (j = 0; j < ARRAY_SIZE(reg_dump_addr_off); j++)
				seq_printf(m, "0x%08lx = 0x%08x\n",
					(unsigned long)EEM_BASEADDR + reg_dump_addr_off[j],
					det->reg_dump_data[j][i]
					);
			#endif
		}
	}
	FUNC_EXIT(FUNC_LV_HELP);
	return 0;
}

/*
 * show current voltage
 */
static int eem_cur_volt_proc_show(struct seq_file *m, void *v)
{
	struct eem_det *det = (struct eem_det *)m->private;
	u32 rdata = 0, i;

	FUNC_ENTER(FUNC_LV_HELP);

	rdata = det->ops->get_volt(det);

	if (rdata != 0)
		seq_printf(m, "%d\n", rdata);
	else
		seq_printf(m, "EEM[%s] read current voltage fail\n", det->name);

	if ((EEM_CTRL_GPU != det->ctrl_id) && (EEM_CTRL_SOC != det->ctrl_id)) {
		for (i = 0; i < det->num_freq_tbl; i++)
			seq_printf(m, "EEM_HW, det->volt_tbl[%d] = [%x], det->volt_tbl_pmic[%d] = [%x]\n",
			i, det->volt_tbl[i], i, det->volt_tbl_pmic[i]);

		for (i = 0; i < NR_FREQ; i++) {
			seq_printf(m, "(iDVFS, 0x%x)(Vs, 0x%x) (Vp, 0x%x, %d) (F_Setting)(%x, %x, %x, %x, %x)\n",
				(det->recordRef[i*2] >> 14) & 0x3FFFF,
				(det->recordRef[i*2] >> 7) & 0x7F,
				det->recordRef[i*2] & 0x7F,
				det->ops->pmic_2_volt(det, (det->recordRef[i*2] & 0x7F)),
				(det->recordRef[i*2+1] >> 21) & 0x1F,
				(det->recordRef[i*2+1] >> 12) & 0x1FF,
				(det->recordRef[i*2+1] >> 7) & 0x1F,
				(det->recordRef[i*2+1] >> 4) & 0x7,
				det->recordRef[i*2+1] & 0xF
				);
		}
	}
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

/*
 * set secure DVFS by procfs
 */
static ssize_t eem_cur_volt_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{
	int ret;
	char *buf = (char *) __get_free_page(GFP_USER);
	unsigned int voltValue = 0, voltProc = 0, voltSram = 0, voltPmic = 0, index = 0;
	struct eem_det *det = (struct eem_det *)PDE_DATA(file_inode(file));

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	/* if (!kstrtoint(buf, 10, &voltValue)) { */
	if (2 == sscanf(buf, "%u %u", &voltValue, &index)) {
		if ((EEM_CTRL_GPU != det->ctrl_id) && (EEM_CTRL_SOC != det->ctrl_id)) {
			ret = 0;
			det->recordRef[NR_FREQ * 2] = 0x00000000;
			mb(); /* SRAM writing */
			voltPmic = det->ops->volt_2_pmic(det, voltValue);
			if (EEM_CTRL_BIG == det->ctrl_id)
				voltProc = clamp(
				(unsigned int)voltPmic,
				(unsigned int)det->VMIN,
				(unsigned int)det->VMAX);
			else
				voltProc = clamp(
				(unsigned int)voltPmic,
				(unsigned int)(det->ops->eem_2_pmic(det, det->VMIN)),
				(unsigned int)(det->ops->eem_2_pmic(det, det->VMAX)));

			voltPmic = det->ops->volt_2_pmic(det, voltValue + 10000);
			voltSram = clamp(
				(unsigned int)(voltPmic),
				(unsigned int)(det->ops->volt_2_pmic(det, VMIN_SRAM)),
				(unsigned int)(det->ops->volt_2_pmic(det, VMAX_SRAM)));

			/* for (i = 0; i < NR_FREQ; i++) */
			det->recordRef[index*2] = ((PMIC_2_RMIC(det, voltSram) & 0x7F) << 7) | (voltProc & 0x7F);

			det->recordRef[NR_FREQ * 2] = 0xFFFFFFFF;
			mb(); /* SRAM writing */
		}
	} else {
		ret = -EINVAL;
		eem_debug("bad argument_1!! voltage = (80000 ~ 115500), index = (0 ~ 15)\n");
	}

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}

/*
 * show current EEM status
 */
static int eem_status_proc_show(struct seq_file *m, void *v)
{
	int i;
	struct eem_det *det = (struct eem_det *)m->private;

	FUNC_ENTER(FUNC_LV_HELP);

	seq_printf(m, "bank = %d, (%d) - (",
		   det->ctrl_id, det->ops->get_temp(det));
	for (i = 0; i < det->num_freq_tbl - 1; i++)
		seq_printf(m, "%d, ", det->ops->pmic_2_volt(det, det->volt_tbl_pmic[i]));
	seq_printf(m, "%d) - (", det->ops->pmic_2_volt(det, det->volt_tbl_pmic[i]));

	for (i = 0; i < det->num_freq_tbl - 1; i++)
		seq_printf(m, "%d, ", det->freq_tbl[i]);
	seq_printf(m, "%d)\n", det->freq_tbl[i]);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

/*
 * set EEM log enable by procfs interface
 */

static int eem_log_en_proc_show(struct seq_file *m, void *v)
{
	FUNC_ENTER(FUNC_LV_HELP);
	seq_printf(m, "%d\n", eem_log_en);
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static ssize_t eem_log_en_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{
	int ret;
	char *buf = (char *) __get_free_page(GFP_USER);

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	ret = -EINVAL;

	if (kstrtoint(buf, 10, &eem_log_en)) {
		eem_debug("bad argument!! Should be \"0\" or \"1\"\n");
		goto out;
	}

	ret = 0;

	switch (eem_log_en) {
	case 0:
		eem_debug("eem log disabled.\n");
		hrtimer_cancel(&eem_log_timer);
		break;

	case 1:
		eem_debug("eem log enabled.\n");
		hrtimer_start(&eem_log_timer, ns_to_ktime(LOG_INTERVAL), HRTIMER_MODE_REL);
		break;

	default:
		eem_debug("bad argument!! Should be \"0\" or \"1\"\n");
		ret = -EINVAL;
	}

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}


/*
 * show EEM offset
 */
static int eem_offset_proc_show(struct seq_file *m, void *v)
{
	struct eem_det *det = (struct eem_det *)m->private;

	FUNC_ENTER(FUNC_LV_HELP);

	seq_printf(m, "%d\n", det->volt_offset);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

/*
 * set EEM offset by procfs
 */
static ssize_t eem_offset_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{
	int ret;
	char *buf = (char *) __get_free_page(GFP_USER);
	int offset = 0;
	struct eem_det *det = (struct eem_det *)PDE_DATA(file_inode(file));
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (!kstrtoint(buf, 10, &offset)) {
		ret = 0;
		det->volt_offset = offset;
		mt_ptp_lock(&flags);
		eem_set_eem_volt(det);
		mt_ptp_unlock(&flags);
	} else {
		ret = -EINVAL;
		eem_debug("bad argument_1!! argument should be \"0\"\n");
	}

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}

#define PROC_FOPS_RW(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner          = THIS_MODULE,				\
		.open           = name ## _proc_open,			\
		.read           = seq_read,				\
		.llseek         = seq_lseek,				\
		.release        = single_release,			\
		.write          = name ## _proc_write,			\
	}

#define PROC_FOPS_RO(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner          = THIS_MODULE,				\
		.open           = name ## _proc_open,			\
		.read           = seq_read,				\
		.llseek         = seq_lseek,				\
		.release        = single_release,			\
	}

#define PROC_ENTRY(name)	{__stringify(name), &name ## _proc_fops}

PROC_FOPS_RW(eem_debug);
PROC_FOPS_RO(eem_status);
PROC_FOPS_RW(eem_cur_volt);
PROC_FOPS_RW(eem_offset);
PROC_FOPS_RO(eem_dump);
PROC_FOPS_RW(eem_log_en);

static int create_procfs(void)
{
	struct proc_dir_entry *eem_dir = NULL;
	struct proc_dir_entry *det_dir = NULL;
	int i;
	struct eem_det *det;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	struct pentry det_entries[] = {
		PROC_ENTRY(eem_debug),
		PROC_ENTRY(eem_status),
		PROC_ENTRY(eem_cur_volt),
		PROC_ENTRY(eem_offset),
	};

	struct pentry eem_entries[] = {
		PROC_ENTRY(eem_dump),
		PROC_ENTRY(eem_log_en),
	};

	FUNC_ENTER(FUNC_LV_HELP);

	eem_dir = proc_mkdir("eem", NULL);

	if (!eem_dir) {
		eem_error("[%s]: mkdir /proc/eem failed\n", __func__);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(eem_entries); i++) {
		if (!proc_create(eem_entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, eem_dir, eem_entries[i].fops)) {
			eem_error("[%s]: create /proc/eem/%s failed\n", __func__, eem_entries[i].name);
			FUNC_EXIT(FUNC_LV_HELP);
			return -3;
		}
	}

	for_each_det(det) {
		if (EEM_CTRL_SOC ==  det->ctrl_id)
			continue;
		det_dir = proc_mkdir(det->name, eem_dir);

		if (!det_dir) {
			eem_debug("[%s]: mkdir /proc/eem/%s failed\n", __func__, det->name);
			FUNC_EXIT(FUNC_LV_HELP);
			return -2;
		}

		for (i = 0; i < ARRAY_SIZE(det_entries); i++) {
			if (!proc_create_data(
				det_entries[i].name,
				S_IRUGO | S_IWUSR | S_IWGRP,
				det_dir,
				det_entries[i].fops,
				det)) {
					eem_debug("[%s]: create /proc/eem/%s/%s failed\n",
						__func__,
						det->name,
						det_entries[i].name);
					FUNC_EXIT(FUNC_LV_HELP);
					return -3;
			}
		}
	}
	FUNC_EXIT(FUNC_LV_HELP);
	return 0;
}
#endif

#if 0
#define VLTE_BIN0_VMAX (121000)
#define VLTE_BIN0_VMIN (104500)
#define VLTE_BIN0_VNRL (110625)

#define VLTE_BIN1_VMAX (121000)
#define VLTE_BIN1_VMIN (102500)
#define VLTE_BIN1_VNRL (105000)

#define VLTE_BIN2_VMAX (121000)
#define VLTE_BIN2_VMIN (97500)
#define VLTE_BIN2_VNRL (100000)

#define VSOC_BIN0_VMAX (126500)
#define VSOC_BIN0_VMIN (109000)
#define VSOC_BIN0_VNRL (115000)

#define VSOC_BIN1_VMAX (126500)
#define VSOC_BIN1_VMIN (107500)
#define VSOC_BIN1_VNRL (110000)

#define VSOC_BIN2_VMAX (126500)
#define VSOC_BIN2_VMIN (102500)
#define VSOC_BIN2_VNRL (105000)

#if defined(SLT_VMAX)
#define VLTE_BIN(x) VLTE_BIN##x##_VMAX
#define VSOC_BIN(x) VSOC_BIN##x##_VMAX
#elif defined(SLT_VMIN)
#define VLTE_BIN(x) VLTE_BIN##x##_VMIN
#define VSOC_BIN(x) VSOC_BIN##x##_VMIN
#else
#define VLTE_BIN(x) VLTE_BIN##x##_VNRL
#define VSOC_BIN(x) VSOC_BIN##x##_VNRL
#endif

void process_voltage_bin(struct eem_devinfo *devinfo)
{
	switch (devinfo->LTE_VOLTBIN) {
	case 0:
		#ifndef EARLY_PORTING
			#ifdef __KERNEL__
				/* det->volt_tbl_bin[0] = det->ops->volt_2_pmic(det, VLTE_BIN(0)); */
				mt_cpufreq_set_lte_volt(det->ops->volt_2_pmic(det, VLTE_BIN(0)));
			#else
				dvfs_set_vlte(det->ops->volt_2_pmic(det, VLTE_BIN(0)));
			#endif
		#endif
		eem_debug("VLTE voltage bin to %dmV\n", (VLTE_BIN(0)/100));
		break;
	case 1:
		#ifndef EARLY_PORTING
			#ifdef __KERNEL__
				/* det->volt_tbl_bin[0] = det->ops->volt_2_pmic(det, 105000); */
				mt_cpufreq_set_lte_volt(det->ops->volt_2_pmic(det, 105000));
			#else
				dvfs_set_vlte(det->ops->volt_2_pmic(det, VLTE_BIN(1)));
			#endif
		#endif
		eem_debug("VLTE voltage bin to %dmV\n", (VLTE_BIN(1)/100));
		break;
	case 2:
		#ifndef EARLY_PORTING
			#ifdef __KERNEL__
				/* det->volt_tbl_bin[0] = det->ops->volt_2_pmic(det, 100000); */
				mt_cpufreq_set_lte_volt(det->ops->volt_2_pmic(det, 100000));
			#else
				dvfs_set_vlte(det->ops->volt_2_pmic(det, VLTE_BIN(2)));
			#endif
		#endif
		eem_debug("VLTE voltage bin to %dmV\n", (VLTE_BIN(2)/100));
		break;
	default:
		#ifndef EARLY_PORTING
			#ifdef __KERNEL__
				mt_cpufreq_set_lte_volt(det->ops->volt_2_pmic(det, 110625));
			#else
				dvfs_set_vlte(det->ops->volt_2_pmic(det, 110625));
			#endif
		#endif
		eem_debug("VLTE voltage bin to 1.10625V\n");
		break;
	};
	/* mt_cpufreq_set_lte_volt(det->volt_tbl_bin[0]); */

	switch (devinfo->SOC_VOLTBIN) {
	case 0:
#ifdef __MTK_SLT_
		if (!slt_is_low_vcore()) {
			dvfs_set_vcore(det->ops->volt_2_pmic(det, VSOC_BIN(0)));
			eem_debug("VCORE voltage bin to %dmV\n", (VSOC_BIN(0)/100));
		} else {
			dvfs_set_vcore(det->ops->volt_2_pmic(det, 105000));
			eem_debug("VCORE voltage bin to %dmV\n", (105000/100));
		}
#else
		dvfs_set_vcore(det->ops->volt_2_pmic(det, VSOC_BIN(0)));
		eem_debug("VCORE voltage bin to %dmV\n", (VSOC_BIN(0)/100));
#endif
		break;
	case 1:
		dvfs_set_vcore(det->ops->volt_2_pmic(det, VSOC_BIN(1)));
		eem_debug("VCORE voltage bin to %dmV\n", (VSOC_BIN(1)/100));
		break;
	case 2:
		dvfs_set_vcore(det->ops->volt_2_pmic(det, VSOC_BIN(2)));
		eem_debug("VCORE voltage bin to %dmV\n", (VSOC_BIN(2)/100));
		break;
	default:
		dvfs_set_vcore(det->ops->volt_2_pmic(det, 105000));
		eem_debug("VCORE voltage bin to 1.05V\n");
		break;
	};
}
#endif

#define VCORE_VOLT_0 0
#define VCORE_VOLT_1 1
#define VCORE_VOLT_2 2

/* SOC Voltage */
unsigned int DDR1866[4] = {105000, 102500, 100000, 97500};
unsigned int DDR1700[4] = {100000, 97500, 95000, 92500};
unsigned int DDR1270[4] = {90000, 87500, 85000, 82500};
unsigned int DDR1066[4] = {90000, 87500, 85000, 82500};

unsigned int vcore0;
unsigned int vcore1;
unsigned int vcore2;
unsigned int have_550;

int is_have_550(void)
{
	return have_550;
}

unsigned int get_vcore_ptp_volt(int seg)
{
	unsigned int ret;

	switch (seg) {
	case VCORE_VOLT_0:
		ret = vcore0;
		break;

	case VCORE_VOLT_1:
		ret = vcore1;
		break;

	case VCORE_VOLT_2:
		ret = vcore2;
		break;

	default:
		ret = ((1000000 / 10) - 60000 + 625 - 1) / 625;
		break;
	}

	return ret;
}

void eem_set_pi_offset(enum eem_ctrl_id id, int step)
{
	struct eem_det *det = id_to_eem_det(id);

	det->pi_offset = step;

#ifdef CONFIG_EEM_AEE_RR_REC
	aee_rr_rec_eem_pi_offset(step);
#endif
}

unsigned int get_efuse_status(void)
{
	return checkEfuse;
}

unsigned int get_eem_status_for_gpu(void)
{
	return informGpuEEMisReady;
}

void eem_efuse_calibration(struct eem_devinfo *devinfo)
{
	int temp;

	eem_error("%s\n", __func__);

	if (devinfo->GPU_DCBDET >= 128) {
		eem_error("GPU_DCBDET = %d which is correct\n", devinfo->GPU_DCBDET);
	} else {
		isGPUDCBDETOverflow = 1;
		temp = devinfo->GPU_DCBDET;
		eem_error("GPU_DCBDET = 0x%x, (%d), (%d), (%d)\n", devinfo->GPU_DCBDET, temp, temp-256, (temp-256)/2);
		devinfo->GPU_DCBDET = (unsigned char)((temp - 256) / 2);
		eem_error("GPU_DCBDET = 0x%x, (%d)\n",
			devinfo->GPU_DCBDET,
			devinfo->GPU_DCBDET);
	}

	if (devinfo->CPU_2L_DCBDET >= 128) {
		eem_error("CPU_2L_DCBDET = %d which is correct\n", devinfo->CPU_2L_DCBDET);
	} else {
		is2LDCBDETOverflow = 1;
		temp = devinfo->CPU_2L_DCBDET;
		devinfo->CPU_2L_DCBDET = (unsigned char)((temp - 256) / 2);
		eem_error("CPU_2L_DCBDET = 0x%x, (%d)\n",
			devinfo->CPU_2L_DCBDET,
			devinfo->CPU_2L_DCBDET);
	}

	if (devinfo->CPU_L_DCBDET >= 128) {
		eem_error("CPU_L_DCBDET = %d which is correct\n", devinfo->CPU_L_DCBDET);
	} else {
		isLDCBDETOverflow = 1;
		temp = devinfo->CPU_L_DCBDET;
		devinfo->CPU_L_DCBDET = (unsigned char)((temp - 256) / 2);
		eem_error("CPU_L_DCBDET = 0x%x, (%d)\n",
			devinfo->CPU_L_DCBDET,
			devinfo->CPU_L_DCBDET);
	}

	if (devinfo->CCI_DCBDET >= 128) {
		eem_error("CPU_CCI_DCBDET = %d which is correct\n", devinfo->CCI_DCBDET);
	} else {
		isCCIDCBDETOverflow = 1;
		temp = devinfo->CCI_DCBDET;
		devinfo->CCI_DCBDET = (unsigned char)((temp - 256) / 2);
		eem_error("CCI_DCBDET = 0x%x, (%d)\n",
			devinfo->CCI_DCBDET,
			devinfo->CCI_DCBDET);
	}
}

#ifdef __KERNEL__
static int __init dt_get_ptp_devinfo(unsigned long node, const char *uname, int depth, void *data)
{
	#if 0
	struct devinfo_ptp_tag *tags;
	unsigned int size = 0;

	if (depth != 1 || (strcmp(uname, "chosen") != 0 && strcmp(uname, "chosen@0") != 0))
		return 0;

	tags = (struct devinfo_ptp_tag *) of_get_flat_dt_prop(node, "atag,ptp", &size);

	if (tags) {
		vcore0 = tags->volt0;
		vcore1 = tags->volt1;
		vcore2 = tags->volt2;
		have_550 = tags->have_550;
		eem_debug("[EEM][VCORE] - Kernel Got from DT (0x%0X, 0x%0X, 0x%0X, 0x%0X)\n",
			vcore0, vcore1, vcore2, have_550);
	}
	#endif

	unsigned int soc_efuse;

	/* Read EFUSE */
	soc_efuse = get_devinfo_with_index(54);
	eem_error("[VCORE] - Kernel Got efuse 0x%0X\n", soc_efuse);

	/* Read SODI level U, H, L */
	switch (get_sodi_fw_mode()) {
	case SODI_FW_LPM: /* 1066/1270/1600 */
		eem_error("[DDR] segment = 1600\n");
		vcore0 = (DDR1700[((soc_efuse >> 4) & 0x03)] - 60000 + 625 - 1) / 625;
		vcore1 = (DDR1270[((soc_efuse >> 2) & 0x03)] - 60000 + 625 - 1) / 625;
		vcore2 = (DDR1066[(soc_efuse & 0x03)] - 60000 + 625 - 1) / 625;
		break;

	case SODI_FW_HPM: /* 1066/1270/1700 */
		eem_error("[DDR] segment = 1700\n");
		vcore0 = (DDR1700[((soc_efuse >> 4) & 0x03)] - 60000 + 625 - 1) / 625;
		vcore1 = (DDR1270[((soc_efuse >> 2) & 0x03)] - 60000 + 625 - 1) / 625;
		vcore2 = (DDR1066[(soc_efuse & 0x03)] - 60000 + 625 - 1) / 625;
		break;

	case SODI_FW_ULTRA: /* 1066/1270/1866 */
		eem_error("[DDR] segment = 1866\n");
		vcore0 = (DDR1866[((soc_efuse >> 6) & 0x03)] - 60000 + 625 - 1) / 625;
		vcore1 = (DDR1270[((soc_efuse >> 2) & 0x03)] - 60000 + 625 - 1) / 625;
		vcore2 = (DDR1066[(soc_efuse & 0x03)] - 60000 + 625 - 1) / 625;
		break;

	default:
		break;
	}

	vcore1 = (vcore1 < vcore2) ? vcore2 : vcore1;
	vcore0 = (vcore0 < vcore1) ? vcore1 : vcore0;

	have_550 = 0;
	eem_error("[EEM][VCORE] - Kernel Got from DT (0x%0X, 0x%0X, 0x%0X, 0x%0X)\n",
			vcore0, vcore1, vcore2, have_550);

	return 1;
}

static int __init vcore_ptp_init(void)
{
	of_scan_flat_dt(dt_get_ptp_devinfo, NULL);

	return 0;
}

/*
 * Module driver
 */

static int __init eem_conf(void)
{
	int i;
	unsigned int binLevel;
	struct device_node *node = of_find_compatible_node(NULL, "cpu", "arm,cortex-a72");

	cpu_speed = 0;

	recordRef = ioremap_nocache(EEMCONF_S, EEMCONF_SIZE);
	eem_debug("@(Record)%s----->(%p)\n", __func__, recordRef);
	memset_io((u8 *)recordRef, 0x00, EEMCONF_SIZE);
	if (!recordRef)
		return -ENOMEM;

	/* read E-fuse for segment selection */
	binLevel = GET_BITS_VAL(3:0, get_devinfo_with_index(22));

	/* get CPU clock-frequency from DT */
	if (!of_property_read_u32(node, "clock-frequency", &cpu_speed)) {
		cpu_speed = cpu_speed / 1000 / 1000; /* MHz */
		eem_error("CPU clock-frequency from DT = %d MHz\n", cpu_speed);
		if (cpu_speed == 1989) /* M */
			binLevel = 2;
	} else {
		eem_error("@%s: missing clock-frequency property, use default CPU level\n", __func__);
		binLevel = 0;
	}

	if (binLevel == 0) {
		gpuTbl = &gpuFy[0];
		recordTbl = &fyTbl[0][0];
		eem_error("@The table ----->(fyTbl)\n");
	} else if (binLevel == 1) {
		gpuTbl = &gpuSb[0];
		recordTbl = &sbTbl[0][0];
		eem_error("@The table ----->(sbTbl)\n");
	} else {
		gpuTbl = &gpuMb[0];
		recordTbl = &mbTbl[0][0];
		eem_error("@The table ----->(mbTbl)\n");
	}

	for (i = 0; i < NR_FREQ; i++) {
		/* LL
		[31:14] = iDVFS % (/2), [13:7] = Vsram pmic value, [6:0] = Vproc pmic value
		*/
		recordRef[i * 2] =
			((*(recordTbl + (i * 8) + 5) & 0x3FFFF) << 14) |
			((*(recordTbl + (i * 8) + 6) & 0x7F) << 7) |
			(*(recordTbl + (i * 8) + 7) & 0x7F);

		/* [25:21] = dcmdiv, [20:12] = DDS, [11:7] = clkdiv, [6:4] = postdiv, [3:0] = CFindex */
		recordRef[i * 2 + 1] =
			((*(recordTbl + (i * 8) + 0) & 0x1F) << 21) |
			((*(recordTbl + (i * 8) + 1) & 0x1FF) << 12) |
			((*(recordTbl + (i * 8) + 2) & 0x1F) << 7) |
			((*(recordTbl + (i * 8) + 3) & 0x7) << 4) |
			(*(recordTbl + (i * 8) + 4) & 0xF);

		/* L
		[31:14] = iDVFS % (/2), [13:7] = Vsram pmic value, [6:0] = Vproc pmic value
		*/
		recordRef[i * 2 + 36] =
			((*(recordTbl + ((i + 16) * 8) + 5) & 0x3FFFF) << 14) |
			((*(recordTbl + ((i + 16) * 8) + 6) & 0x7F) << 7) |
			(*(recordTbl + ((i + 16) * 8) + 7) & 0x7F);

		/* [25:21] = dcmdiv, [20:12] = DDS, [11:7] = clkdiv, [6:4] = postdiv, [3:0] = CFindex */
		recordRef[i * 2 + 36 + 1] =
			((*(recordTbl + ((i + 16) * 8) + 0) & 0x1F) << 21) |
			((*(recordTbl + ((i + 16) * 8) + 1) & 0x1FF) << 12) |
			((*(recordTbl + ((i + 16) * 8) + 2) & 0x1F) << 7) |
			((*(recordTbl + ((i + 16) * 8) + 3) & 0x7) << 4) |
			(*(recordTbl + ((i + 16) * 8) + 4) & 0xF);

		/* CCI
		[31:14] = iDVFS % (/2), [13:7] = Vsram pmic value, [6:0] = Vproc pmic value
		*/
		recordRef[i * 2 + 72] =
			((*(recordTbl + ((i + 32) * 8) + 5) & 0x3FFFF) << 14) |
			((*(recordTbl + ((i + 32) * 8) + 6) & 0x7F) << 7) |
			(*(recordTbl + ((i + 32) * 8) + 7) & 0x7F);

		/* [25:21] = dcmdiv, [20:12] = DDS, [11:7] = clkdiv, [6:4] = postdiv, [3:0] = CFindex */
		recordRef[i * 2 + 72 + 1] =
			((*(recordTbl + ((i + 32) * 8) + 0) & 0x1F) << 21) |
			((*(recordTbl + ((i + 32) * 8) + 1) & 0x1FF) << 12) |
			((*(recordTbl + ((i + 32) * 8) + 2) & 0x1F) << 7) |
			((*(recordTbl + ((i + 32) * 8) + 3) & 0x7) << 4) |
			(*(recordTbl + ((i + 32) * 8) + 4) & 0xF);

		/* BIG
		[31:14] = iDVFS % (/2), [13:7] = Vsram pmic value, [6:0] = Vproc pmic value
		*/
		recordRef[i * 2 + 108] =
			((*(recordTbl + ((i + 48) * 8) + 5) & 0x3FFFF) << 14) |
			((*(recordTbl + ((i + 48) * 8) + 6) & 0x7F) << 7) |
			(*(recordTbl + ((i + 48) * 8) + 7) & 0x7F);

		/* [25:21] = dcmdiv, [20:12] = DDS, [11:7] = clkdiv, [6:4] = postdiv, [3:0] = CFindex */
		recordRef[i * 2 + 108 + 1] =
					((*(recordTbl + ((i + 48) * 8) + 0) & 0x1F) << 21) |
					((*(recordTbl + ((i + 48) * 8) + 1) & 0x1FF) << 12) |
					((*(recordTbl + ((i + 48) * 8) + 2) & 0x1F) << 7) |
					((*(recordTbl + ((i + 48) * 8) + 3) & 0x7) << 4) |
					(*(recordTbl + ((i + 48) * 8) + 4) & 0xF);
	}
	recordRef[i*2] = 0xffffffff;
	recordRef[i*2+36] = 0xffffffff;
	recordRef[i*2+72] = 0xffffffff;
	recordRef[i*2+108] = 0xffffffff;
	mb(); /* SRAM writing */
	for (i = 0; i < NR_FREQ; i++)
		eem_debug("LL (iDVFS, 0x%x), (Vs, 0x%x), (Vp, 0x%x), (F_Setting)(%x, %x, %x, %x, %x)\n",
			((*(recordRef + (i * 2))) >> 14) & 0x3FFFF,
			((*(recordRef + (i * 2))) >> 7) & 0x7F,
			(*(recordRef + (i * 2))) & 0x7F,
			((*(recordRef + (i * 2) + 1)) >> 21) & 0x1F,
			((*(recordRef + (i * 2) + 1)) >> 12) & 0x1FF,
			((*(recordRef + (i * 2) + 1)) >> 7) & 0x1F,
			((*(recordRef + (i * 2) + 1)) >> 4) & 0x7,
			((*(recordRef + (i * 2) + 1)) & 0xF)
			);

	for (i = 0; i < NR_FREQ; i++)
		eem_debug("L (iDVFS, 0x%x), (Vs, 0x%x), (Vp, 0x%x), (F_Setting)(%x, %x, %x, %x, %x)\n",
			((*(recordRef + 36 + (i * 2))) >> 14) & 0x3FFFF,
			((*(recordRef + 36 + (i * 2))) >> 7) & 0x7F,
			(*(recordRef + 36 + (i * 2))) & 0x7F,
			((*(recordRef + 36 + (i * 2) + 1)) >> 21) & 0x1F,
			((*(recordRef + 36 + (i * 2) + 1)) >> 12) & 0x1FF,
			((*(recordRef + 36 + (i * 2) + 1)) >> 7) & 0x1F,
			((*(recordRef + 36 + (i * 2) + 1)) >> 4) & 0x7,
			((*(recordRef + 36 + (i * 2) + 1)) & 0xF)
			);

	for (i = 0; i < NR_FREQ; i++)
		eem_debug("CCI (iDVFS, 0x%x), (Vs, 0x%x), (Vp, 0x%x), (F_Setting)(%x, %x, %x, %x, %x)\n",
			((*(recordRef + 72 + (i * 2))) >> 14) & 0x3FFFF,
			((*(recordRef + 72 + (i * 2))) >> 7) & 0x7F,
			(*(recordRef + 72 + (i * 2))) & 0x7F,
			((*(recordRef + 72 + (i * 2) + 1)) >> 21) & 0x1F,
			((*(recordRef + 72 + (i * 2) + 1)) >> 12) & 0x1FF,
			((*(recordRef + 72 + (i * 2) + 1)) >> 7) & 0x1F,
			((*(recordRef + 72 + (i * 2) + 1)) >> 4) & 0x7,
			((*(recordRef + 72 + (i * 2) + 1)) & 0xF)
			);

	for (i = 0; i < NR_FREQ; i++)
		eem_debug("BIG (iDVFS, 0x%x), (Vs, 0x%x) (Vp, 0x%x) (F_Setting)(%x, %x, %x, %x, %x)\n",
			((*(recordRef + 108 + (i * 2))) >> 14) & 0x3FFFF,
			((*(recordRef + 108 + (i * 2))) >> 7) & 0x7F,
			(*(recordRef + 108 + (i * 2))) & 0x7F,
			((*(recordRef + 108 + (i * 2) + 1)) >> 21) & 0x1F,
			((*(recordRef + 108 + (i * 2) + 1)) >> 12) & 0x1FF,
			((*(recordRef + 108 + (i * 2) + 1)) >> 7) & 0x1F,
			((*(recordRef + 108 + (i * 2) + 1)) >> 4) & 0x7,
			((*(recordRef + 108 + (i * 2) + 1)) & 0x1F)
			);
	return 0;
}

static int new_eem_val = 1; /* default no change */
static int  __init fn_change_eem_status(char *str)
{
	int new_set_val;

	eem_debug("fn_change_eem_status\n");
	if (get_option(&str, &new_set_val)) {
		new_eem_val = new_set_val;
		eem_debug("new_eem_val = %d\n", new_eem_val);
		return 0;
	}
	return -EINVAL;
}
early_param("eemen", fn_change_eem_status);

static int __init eem_init(void)
#else
int __init eem_init(void)
#endif
{
	int err = 0;
#ifdef __KERNEL__
	struct device_node *node = NULL;
#endif
	FUNC_ENTER(FUNC_LV_MODULE);

#ifdef __KERNEL__
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6797-eem_fsm");

	if (node) {
		/* Setup IO addresses */
		eem_base = of_iomap(node, 0);
		eem_debug("[EEM] eem_base = 0x%p\n", eem_base);
	}

	/*get eem irq num*/
	eem_irq_number = irq_of_parse_and_map(node, 0);
	eem_debug("[THERM_CTRL] eem_irq_number=%d\n", eem_irq_number);
	if (!eem_irq_number) {
		eem_debug("[EEM] get irqnr failed=0x%x\n", eem_irq_number);
		return 0;
	}
	eem_debug("[EEM] new_eem_val=%d, ctrl_EEM_Enable=%d\n", new_eem_val, ctrl_EEM_Enable);
#endif

	get_devinfo(&eem_devinfo);
	eem_efuse_calibration(&eem_devinfo);
#if 0 /* def __KERNEL__ */
	if (new_eem_val == 0) {
		ctrl_EEM_Enable = 0;
		eem_debug("Disable EEM by kernel config\n");
	}
#endif

	/* process_voltage_bin(&eem_devinfo); */ /* LTE voltage bin use I-Chang */
	if (0 == ctrl_EEM_Enable) {
		informGpuEEMisReady = 1;
		eem_error("ctrl_EEM_Enable = 0x%X\n", ctrl_EEM_Enable);
		FUNC_EXIT(FUNC_LV_MODULE);
		return 0;
	}

#ifdef __KERNEL__
	/*
	 * init timer for log / volt
	 */
	hrtimer_init(&eem_log_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	eem_log_timer.function = eem_log_timer_func;

	create_procfs();
#endif

	/*
	 * reg platform device driver
	 */
	err = platform_driver_register(&eem_driver);

	if (err) {
		eem_debug("EEM driver callback register failed..\n");
		FUNC_EXIT(FUNC_LV_MODULE);
		return err;
	}

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static void __exit eem_exit(void)
{
	FUNC_ENTER(FUNC_LV_MODULE);
	eem_debug("eem de-initialization\n");
	FUNC_EXIT(FUNC_LV_MODULE);
}

#ifdef __KERNEL__
module_init(eem_conf);
arch_initcall(vcore_ptp_init); /* I-Chang */
late_initcall(eem_init);
#endif
#endif

MODULE_DESCRIPTION("MediaTek EEM Driver v0.3");
MODULE_LICENSE("GPL");
#ifdef EARLY_PORTING
	#undef EARLY_PORTING
#endif
#undef __MT_EEM_C__

