/*
 * win32_compat.c - POSIX compatibility implementations for Windows MSVC.
 */

#include "win32_compat.h"
#include "dirent.h"
#include "libgen.h"
#include "pwd.h"
#include "sys/wait.h"
#include <windows.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

/* --- Directory operations (opendir, readdir, closedir) --- */

DIR *opendir(const char *name)
{
    DIR *dir;
    char search_path[MAX_PATH * 3];
    wchar_t *wsearch;
    size_t len;

    if (!name) {
        errno = ENOENT;
        return NULL;
    }

    dir = (DIR *)malloc(sizeof(DIR));
    if (!dir) {
        errno = ENOMEM;
        return NULL;
    }

    len = strlen(name);
    if (len == 0) {
        snprintf(search_path, sizeof(search_path), "./*");
    } else if (name[len - 1] == '/' || name[len - 1] == '\\') {
        snprintf(search_path, sizeof(search_path), "%s*", name);
    } else {
        snprintf(search_path, sizeof(search_path), "%s/*", name);
    }

    wsearch = win32_utf8_to_wchar(search_path);
    if (!wsearch) {
        free(dir);
        errno = ENOMEM;
        return NULL;
    }

    dir->handle = FindFirstFileW(wsearch, &dir->find_data);
    free(wsearch);

    if (dir->handle == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        free(dir);
        errno = (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) ? ENOENT : EACCES;
        return NULL;
    }

    dir->is_first = 1;
    return dir;
}

struct dirent *readdir(DIR *dir)
{
    if (!dir || dir->handle == INVALID_HANDLE_VALUE) {
        errno = EBADF;
        return NULL;
    }

    if (dir->is_first) {
        dir->is_first = 0;
    } else {
        if (!FindNextFileW(dir->handle, &dir->find_data))
            return NULL;
    }

    WideCharToMultiByte(CP_UTF8, 0, dir->find_data.cFileName, -1,
                        dir->entry.d_name, sizeof(dir->entry.d_name), NULL, NULL);

