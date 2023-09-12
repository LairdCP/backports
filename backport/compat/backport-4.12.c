#include <linux/device.h>
#include <linux/property.h>

#if LINUX_VERSION_IS_GEQ(4,1,0)

#include <linux/of.h>

struct fwnode_handle *dev_fwnode(struct device *dev)
{
	return dev_of_node(dev) ? &dev_of_node(dev)->fwnode : dev->fwnode;
}

#else  /* LINUX_VERSION_IS_GEQ(4,1,0) */

struct fwnode_handle *dev_fwnode(struct device *dev)
{
	return NULL;
}

#endif /* LINUX_VERSION_IS_GEQ(4,1,0) */

EXPORT_SYMBOL_GPL(dev_fwnode);
