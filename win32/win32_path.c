/*
 * win32_path.c - UTF-8 Windows filesystem and environment integration for GNU nano.
 */

#include "win32_path.h"
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

wchar_t *win32_utf8_to_wchar(const char *utf8)
{
    int needed;
    wchar_t *wstr;

    if (!utf8)
        return NULL;

    needed = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (needed <= 0)
        return NULL;

    wstr = (wchar_t *)malloc(needed * sizeof(wchar_t));
    if (!wstr)
        return NULL;

    if (MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, needed) <= 0) {
        free(wstr);
        return NULL;
    }

    return wstr;
}

char *win32_wchar_to_utf8(const wchar_t *wstr)
{
    int needed;
    char *utf8;

    if (!wstr)
        return NULL;

    needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (needed <= 0)
        return NULL;

    utf8 = (char *)malloc(needed);
    if (!utf8)
        return NULL;

    if (WideCharToMultiByte(CP_UTF8, 0, wstr, -1, utf8, needed, NULL, NULL) <= 0) {
        free(utf8);
        return NULL;
    }

    return utf8;
}

void win32_normalize_path(char *path)
{
    if (!path)
        return;

    for (char *p = path; *p != '\0'; ++p) {
        if (*p == '\\')
            *p = '/';
    }
}

FILE *win32_fopen(const char *filename, const char *mode)
{
    wchar_t *wfilename = win32_utf8_to_wchar(filename);
    wchar_t *wmode = win32_utf8_to_wchar(mode);
    FILE *fp = NULL;

    if (wfilename && wmode) {
        fp = _wfopen(wfilename, wmode);
    }

    free(wfilename);
    free(wmode);
    return fp;
}

int win32_open(const char *filename, int flags, ...)
{
    wchar_t *wfilename = win32_utf8_to_wchar(filename);
    int fd = -1;
    int mode = 0;

    if (flags & _O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, int);
        va_end(ap);
    }

    if (wfilename) {
        _wsopen_s(&fd, wfilename, flags | _O_BINARY, _SH_DENYNO, mode ? mode : (_S_IREAD | _S_IWRITE));
        free(wfilename);
    }

    return fd;
}

int win32_stat(const char *path, struct stat *buf)
{
    wchar_t *wpath;
    int res = -1;

    if (!path || !buf)
        return -1;

    wpath = win32_utf8_to_wchar(path);
    if (wpath) {
        /* Strip trailing slash unless root */
        size_t len = wcslen(wpath);
        if (len > 1 && (wpath[len - 1] == L'/' || wpath[len - 1] == L'\\') &&
            !(len == 3 && wpath[1] == L':')) {
            wpath[len - 1] = L'\0';
        }
        res = _wstat64i32(wpath, (struct _stat64i32 *)buf);
        free(wpath);
    }

    return res;
}

int win32_access(const char *path, int mode)
{
    wchar_t *wpath;
    int crt_mode = 0;
    int res = -1;

    if (!path) {
        errno = ENOENT;
        return -1;
    }

    wpath = win32_utf8_to_wchar(path);
    if (!wpath) {
        errno = ENOENT;
        return -1;
    }

    /* Translate POSIX access modes (0=F_OK, 1=X_OK, 2=W_OK, 4=R_OK) to MSVC CRT modes (0, 2, 4, 6) */
    if (mode & 2) /* W_OK */
        crt_mode |= 2;
    if (mode & 4) /* R_OK */
        crt_mode |= 4;
    /* X_OK (1) has no Windows equivalent; maps to existence check if neither R nor W was set */

    res = _waccess(wpath, crt_mode);
    free(wpath);
    return res;
}

int win32_chmod(const char *path, int mode)
{
    wchar_t *wpath;
    int crt_mode = _S_IREAD;
    int res = -1;

    if (!path) {
        errno = ENOENT;
        return -1;
    }

    wpath = win32_utf8_to_wchar(path);
    if (!wpath) {
        errno = ENOENT;
        return -1;
    }

    if (mode & (_S_IWRITE | 0222))
        crt_mode |= _S_IWRITE;

    res = _wchmod(wpath, crt_mode);
    free(wpath);
    return res;
}

int win32_unlink(const char *path)
{
    wchar_t *wpath = win32_utf8_to_wchar(path);
    int res = -1;

    if (wpath) {
        res = _wunlink(wpath);
        free(wpath);
    }

    return res;
}

int win32_rename(const char *oldpath, const char *newpath)
{
    wchar_t *wold = win32_utf8_to_wchar(oldpath);
    wchar_t *wnew = win32_utf8_to_wchar(newpath);
    int res = -1;

    if (wold && wnew) {
        /* POSIX rename overwrites destination atomically if it exists */
        if (MoveFileExW(wold, wnew, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
            res = 0;
        } else {
            errno = EACCES;
        }
    }

    free(wold);
    free(wnew);
    return res;
}

int win32_mkdir(const char *path, int mode)
{
    (void)mode;
    wchar_t *wpath = win32_utf8_to_wchar(path);
    int res = -1;

    if (wpath) {
        res = _wmkdir(wpath);
        free(wpath);
    }

    return res;
}

int win32_rmdir(const char *path)
{
    wchar_t *wpath = win32_utf8_to_wchar(path);
    int res = -1;

    if (wpath) {
        res = _wrmdir(wpath);
        free(wpath);
    }

    return res;
}

char *win32_canonicalize_file_name(const char *path)
{
    wchar_t *wpath;
    wchar_t wfull[MAX_PATH * 2];
    char *utf8_full = NULL;

    if (!path)
        return NULL;

    wpath = win32_utf8_to_wchar(path);
    if (!wpath)
        return NULL;

    if (_wfullpath(wfull, wpath, MAX_PATH * 2)) {
        utf8_full = win32_wchar_to_utf8(wfull);
        if (utf8_full)
            win32_normalize_path(utf8_full);
    }

    free(wpath);
    return utf8_full;
}

char *win32_getenv(const char *name)
{
    char *val;

    if (!name)
        return NULL;

    val = getenv(name);
    if (val && *val != '\0')
        return val;

    if (strcmp(name, "HOME") == 0) {
        val = getenv("USERPROFILE");
        if (val && *val != '\0')
            return val;
    } else if (strcmp(name, "XDG_CONFIG_HOME") == 0) {
        val = getenv("APPDATA");
        if (val && *val != '\0')
            return val;
    }

    return val;
}
