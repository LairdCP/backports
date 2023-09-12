#ifndef _BACKPORT_LINUX_REGULATOR_CONSUMER_H_
#define _BACKPORT__LINUX_REGULATOR_CONSUMER_H_
#include_next <linux/regulator/consumer.h>
#include <linux/version.h>

#if LINUX_VERSION_IS_LESS(3,4,0)
static inline int devm_regulator_bulk_get(struct device *dev, int num_consumers,
					  struct regulator_bulk_data *consumers)
{
	return 0;
}
#endif

#endif /* _BACKPORT_LINUX_REGULATOR_CONSUMER_H_ */
