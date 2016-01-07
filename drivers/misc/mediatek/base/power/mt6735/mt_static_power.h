#ifndef MT_STATIC_POWER_H
#define MT_STATIC_POWER_H


enum {
	MT_SPOWER_CPU = 0,
	MT_SPOWER_VCORE,
	MT_SPOWER_LTE,
	MT_SPOWER_MAX,
};

/*
 * @argument
 * dev: the enum of MT_SPOWER_xxx
 * voltage: the operating voltage, mV.
 * degree: the Tj. (degree C)
 * @return
 *  -1, means sptab is not yet ready.
 *  other value: the mW of leakage value.
 */
extern u32 get_devinfo_with_index(u32 index);
extern int mt_spower_get_leakage(int dev, int voltage, int degree);
extern int mt_spower_init(void);


#endif
