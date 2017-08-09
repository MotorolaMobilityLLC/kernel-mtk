#ifndef __MT_PARTITIONINFO_H__
#define __MT_PARTITIONINFO_H__

/* #define PT_ABTC_ATAG */
/* #define ATAG_OTP_INFO       0x54430004 */
/* #define ATAG_BMT_INFO       0x54430005 */

struct tag_pt_info {
	unsigned long long size;	/* partition size */
	unsigned long long start_address;	/* partition start */
};
#endif