    if (dir->find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        dir->entry.d_type = DT_DIR;
    else
        dir->entry.d_type = DT_REG;

    return &dir->entry;
}

int closedir(DIR *dir)
{
    if (!dir) {
        errno = EBADF;
        return -1;
    }

    if (dir->handle != INVALID_HANDLE_VALUE)
        FindClose(dir->handle);

    free(dir);
    return 0;
}

void rewinddir(DIR *dir)
{
    (void)dir;
}

/* --- Path helpers (dirname, basename) --- */

char *win32_dirname(char *path)
{
    static char dot[] = ".";
    char *p;

    if (!path || *path == '\0')
        return dot;

    win32_normalize_path(path);
    p = strrchr(path, '/');
    if (!p)
        return dot;

    if (p == path) {
        /* Root / */
        *(p + 1) = '\0';
        return path;
    }

    /* Drive letter root like C:/ */
    if (p == path + 2 && path[1] == ':') {
        *(p + 1) = '\0';
        return path;
    }

    *p = '\0';
    return path;
}

char *win32_basename(char *path)
{
    static char dot[] = ".";
    char *p;

    if (!path || *path == '\0')
        return dot;

    win32_normalize_path(path);
    p = strrchr(path, '/');
    if (!p)
        return path;

    return p + 1;
}

/* --- getline & getdelim --- */

ssize_t getdelim(char **lineptr, size_t *n, int delimiter, FILE *stream)
{
    char *cur;
    int c;
    size_t count = 0;

    if (!lineptr || !n || !stream) {
        errno = EINVAL;
        return -1;
    }

    if (*lineptr == NULL || *n == 0) {
        *n = 128;
        *lineptr = (char *)malloc(*n);
        if (*lineptr == NULL) {
            errno = ENOMEM;
            return -1;
        }
    }

    cur = *lineptr;
    while ((c = fgetc(stream)) != EOF) {
        if (count + 2 >= *n) {
            size_t new_n = *n * 2;
            char *new_ptr = (char *)realloc(*lineptr, new_n);
            if (!new_ptr) {
                errno = ENOMEM;
                return -1;
            }
            *lineptr = new_ptr;
            *n = new_n;
            cur = *lineptr + count;
        }

        *cur++ = (char)c;
        count++;

        if (c == delimiter)
            break;
    }

    if (count == 0)
        return -1;

    *cur = '\0';
    return (ssize_t)count;
}

ssize_t getline(char **lineptr, size_t *n, FILE *stream)
{
    return getdelim(lineptr, n, '\n', stream);
}

/* --- strcasestr --- */

char *strcasestr(const char *haystack, const char *needle)
{
    size_t needle_len;

    if (!haystack || !needle)
        return NULL;

    needle_len = strlen(needle);
    if (needle_len == 0)
        return (char *)haystack;

    for (; *haystack != '\0'; haystack++) {
        if (tolower((unsigned char)*haystack) == tolower((unsigned char)*needle)) {
            if (_strnicmp(haystack, needle, needle_len) == 0)
                return (char *)haystack;
        }
    }

    return NULL;
}

/* --- mkstemps --- */

int mkstemps(char *template_str, int suffixlen)
{
    size_t len;
    char *sub;
    static unsigned int counter = 0;
    int tries;

    if (!template_str || suffixlen < 0) {
        errno = EINVAL;
        return -1;
    }

    len = strlen(template_str);
    if ((int)len < 6 + suffixlen) {
        errno = EINVAL;
        return -1;
    }

    sub = template_str + len - suffixlen - 6;
    if (strncmp(sub, "XXXXXX", 6) != 0) {
        errno = EINVAL;
        return -1;
    }

    for (tries = 0; tries < 1000; tries++) {
        unsigned int val = (unsigned int)GetCurrentProcessId() ^ (unsigned int)GetTickCount() ^ (++counter);
        int fd;

        snprintf(sub, 7, "%06x", val & 0xffffff);

        fd = win32_open(template_str, _O_CREAT | _O_EXCL | _O_RDWR | _O_BINARY, _S_IREAD | _S_IWRITE);
        if (fd >= 0)
            return fd;

        if (errno != EEXIST)
            return -1;
    }

    return -1;
}

/* --- Process / User / Signals / Time --- */

uid_t getuid(void)  { return 1000; }
uid_t geteuid(void) { return 1000; }
gid_t getgid(void)  { return 1000; }
gid_t getegid(void) { return 1000; }

pid_t fork(void)
{
    errno = ENOSYS;
    return -1;
}

int pipe(int fds[2])
{
    return _pipe(fds, 4096, _O_BINARY);
}

unsigned int sleep(unsigned int seconds)
{
    Sleep(seconds * 1000);
    return 0;
}

int usleep(unsigned int usec)
{
    Sleep((usec + 999) / 1000);
    return 0;
}

int ftruncate(int fd, __int64 length)
{
    return _chsize_s(fd, length);
}

int truncate(const char *path, __int64 length)
{
    int fd = win32_open(path, _O_RDWR | _O_BINARY, 0);
    int ret;

    if (fd < 0)
        return -1;

    ret = _chsize_s(fd, length);
    _close(fd);
    return ret;
}

int fsync(int fd)
{
    return _commit(fd);
}

int futimens(int fd, const struct timespec times[2])
{
    (void)fd;
    (void)times;
    return 0;
}

char *nl_langinfo(nl_item item)
{
    if (item == CODESET)
        return "UTF-8";
    return "";
}

struct passwd *getpwuid(uid_t uid)
{
    static struct passwd pw;
    static char name[MAX_PATH];
    static char dir[MAX_PATH];
    DWORD name_len = sizeof(name);

    (void)uid;
    GetUserNameA(name, &name_len);
    const char *profile = win32_getenv("USERPROFILE");
    if (!profile)
        profile = "C:\\";

    strncpy(dir, profile, sizeof(dir) - 1);
    dir[sizeof(dir) - 1] = '\0';
    win32_normalize_path(dir);

    pw.pw_name = name;
    pw.pw_passwd = "";
    pw.pw_uid = 1000;
    pw.pw_gid = 1000;
    pw.pw_gecos = name;
    pw.pw_dir = dir;
    pw.pw_shell = "cmd.exe";

    return &pw;
}

struct passwd *getpwnam(const char *name)
{
    (void)name;
    return getpwuid(1000);
}

pid_t wait(int *status)
{
    if (status)
        *status = 0;
    return -1;
}

pid_t waitpid(pid_t pid, int *status, int options)
{
    (void)pid;
    (void)options;
    if (status)
        *status = 0;
    return -1;
}
