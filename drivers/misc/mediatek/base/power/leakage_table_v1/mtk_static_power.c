/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/export.h>
#include <linux/module.h>

#include "mtk_spower_data.h"
#include "mtk_static_power.h"

#define SP_TAG     "[Power/spower] "
#define SPOWER_LOG_NONE		0
#define SPOWER_LOG_WITH_PRINTK	1

/* #define SPOWER_LOG_PRINT SPOWER_LOG_WITH_PRINTK */
#define SPOWER_LOG_PRINT SPOWER_LOG_NONE

#if (SPOWER_LOG_PRINT == SPOWER_LOG_NONE)
#define SPOWER_INFO(fmt, args...)
#elif (SPOWER_LOG_PRINT == SPOWER_LOG_WITH_PRINTK)
#define SPOWER_INFO(fmt, args...)	 pr_emerg(SP_TAG fmt, ##args)
#endif

static struct sptbl_t sptab[MTK_SPOWER_MAX];
static int mtk_efuse_leakage[MTK_LEAKAGE_MAX];

int interpolate(int x1, int x2, int x3, int y1, int y2)
{
	if (x1 == x2)
		return (y1+y2)/2;

	return (x3-x1) * (y2-y1) / (x2 - x1) + y1;
}

int interpolate_2d(struct sptbl_t *tab, int v1, int v2, int t1, int t2, int voltage, int degree)
{
	int c1, c2, p1, p2, p;

	if ((v1 == v2) && (t1 == t2)) {
		p = mA(tab, v1, t1);
	} else if (v1 == v2) {
		c1 = mA(tab, v1, t1);
		c2 = mA(tab, v1, t2);
		p = interpolate(deg(tab, t1), deg(tab, t2), degree, c1, c2);
	} else if (t1 == t2) {
		c1 = mA(tab, v1, t1);
		c2 = mA(tab, v2, t1);
		p = interpolate(mV(tab, v1), mV(tab, v2), voltage, c1, c2);
	} else {
		c1 = mA(tab, v1, t1);
		c2 = mA(tab, v1, t2);
		p1 = interpolate(deg(tab, t1), deg(tab, t2), degree, c1, c2);

		c1 = mA(tab, v2, t1);
		c2 = mA(tab, v2, t2);
		p2 = interpolate(deg(tab, t1), deg(tab, t2), degree, c1, c2);

		p = interpolate(mV(tab, v1), mV(tab, v2), voltage, p1, p2);
	}

	return p;
}

void interpolate_table(struct sptbl_t *spt, int c1, int c2, int c3, struct sptbl_t *tab1, struct sptbl_t *tab2)
{
	int v, t;

	/* avoid divid error, if we have bad raw data table */
	if (unlikely(c1 == c2)) {
		*spt = *tab1;
		SPOWER_INFO("sptab equal to tab1:%d/%d\n",  c1, c3);
	} else {
		SPOWER_INFO("make sptab %d, %d, %d\n", c1, c2, c3);
		for (t = 0; t < tsize(spt); t++) {
			for (v = 0; v < vsize(spt); v++) {
				int *p = &mA(spt, v, t);

				p[0] = interpolate(c1, c2, c3,
					   mA(tab1, v, t),
					   mA(tab2, v, t));

				SPOWER_INFO("%d ", p[0]);
			}
			SPOWER_INFO("\n");
		}
		SPOWER_INFO("make sptab done!\n");
	}
}


int sptab_lookup(struct sptbl_t *tab, int voltage, int degree)
{
	int x1, x2, y1, y2, i;
	int mamper;

	/* lookup voltage */
	for (i = 0; i < vsize(tab); i++) {
		if (voltage <= mV(tab, i))
			break;
	}

	if (unlikely(voltage == mV(tab, i))) {
		x1 = x2 = i;
	} else if (unlikely(i == vsize(tab))) {
		x1 = vsize(tab)-2;
		x2 = vsize(tab)-1;
	} else if (i == 0) {
		x1 = 0;
		x2 = 1;
	} else {
		x1 = i-1;
		x2 = i;
	}


	/* lookup degree */
	for (i = 0; i < tsize(tab); i++) {
		if (degree <= deg(tab, i))
			break;
	}

	if (unlikely(degree == deg(tab, i))) {
		y1 = y2 = i;
	} else if (unlikely(i == tsize(tab))) {
		y1 = tsize(tab)-2;
		y2 = tsize(tab)-1;
	} else if (i == 0) {
		y1 = 0;
		y2 = 1;
	} else {
		y1 = i-1;
		y2 = i;
	}

	mamper = interpolate_2d(tab, x1, x2, y1, y2, voltage, degree);

	/*
	 * SPOWER_INFO("x1=%d, x2=%d, y1=%d, y2=%d\n", x1, x2, y1, y2);
	 * SPOWER_INFO("sptab_lookup-volt=%d, deg=%d, lkg=%d\n",
	 *                                 voltage, degree, mamper);
	 */
	return mamper;
}

