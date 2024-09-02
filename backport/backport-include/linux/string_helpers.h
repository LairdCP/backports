#ifndef __BACKPORT_LINUX_STRING_HELPER_H
#define __BACKPORT_LINUX_STRING_HELPER_H
#include_next <linux/string_helpers.h>

#if LINUX_VERSION_IS_LESS(5,18,0)
static inline const char *str_on_off(bool v)
{
	return v ? "on" : "off";
}
#endif

#endif