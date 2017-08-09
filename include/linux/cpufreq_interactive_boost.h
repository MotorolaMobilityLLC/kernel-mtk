#ifndef _LINUX_CPUFREQ_INTERACTIVE_BOOST_H
#define _LINUX_CPUFREQ_INTERACTIVE_BOOST_H

#ifdef CONFIG_CPU_FREQ_GOV_INTERACTIVE
extern int interactive_boost_cpu(int boost);
#else
static inline int interactive_boost_cpu(int boost)
{
	return 0;
}
#endif

#endif /* _LINUX_CPUFREQ_INTERACTIVE_BOOST_H */