/* use efuse power(wat) and FF, TT, SS table to calculate new tables */
/* ex: if want to calculate big cluster leakage table
 * spower_raw_main will be "big cluster spower raw data"
 * spower_raw_sub will be "big core spower raw data"
 * wat : efuse power of cpu big (include core + cluster)
 */
int mtk_spower_make_table_cpu(struct sptbl_t *spt, struct spower_raw_t *spower_raw_main, struct spower_raw_t
*spower_raw_sub, int wat, int voltage, int degree, int dev)
{
	struct sptbl_t tab_main[MAX_TABLE_SIZE], tab_sub[MAX_TABLE_SIZE], *tab1, *tab2, *tspt;
	struct sptbl_t *tab1_sub, *tab2_sub;
	int i;
	int c[MAX_TABLE_SIZE] = {0};

	/* tab_main[] points to main bank's FF, TT, SS raw data */
	/* tab_sub[] points to main bank's FF, TT, SS raw data */
	spower_tab_construct(&tab_main, spower_raw_main);
	spower_tab_construct(&tab_sub, spower_raw_sub);
	/* find c in FF, TT, SS tables according to efuse volt, degree */
	/* and compare to efuse power(wat) to decide interval of this chip */
	/* wat is total efuse power, hence we use c(cpu)+c(ncpu) to compare with wat */
	for (i = 0; i < spower_raw_main->table_size; i++) {
		/* ex: cpu FF + ncpu FF, cpu TT + ncpu TT */
		if (dev == MTK_SPOWER_CPUBIG) {
			c[i] = ((sptab_lookup(&tab_main[i], voltage, degree) * 2) +
				sptab_lookup(&tab_sub[i], voltage, degree)) >> 10;
		} else if (dev == MTK_SPOWER_CPUBIG_CLUSTER) {
			c[i] = (sptab_lookup(&tab_main[i], voltage, degree) +
				(sptab_lookup(&tab_sub[i], voltage, degree) * 2)) >> 10;
		} else if ((dev == MTK_SPOWER_CPULL) || (dev == MTK_SPOWER_CPUL)) {
			c[i] = ((sptab_lookup(&tab_main[i], voltage, degree) * 4) +
				sptab_lookup(&tab_sub[i], voltage, degree)) >> 10;
		} else if ((dev == MTK_SPOWER_CPULL_CLUSTER) || (dev == MTK_SPOWER_CPUL_CLUSTER)) {
			c[i] = (sptab_lookup(&tab_main[i], voltage, degree) +
				(sptab_lookup(&tab_sub[i], voltage, degree) * 4)) >> 10;
		} else {
			SPOWER_INFO("mtk_spower_make_table_cpu: dev error\n");
		}
		/* compare with efuse power */
		if (wat >= c[i])
			break;
	}

	/** FIXME,
	 * There are only 2 tables are used to interpolate to form SPTAB.
	 * Thus, sptab takes use of the container which raw data is not used anymore.
	 **/
	if (wat == c[i]) {
		/** just match **/
		tspt = tab1 = tab2 = &tab_main[i];
		SPOWER_INFO("sptab equal to tab:%d/%d\n",  wat, c[i]);
	} else if (i == spower_raw_main->table_size) {
		/** above all **/
		tspt = tab1 = tab2 = &tab_main[spower_raw_main->table_size-1];
		SPOWER_INFO("sptab max tab:%d/%d\n",  wat, c[i]);
	} else if (i == 0) {
		tspt = tab1 = tab2 = &tab_main[0];
		SPOWER_INFO("sptab min tab:%d/%d\n",  wat, c[i]);
	} else {
		/** anyone **/
		tab1 = &tab_main[i-1];
		tab2 = &tab_main[i];
		tab1_sub = &tab_sub[i-1];
		tab2_sub = &tab_sub[i];
		/** occupy the free container**/
		tspt = &tab_main[(i+1)%spower_raw_main->table_size];

		SPOWER_INFO("sptab interpolate tab:%d/%d, i:%d\n",  wat, c[i], i);
	}

	/** sptab needs to interpolate 2 tables. **/
	if (tab1 != tab2) {
		/* c1, c2, wat; total power */
		/* tab1, tab2 : target table, ex: big cluster FF, big cluster TT */
		interpolate_table(tspt, c[i-1], c[i], wat, tab1, tab2);
	}

	/** update to global data **/
	*spt = *tspt;

	return 0;
}

