#include <linux/notifier.h>
#include <linux/export.h>

static BLOCKING_NOTIFIER_HEAD(cam_vcm_notifier_list);

/**
 *	cam_vcm_register_client - register a client notifier
 *	@nb: notifier block to callback on events
 */
int cam_vcm_register_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&cam_vcm_notifier_list, nb);
}
EXPORT_SYMBOL(cam_vcm_register_client);

/**
 *	cam_vcm_unregister_client - unregister a client notifier
 *	@nb: notifier block to callback on events
 */
int cam_vcm_unregister_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&cam_vcm_notifier_list, nb);
}
EXPORT_SYMBOL(cam_vcm_unregister_client);

/**
 * cam_vcm_notifier_call_chain - notify clients of cam_vcm_events
 *
 */

int cam_vcm_notifier_call_chain(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&cam_vcm_notifier_list, val, v);
}
EXPORT_SYMBOL_GPL(cam_vcm_notifier_call_chain);
