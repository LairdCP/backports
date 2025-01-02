#ifndef __BP_LINUX_IO_H
#define __BP_LINUX_IO_H
#include_next <linux/io.h>

#ifndef IOMEM_ERR_PTR
#define IOMEM_ERR_PTR(err) (__force void __iomem *)ERR_PTR(err)
#endif

#if LINUX_VERSION_IS_LESS(4,5,0)
#define __ioread32_copy LINUX_BACKPORT(__ioread32_copy)
void __ioread32_copy(void *to, const void __iomem *from, size_t count);
#endif

#endif /* __BP_LINUX_IO_H */