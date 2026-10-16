/*
 * wcwidth.h - Unicode character width calculation for Windows MSVC.
 */

#ifndef WIN32_WCWIDTH_H
#define WIN32_WCWIDTH_H

#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

int mk_wcwidth(wchar_t ucs);
int mk_wcswidth(const wchar_t *pwcs, size_t n);

#ifndef wcwidth
#define wcwidth mk_wcwidth
#endif

#ifndef wcswidth
#define wcswidth mk_wcswidth
#endif

#ifdef __cplusplus
}
#endif

#endif /* WIN32_WCWIDTH_H */