int mtk_spower_make_table(struct sptbl_t *spt, struct spower_raw_t *spower_raw, int wat, int voltage, int degree)
{
	int i;
	int c1, c2, c = -1;
	struct sptbl_t tab[MAX_TABLE_SIZE], *tab1, *tab2, *tspt;

	/** FIXME, test only; please read efuse to assign. **/
	/* wat = 80; */
	/* voltage = 1150; */
	/* degree = 30; */

	SPOWER_INFO("spower_raw->table_size : %d\n", spower_raw->table_size);

	/** structurize the raw data **/
	spower_tab_construct(&tab, spower_raw);

	/** lookup tables which the chip type locates to **/
	for (i = 0; i < spower_raw->table_size; i++) {
		c = sptab_lookup(&tab[i], voltage, degree) >> 10;
		/** table order: ff, tt, ss **/
		if (wat >= c)
			break;
	}

	/** FIXME,
	 * There are only 2 tables are used to interpolate to form SPTAB.
	 * Thus, sptab takes use of the container which raw data is not used anymore.
	 **/
	if (wat == c) {
		/** just match **/
		tab1 = tab2 = &tab[i];
		/** pointer duplicate  **/
		tspt = tab1;
		SPOWER_INFO("sptab equal to tab:%d/%d\n",  wat, c);
	} else if (i == spower_raw->table_size) {
		/** above all **/
#if defined(EXTER_POLATION)
		tab1 = &tab[spower_raw->table_size-2];
		tab2 = &tab[spower_raw->table_size-1];

		/** occupy the free container**/
		tspt = &tab[spower_raw->table_size-3];
#else /* #if defined(EXTER_POLATION) */
		tspt = tab1 = tab2 = &tab[spower_raw->table_size-1];
#endif /* #if defined(EXTER_POLATION) */

		SPOWER_INFO("sptab max tab:%d/%d\n",  wat, c);
	} else if (i == 0) {
#if defined(EXTER_POLATION)
		/** below all **/
		tab1 = &tab[0];
		tab2 = &tab[1];

		/** occupy the free container**/
		tspt = &tab[2];
#else /* #if defined(EXTER_POLATION) */
		tspt = tab1 = tab2 = &tab[0];
#endif /* #if defined(EXTER_POLATION) */

		SPOWER_INFO("sptab min tab:%d/%d\n",  wat, c);
	} else {
		/** anyone **/
		tab1 = &tab[i-1];
		tab2 = &tab[i];

		/** occupy the free container**/
		tspt = &tab[(i+1)%spower_raw->table_size];

		SPOWER_INFO("sptab interpolate tab:%d/%d, i:%d\n",  wat, c, i);
	}


	/** sptab needs to interpolate 2 tables. **/
	if (tab1 != tab2) {
		c1 = sptab_lookup(tab1, voltage, degree) >> 10;
		c2 = sptab_lookup(tab2, voltage, degree) >> 10;

		interpolate_table(tspt, c1, c2, wat, tab1, tab2);
	}

	/** update to global data **/
	*spt = *tspt;

	return 0;
}

