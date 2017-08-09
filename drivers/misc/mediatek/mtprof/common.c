#include <linux/irq.h>
#include <linux/irqnr.h>
#include <linux/interrupt.h>
#include "internal.h"

long long nsec_high(unsigned long long nsec)
{
	if ((long long)nsec < 0) {
		nsec = -nsec;
		do_div(nsec, 1000000);
		return -nsec;
	}
	do_div(nsec, 1000000);

	return nsec;
}

unsigned long nsec_low(unsigned long long nsec)
{
	if ((long long)nsec < 0)
		nsec = -nsec;

	return do_div(nsec, 1000000);
}

long long usec_high(unsigned long long usec)
{
	if ((long long)usec < 0) {
		usec = -usec;
		do_div(usec, 1000);
		return -usec;
	}
	do_div(usec, 1000);

	return usec;
}

unsigned long usec_low(unsigned long long usec)
{
	if ((long long)usec < 0)
		usec = -usec;

	return do_div(usec, 1000);
}

const char *isr_name(int irq)
{
	struct irq_desc *desc;

	desc = irq_to_desc(irq);

	if (desc && desc->action && desc->action->name)
		return desc->action->name;
	return NULL;
}
