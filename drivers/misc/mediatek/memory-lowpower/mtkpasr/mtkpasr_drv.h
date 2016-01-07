#ifndef _MTKPASR_DRV_H_
#define _MTKPASR_DRV_H_

#define IN_RANGE(s, e, rs, re)	(s >= rs && e <= re)

/*-- Data structures */

/* Bank information (1 PASR unit) */
struct mtkpasr_bank {
	unsigned long start_pfn;	/* The 1st pfn */
	unsigned long end_pfn;		/* The pfn after the last valid one */
	unsigned long free;		/* The number of free pages */
	int segment;			/* Corresponding to which segment */
	int rank;			/* Associated rank */
};

/* MTKPASR internal functions */
extern int __init mtkpasr_init_range(unsigned long start_pfn, unsigned long end_pfn);

/* Give bank, this function will return its (start_pfn, end_pfn) and corresponding rank */
extern int __init query_bank_rank_information(int bank, unsigned long *spfn, unsigned long *epfn, int *segn);

/* The number of pages in one PASR bank */
extern unsigned long bank_pfns;
#endif
