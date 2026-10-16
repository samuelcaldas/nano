/*
 * libgen.h - POSIX dirname and basename functions for Windows MSVC.
 */

#ifndef WIN32_LIBGEN_H
#define WIN32_LIBGEN_H

#ifdef __cplusplus
extern "C" {
#endif

char *win32_dirname(char *path);
char *win32_basename(char *path);

#ifndef dirname
#define dirname win32_dirname
#endif

#ifndef basename
#define basename win32_basename
#endif

#ifdef __cplusplus
}
#endif

#endif /* WIN32_LIBGEN_H */
