#ifndef __BACKPORT_LINUX_MMC_CORE_H
#define __BACKPORT_LINUX_MMC_CORE_H
#include_next <linux/mmc/core.h>

#if LINUX_VERSION_IS_LESS(5,18,0)
#if LINUX_VERSION_IS_GEQ(3,2,0)
#define mmc_hw_reset(card) mmc_hw_reset(card->host)
#else
#define mmc_hw_reset(card) -EOPNOTSUPP
#endif
#endif

#endif /* __BACKPORT_LINUX_MMC_CORE_H */
