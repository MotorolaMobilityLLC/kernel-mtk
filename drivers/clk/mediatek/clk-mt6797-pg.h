#ifndef __DRV_CLK_MT6755_PG_H
#define __DRV_CLK_MT6755_PG_H

enum subsys_id {
	SYS_MD1 = 0,
	SYS_CONN = 1,
	SYS_DPY = 2,
	SYS_DIS = 3,
	SYS_DPY_CH1 = 4,
	SYS_ISP = 5,
	SYS_IFR = 6,
	SYS_VDE = 7,
	SYS_MFG_CORE3 = 8,
	SYS_MFG_CORE2 = 9,
	SYS_MFG_CORE1 = 10,
	SYS_MFG_CORE0 = 11,
	SYS_MFG = 12,
	SYS_MFG_ASYNC = 13,
	SYS_MJC = 14,
	SYS_VEN = 15,
	SYS_AUDIO = 16,
	SYS_C2K = 17,
	NR_SYSS = 18,
};

struct pg_callbacks {
	void (*before_off)(enum subsys_id sys);
	void (*after_on)(enum subsys_id sys);
};

/* register new pg_callbacks and return previous pg_callbacks. */
extern struct pg_callbacks *register_pg_callback(struct pg_callbacks *pgcb);

#endif				/* __DRV_CLK_MT6755_PG_H */
