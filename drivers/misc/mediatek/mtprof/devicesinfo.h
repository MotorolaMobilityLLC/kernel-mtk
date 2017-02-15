#ifndef _DEVICESINFO_H_
#define _DEVICESINFO_H_

struct dram_info {
	const char *vendor;
	const char *flash_name;
	const char *size;
	const char *type;
};

struct emmc_info {
	const char *vendor;
	const char *flash_name;
	const char *size;
	const char *type;
	const unsigned int mid;/*manufacturer ID*/
	const unsigned int pnm;/*Product name*/
};

extern char *saved_command_line;
extern unsigned int emmc_raw_cid[];
#endif
