#ifndef __M4U_PLATFORM_D3_H__
#define __M4U_PLATFORM_D3_H__

#define TOTAL_M4U_NUM           1

#define M4U_BASE0                0xf0205000

#define LARB0_BASE	0xf4015000
#define LARB1_BASE	0xf6010000
#define LARB2_BASE	0xf5001000
#define LARB3_BASE	0xf7002000

/* mau related */
#define MAU_NR_PER_M4U_SLAVE    4

/* smi */
#define SMI_LARB_NR     4

/* seq range related */
#define SEQ_NR_PER_MM_SLAVE    8
#define SEQ_NR_PER_PERI_SLAVE    0

#define TF_PROTECT_BUFFER_SIZE 128L

#define MMU_PFH_DIST_NR 7
#define MMU_PFH_DIR_NR 2

#define DOMAIN_VALUE 0x44444444

#endif
