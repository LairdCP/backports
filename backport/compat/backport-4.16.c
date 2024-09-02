#include <linux/export.h>
#include <linux/device.h>
#include <linux/property.h>

#include <linux/of.h>
#include <linux/of_device.h>


const void *device_get_match_data(struct device *dev)
{
#if defined(CONFIG_OF)
#if LINUX_VERSION_IS_GEQ(4,2,0)
	return of_device_get_match_data(dev);
#else
	const struct of_device_id *match;

	match = of_match_device(dev->driver->of_match_table, dev);
	if (!match)
		return NULL;

	return match->data;
#endif
#else
	return NULL;
#endif
}
EXPORT_SYMBOL_GPL(device_get_match_data);
