/*
 * unistd.h - POSIX API compatibility header for Windows MSVC.
 */

#ifndef WIN32_UNISTD_H
#define WIN32_UNISTD_H

#include <io.h>
#include <process.h>
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STDIN_FILENO
#define STDIN_FILENO  0
#endif
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

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

unsigned int sleep(unsigned int seconds);
int          usleep(unsigned int usec);
int          ftruncate(int fd, __int64 length);
int          truncate(const char *path, __int64 length);
int          fsync(int fd);
uid_t        getuid(void);
uid_t        geteuid(void);
gid_t        getgid(void);
gid_t        getegid(void);
int          pipe(int fds[2]);
pid_t        fork(void);

#ifdef __cplusplus
}
#endif

#endif /* WIN32_UNISTD_H */
