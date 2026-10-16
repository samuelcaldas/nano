/*
 * langinfo.h - Minimal langinfo compatibility for Windows MSVC.
 */

#ifndef WIN32_LANGINFO_H
#define WIN32_LANGINFO_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int nl_item;

#define CODESET 1

char *nl_langinfo(nl_item item);

#ifdef __cplusplus
}
#endif

#endif /* WIN32_LANGINFO_H */