#if defined(MTK_SPOWER_UT)
void mtk_spower_ut(void)
{
	int v, t, p, i;

	for (i = 0; i < MTK_SPOWER_MAX; i++) {
		struct sptbl_t *spt = &sptab[i];

		if (i == MTK_SPOWER_CPU_MAX)
			continue;

		SPOWER_INFO("This is %s\n", spower_name[i]);

		v = 750;
		t = 22;
		p  = sptab_lookup(spt, v, t);

		v = 750;
		t = 25;
		p  = sptab_lookup(spt, v, t);

		v = 750;
		t = 28;
		p  = sptab_lookup(spt, v, t);

		v = 750;
		t = 82;
		p  = sptab_lookup(spt, v, t);

		v = 750;
		t = 120;
		p  = sptab_lookup(spt, v, t);

		v = 820;
		t = 22;
		p  = sptab_lookup(spt, v, t);

		v = 820;
		t = 25;
		p  = sptab_lookup(spt, v, t);

		v = 820;
		t = 28;
		p  = sptab_lookup(spt, v, t);

		v = 820;
		t = 82;
		p  = sptab_lookup(spt, v, t);

		v = 820;
		t = 120;
		p  = sptab_lookup(spt, v, t);

		v = 1200;
		t = 22;
		p  = sptab_lookup(spt, v, t);

		v = 1200;
		t = 25;
		p  = sptab_lookup(spt, v, t);

		v = 1200;
		t = 28;
		p  = sptab_lookup(spt, v, t);

		v = 1200;
		t = 82;
		p  = sptab_lookup(spt, v, t);

		v = 1200;
		t = 120;
		p  = sptab_lookup(spt, v, t);

		v = 950;
		t = 80;
		p  = sptab_lookup(spt, v, t);

		v = 1000;
		t = 85;
		p  = sptab_lookup(spt, v, t);

		/* new test case */
		v = 300;
		t = -50;
		p = mt_spower_get_leakage(i, v, t);

		v = 300;
		t = 150;
		p = mt_spower_get_leakage(i, v, t);

		v = 1500;
		t = -50;
		p = mt_spower_get_leakage(i, v, t);

		v = 1500;
		t = 150;
		p = mt_spower_get_leakage(i, v, t);

		v = 1150;
		t = 105;
		p = mt_spower_get_leakage(i, v, t);

		v = 700;
		t = 20;
		p = mt_spower_get_leakage(i, v, t);

		v = 820;
		t = 120;
		p = mt_spower_get_leakage(i, v, t);

		v = 650;
		t = 18;
		p = mt_spower_get_leakage(i, v, t);

		v = 600;
		t = 15;
		p = mt_spower_get_leakage(i, v, t);

		v = 550;
		t = 22;
		p = mt_spower_get_leakage(i, v, t);

		v = 550;
		t = 10;
		p = mt_spower_get_leakage(i, v, t);

		v = 400;
		t = 10;
		p = mt_spower_get_leakage(i, v, t);

		v = 320;
		t = 5;
		p = mt_spower_get_leakage(i, v, t);

		v = 220;
		t = 0;
		p = mt_spower_get_leakage(i, v, t);

		v = 80;
		t = -5;
		p = mt_spower_get_leakage(i, v, t);

		v = 0;
		t = -10;
		p = mt_spower_get_leakage(i, v, t);

		v = 1200;
		t = -10;
		p = mt_spower_get_leakage(i, v, t);

		v = 1200;
		t = -25;
		p = mt_spower_get_leakage(i, v, t);

		v = 1200;
		t = -28;
		p = mt_spower_get_leakage(i, v, t);

		v = 120;
		t = -39;
		p = mt_spower_get_leakage(i, v, t);

		v = 120;
		t = -120;
		p = mt_spower_get_leakage(i, v, t);

		v = 950;
		t = -80;
		p = mt_spower_get_leakage(i, v, t);

		v = 1000;
		t = 5;
		p = mt_spower_get_leakage(i, v, t);

		v = 1150;
		t = 10;
		p = mt_spower_get_leakage(i, v, t);

		SPOWER_INFO("%s efuse: %d\n", spower_name[i], mt_spower_get_efuse_lkg(i));
		SPOWER_INFO("%s Done\n", spower_name[i]);
	}
}
#endif

