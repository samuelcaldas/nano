/*
 * dirent.h - POSIX directory entry definitions for Windows MSVC.
 */

#ifndef WIN32_DIRENT_H
#define WIN32_DIRENT_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DT_UNKNOWN 0
#define DT_FIFO    1
#define DT_CHR     2
#define DT_DIR     4
#define DT_BLK     6
#define DT_REG     8
#define DT_LNK     10
#define DT_SOCK    12
#define DT_WHT     14

struct dirent {
    unsigned char  d_type;
    char           d_name[MAX_PATH * 3];
};

typedef struct DIR {
    HANDLE          handle;
    WIN32_FIND_DATAW find_data;
    struct dirent   entry;
    int             is_first;
} DIR;

DIR           *opendir(const char *name);
struct dirent *readdir(DIR *dir);
int            closedir(DIR *dir);
void           rewinddir(DIR *dir);

#ifdef __cplusplus
}
#endif

#endif /* WIN32_DIRENT_H */
