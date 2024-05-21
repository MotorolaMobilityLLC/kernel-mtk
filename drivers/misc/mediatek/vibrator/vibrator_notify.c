#include <linux/notifier.h>
#include <linux/export.h>

static BLOCKING_NOTIFIER_HEAD(vibrator_notifier_list);

/**
 *	vibrator_register_client - register a client notifier
 *	@nb: notifier block to callback on events
 */
int vibrator_register_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&vibrator_notifier_list, nb);
}
EXPORT_SYMBOL(vibrator_register_client);

/**
 *	vibrator_unregister_client - unregister a client notifier
 *	@nb: notifier block to callback on events
 */
int vibrator_unregister_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&vibrator_notifier_list, nb);
}
EXPORT_SYMBOL(vibrator_unregister_client);

/**
 * vibrator_notifier_call_chain - notify clients of vibrator_events
 *
 */

int vibrator_notifier_call_chain(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&vibrator_notifier_list, val, v);
}
EXPORT_SYMBOL_GPL(vibrator_notifier_call_chain);