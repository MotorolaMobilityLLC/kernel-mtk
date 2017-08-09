#ifndef __DRV_CLK_MT6755_PG_H
#define __DRV_CLK_MT6755_PG_H

enum subsys_id {
	SYS_MD1 = 0,
	SYS_MD2 = 1,
	SYS_CONN = 2,
	SYS_DIS = 3,
	SYS_MFG = 4,
	SYS_ISP = 5,
	SYS_VDE = 6,
	SYS_VEN = 7,
	SYS_AUD = 8,
	NR_SYSS = 9,
};

struct pg_callbacks {
	void (*before_off)(enum subsys_id sys);
	void (*after_on)(enum subsys_id sys);
};

/* register new pg_callbacks and return previous pg_callbacks. */
extern struct pg_callbacks *register_pg_callback(struct pg_callbacks *pgcb);
extern int spm_topaxi_protect(unsigned int mask_value, int en);
#endif /* __DRV_CLK_MT6755_PG_H */
