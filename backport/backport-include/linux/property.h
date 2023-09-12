#ifndef __BACKPORT_LINUX_PROPERTY_H_
#define __BACKPORT_LINUX_PROPERTY_H_
#include <linux/version.h>

#if LINUX_VERSION_IS_GEQ(3,18,17)
#include_next <linux/property.h>
#endif

#if LINUX_VERSION_IS_LESS(3,19,0)
enum fwnode_type {
	FWNODE_INVALID = 0,
	FWNODE_OF,
	FWNODE_ACPI,
};

struct fwnode_handle {
	enum fwnode_type type;
};

static inline int fwnode_property_read_u8_array(struct fwnode_handle *fwnode,
				  const char *propname, u8 *val, size_t nval)
{
	return -ENXIO;
}
#endif /* LINUX_VERSION_IS_LESS(3,19,0) */

#if LINUX_VERSION_IS_LESS(4,3,0)
#define device_get_mac_address LINUX_BACKPORT(device_get_mac_address)
void *device_get_mac_address(struct device *dev, char *addr, int alen);
#endif /* LINUX_VERSION_IS_LESS(4,3,0) */

#if LINUX_VERSION_IS_LESS(4,12,0)
#define dev_fwnode LINUX_BACKPORT(dev_fwnode)
struct fwnode_handle *dev_fwnode(struct device *dev);
#endif /* LINUX_VERSION_IS_LESS(4,12,0) */

#if LINUX_VERSION_IS_LESS(4,16,0)
#define device_get_match_data LINUX_BACKPORT(device_get_match_data)
const void *device_get_match_data(struct device *dev);
#endif

#endif /* __BACKPORT_LINUX_PROPERTY_H_ */
