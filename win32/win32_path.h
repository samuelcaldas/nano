/*
 * win32_path.h - Windows UTF-8 / UTF-16 filesystem path conversion & I/O shims.
 */

#ifndef WIN32_PATH_H
#define WIN32_PATH_H

#include <stdio.h>
#include <wchar.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

wchar_t *win32_utf8_to_wchar(const char *utf8);
char    *win32_wchar_to_utf8(const wchar_t *wstr);
void     win32_normalize_path(char *path);

FILE    *win32_fopen(const char *filename, const char *mode);
int      win32_open(const char *filename, int flags, ...);
int      win32_stat(const char *path, struct stat *buf);
int      win32_access(const char *path, int mode);
int      win32_chmod(const char *path, int mode);
int      win32_unlink(const char *path);
int      win32_rename(const char *oldpath, const char *newpath);
int      win32_mkdir(const char *path, int mode);
int      win32_rmdir(const char *path);
char    *win32_canonicalize_file_name(const char *path);
char    *win32_getenv(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* WIN32_PATH_H */