static unsigned int mtSpowerInited;
int mt_spower_init(void)
{
	int i, devinfo = 0;

	if (mtSpowerInited == 1)
		return 0;

	/* avoid side effect from multiple invocation */
	if (tab_validate(&sptab[MTK_SPOWER_CPUBIG]))
		return 0;

	for (i = 0; i < MTK_LEAKAGE_MAX; i++) {
		devinfo = (int)get_devinfo_with_index(devinfo_idx[i]);
		mtk_efuse_leakage[i] = (devinfo >> devinfo_offset[i]) & 0xff;
		SPOWER_INFO("[Efuse] %s => 0x%x\n", leakage_name[i], mtk_efuse_leakage[i]);
		mtk_efuse_leakage[i] = (int) devinfo_table[mtk_efuse_leakage[i]];
		SPOWER_INFO("[Efuse Leakage] %s => 0x%x\n", leakage_name[i], mtk_efuse_leakage[i]);
#if 0
		if (mtk_efuse_leakage[i] != 0) {
			mtk_efuse_leakage[i] = (int) devinfo_table[mtk_efuse_leakage[i]];
			mtk_efuse_leakage[i] = (int) (mtk_efuse_leakage[i] * V_OF_FUSE / 1000);
		} else {
			mtk_efuse_leakage[i] = default_leakage[i];
		}
#else
		mtk_efuse_leakage[i] = default_leakage[i];
#endif
		SPOWER_INFO("[Default Leakage] %s => 0x%x\n", leakage_name[i], mtk_efuse_leakage[i]);
	}

	/* for cpu, ncpu leakage table , use mtk_spower_make_table_cpu */
	for (i = 0; i < MTK_SPOWER_CPU_MAX; i += 2) {
		SPOWER_INFO("%s\n", spower_name[i]);
		mtk_spower_make_table_cpu(&sptab[i], &spower_raw[i], &spower_raw[i + 1],
				mtk_efuse_leakage[i >> 1], V_OF_FUSE, T_OF_FUSE, i);

		SPOWER_INFO("%s\n", spower_name[i + 1]);
		mtk_spower_make_table_cpu(&sptab[i + 1], &spower_raw[i + 1], &spower_raw[i],
				mtk_efuse_leakage[i >> 1], V_OF_FUSE, T_OF_FUSE, i + 1);
	}

	/* for others' leakage table , use mtk_spower_make_table */
	for (i = MTK_SPOWER_CPU_MAX + 1; i < MTK_SPOWER_MAX; i++) {
		SPOWER_INFO("%s\n", spower_name[i]);
		mtk_spower_make_table(&sptab[i], &spower_raw[i],
				mtk_efuse_leakage[i - (MTK_SPOWER_CPU_MAX >> 1) - 1],
				V_OF_FUSE, T_OF_FUSE);
	}

#if defined(MTK_SPOWER_UT)
	SPOWER_INFO("Start SPOWER UT!\n");
	mtk_spower_ut();
	SPOWER_INFO("End SPOWER UT!\n");
#endif

	mtSpowerInited = 1;
	return 0;
}

module_init(mt_spower_init);

/* return 0, means sptab is not yet ready. */
/* vol unit should be mv */
int mt_spower_get_leakage(int dev, unsigned int vol, int deg)
{
	int ret;

	if (!tab_validate(&sptab[dev]))
		return 0;

	if (vol > mV(&sptab[dev], VSIZE - 1))
		vol = mV(&sptab[dev], VSIZE - 1);
	else if (vol < mV(&sptab[dev], 0))
		vol = mV(&sptab[dev], 0);

	if (deg > deg(&sptab[dev], TSIZE - 1))
		deg = deg(&sptab[dev], TSIZE - 1);
	else if (deg < deg(&sptab[dev], 0))
		deg = deg(&sptab[dev], 0);

	ret = sptab_lookup(&sptab[dev], (int)vol, deg) >> 10;
	SPOWER_INFO("mt_spower_get_leakage-dev=%d, volt=%d, deg=%d, lkg=%d\n",
		    dev, vol, deg, ret);
	return ret;
}
EXPORT_SYMBOL(mt_spower_get_leakage);

int mt_spower_get_efuse_lkg(int dev)
{
	int devinfo = 0, efuse_lkg = 0, efuse_lkg_mw = 0;

	devinfo = (int) get_devinfo_with_index(devinfo_idx[dev]);
	efuse_lkg = (devinfo >> devinfo_offset[dev]) & 0xff;
	efuse_lkg_mw = (efuse_lkg == 0) ? default_leakage[dev] : (int) (devinfo_table[efuse_lkg] * V_OF_FUSE / 1000);

	return efuse_lkg_mw;
}
EXPORT_SYMBOL(mt_spower_get_efuse_lkg);

