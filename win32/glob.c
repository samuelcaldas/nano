/*
 * glob.c - Minimal POSIX glob implementation using Win32 FindFirstFileW.
 */

#include "glob.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int glob(const char *pattern, int flags, int (*errfunc)(const char *, int), glob_t *pglob)
{
    char dir[MAX_PATH];
    char file_pattern[MAX_PATH];
    wchar_t wsearch_path[MAX_PATH * 2];
    WIN32_FIND_DATAW find_data;
    HANDLE hFind;
    const char *last_slash;
    size_t dir_len;
    int count = 0;
    int capacity = 16;
    char **pathv;

    if (!pattern || !pglob)
        return GLOB_ABORTED;

    pglob->gl_pathc = 0;
    pglob->gl_pathv = NULL;
    pglob->gl_offs = 0;

    last_slash = strrchr(pattern, '/');
    if (!last_slash)
        last_slash = strrchr(pattern, '\\');

    if (last_slash) {
        dir_len = (size_t)(last_slash - pattern) + 1;
        if (dir_len >= sizeof(dir))
            return GLOB_NOSPACE;
        memcpy(dir, pattern, dir_len);
        dir[dir_len] = '\0';
        strncpy(file_pattern, last_slash + 1, sizeof(file_pattern) - 1);
        file_pattern[sizeof(file_pattern) - 1] = '\0';
    } else {
        dir[0] = '\0';
        strncpy(file_pattern, pattern, sizeof(file_pattern) - 1);
        file_pattern[sizeof(file_pattern) - 1] = '\0';
    }

    if (!strpbrk(file_pattern, "*?")) {
        /* No wildcards - check if file exists */
        wchar_t wpath[MAX_PATH * 2];
        MultiByteToWideChar(CP_UTF8, 0, pattern, -1, wpath, MAX_PATH * 2);
        if (GetFileAttributesW(wpath) != INVALID_FILE_ATTRIBUTES) {
            pglob->gl_pathv = (char **)malloc(2 * sizeof(char *));
            if (!pglob->gl_pathv)
                return GLOB_NOSPACE;
            pglob->gl_pathv[0] = _strdup(pattern);
            pglob->gl_pathv[1] = NULL;
            pglob->gl_pathc = 1;
            return 0;
        }
        return GLOB_NOMATCH;
    }

    /* Wildcard expansion */
    MultiByteToWideChar(CP_UTF8, 0, pattern, -1, wsearch_path, MAX_PATH * 2);

    hFind = FindFirstFileW(wsearch_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE)
        return GLOB_NOMATCH;

    pathv = (char **)malloc(capacity * sizeof(char *));
    if (!pathv) {
        FindClose(hFind);
        return GLOB_NOSPACE;
    }

    do {
        char matched_file[MAX_PATH * 3];
        char full_path[MAX_PATH * 4];

        if (wcscmp(find_data.cFileName, L".") == 0 || wcscmp(find_data.cFileName, L"..") == 0)
            continue;

        WideCharToMultiByte(CP_UTF8, 0, find_data.cFileName, -1,
                            matched_file, sizeof(matched_file), NULL, NULL);

        if (dir[0] != '\0')
            snprintf(full_path, sizeof(full_path), "%s%s", dir, matched_file);
        else
            snprintf(full_path, sizeof(full_path), "%s", matched_file);

        if (count + 1 >= capacity) {
            capacity *= 2;
            char **new_pathv = (char **)realloc(pathv, capacity * sizeof(char *));
            if (!new_pathv) {
                FindClose(hFind);
                for (int i = 0; i < count; i++)
                    free(pathv[i]);
                free(pathv);
                return GLOB_NOSPACE;
            }
            pathv = new_pathv;
        }

        pathv[count++] = _strdup(full_path);
    } while (FindNextFileW(hFind, &find_data));

    FindClose(hFind);

    if (count == 0) {
        free(pathv);
        return GLOB_NOMATCH;
    }

    pathv[count] = NULL;
    pglob->gl_pathc = count;
    pglob->gl_pathv = pathv;
    return 0;
}

void globfree(glob_t *pglob)
{
    if (!pglob || !pglob->gl_pathv)
        return;

    for (size_t i = 0; i < pglob->gl_pathc; i++) {
        free(pglob->gl_pathv[i]);
    }
    free(pglob->gl_pathv);
    pglob->gl_pathv = NULL;
    pglob->gl_pathc = 0;
}
