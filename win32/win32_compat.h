/*
 * win32_compat.h - Master POSIX-to-Win32 compatibility header for GNU nano on MSVC.
 */

#ifndef WIN32_COMPAT_H
#define WIN32_COMPAT_H

#if defined(_WIN32)

#include <windows.h>
#include <signal.h>
#include <io.h>
#include <direct.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

#ifdef MB_CUR_MAX
#undef MB_CUR_MAX
#endif
#define MB_CUR_MAX 16

#ifdef MOUSE_MOVED
#undef MOUSE_MOVED
#endif

#include "win32_path.h"
#include "wcwidth.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef P_tmpdir
#define P_tmpdir "C:\\Windows\\Temp"
#endif

#define realpath(path, resolved)   win32_canonicalize_file_name(path)
#define mkstemp(template_str)      mkstemps(template_str, 0)

/* Standard POSIX descriptors */
#ifndef STDIN_FILENO
#define STDIN_FILENO  0
#endif
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

/* POSIX access modes */
#ifndef R_OK
#define R_OK 4
#endif
#ifndef W_OK
#define W_OK 2
#endif
#ifndef X_OK
#define X_OK 1
#endif
#ifndef F_OK
#define F_OK 0
#endif

/* POSIX types */
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
#if defined(_WIN64)
typedef __int64 ssize_t;
#else
typedef int ssize_t;
#endif
#endif

#ifndef _PID_T_DEFINED
#define _PID_T_DEFINED
typedef int pid_t;
#endif

#ifndef _UID_T_DEFINED
#define _UID_T_DEFINED
typedef unsigned int uid_t;
typedef unsigned int gid_t;
#endif

#ifndef _MODE_T_DEFINED
#define _MODE_T_DEFINED
typedef unsigned short mode_t;
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* POSIX file permission bits */
#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#endif
#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE
#endif
#ifndef S_IXUSR
#define S_IXUSR _S_IEXEC
#endif
#ifndef S_IRWXU
#define S_IRWXU (S_IRUSR | S_IWUSR | S_IXUSR)
#endif
#ifndef S_IRGRP
#define S_IRGRP (S_IRUSR >> 3)
#endif
#ifndef S_IWGRP
#define S_IWGRP (S_IWUSR >> 3)
#endif
#ifndef S_IXGRP
#define S_IXGRP (S_IXUSR >> 3)
#endif
#ifndef S_IRWXG
#define S_IRWXG (S_IRGRP | S_IWGRP | S_IXGRP)
#endif
#ifndef S_IROTH
#define S_IROTH (S_IRGRP >> 3)
#endif
#ifndef S_IWOTH
#define S_IWOTH (S_IWGRP >> 3)
#endif
#ifndef S_IXOTH
#define S_IXOTH (S_IXGRP >> 3)
#endif
#ifndef S_IRWXO
#define S_IRWXO (S_IROTH | S_IWOTH | S_IXOTH)
#endif

/* File type test macros */
#ifndef S_ISREG
#define S_ISREG(m)  (((m) & _S_IFMT) == _S_IFREG)
#endif
#ifndef S_ISDIR
#define S_ISDIR(m)  (((m) & _S_IFMT) == _S_IFDIR)
#endif
#ifndef S_ISCHR
#define S_ISCHR(m)  (((m) & _S_IFMT) == _S_IFCHR)
#endif
#ifndef S_ISFIFO
#define S_ISFIFO(m) (((m) & _S_IFMT) == 0)
#endif
#ifndef S_ISLNK
#define S_ISLNK(m)  (0)
#endif
#ifndef S_ISSOCK
#define S_ISSOCK(m) (0)
#endif
#ifndef S_ISBLK
#define S_ISBLK(m)  (0)
#endif

/* Signal structure and stubs */
struct sigaction {
    void (*sa_handler)(int);
    unsigned int sa_flags;
    int sa_mask;
};

#define SA_RESETHAND 0x80000000
#define sigemptyset(set)        (*(set) = 0)
#define sigfillset(set)         (*(set) = ~0)
#define sigaddset(set, sig)     (*(set) |= (1 << (sig)))
#define sigdelset(set, sig)     (*(set) &= ~(1 << (sig)))
#define sigprocmask(how, s, os) (0)

static inline int sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
    (void)oact;
    if (act && act->sa_handler)
        signal(sig, act->sa_handler);
    return 0;
}

/* String case-insensitive comparisons */
#define strcasecmp  _stricmp
#define strncasecmp _strnicmp

char *strcasestr(const char *haystack, const char *needle);

/* POSIX getline and getdelim */
ssize_t getdelim(char **lineptr, size_t *n, int delimiter, FILE *stream);
ssize_t getline(char **lineptr, size_t *n, FILE *stream);

/* Temporary file with suffix */
int mkstemps(char *template_str, int suffixlen);

/* Process & User stubs */
uid_t getuid(void);
uid_t geteuid(void);
gid_t getgid(void);
gid_t getegid(void);
pid_t fork(void);
int   pipe(int fds[2]);

/* System & file control shims */
unsigned int sleep(unsigned int seconds);
int          usleep(unsigned int usec);
int          ftruncate(int fd, __int64 length);
int          truncate(const char *path, __int64 length);
int          fsync(int fd);
int          futimens(int fd, const struct timespec times[2]);

/* Langinfo */
typedef int nl_item;
#define CODESET 1
char *nl_langinfo(nl_item item);

/* Redefine filesystem calls to UTF-8 aware win32 shims */
#define stat(p, b)                 win32_stat(p, b)
#define lstat(p, b)                win32_stat(p, b)
#define fopen(f, m)                win32_fopen(f, m)
#define open                       win32_open
#define access(p, m)               win32_access(p, m)
#define chmod(p, m)                win32_chmod(p, m)
#define unlink(p)                  win32_unlink(p)
#define rename(o, n)               win32_rename(o, n)
#define mkdir(p, m)                win32_mkdir(p, m)
#define rmdir(p)                   win32_rmdir(p)
#define canonicalize_file_name(p)  win32_canonicalize_file_name(p)
#define getenv(n)                  win32_getenv(n)

#ifdef __cplusplus
}
#endif

#endif /* _WIN32 */

#endif /* WIN32_COMPAT_H */
