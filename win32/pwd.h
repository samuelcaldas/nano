/*
 * pwd.h - Minimal password/user database compatibility for Windows MSVC.
 */

#ifndef WIN32_PWD_H
#define WIN32_PWD_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _UID_T_DEFINED
#define _UID_T_DEFINED
typedef unsigned int uid_t;
typedef unsigned int gid_t;
#endif

struct passwd {
    char  *pw_name;
    char  *pw_passwd;
    uid_t  pw_uid;
    gid_t  pw_gid;
    char  *pw_gecos;
    char  *pw_dir;
    char  *pw_shell;
};

struct passwd *getpwuid(uid_t uid);
struct passwd *getpwnam(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* WIN32_PWD_H */
