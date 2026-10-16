/*
 * strings.h - POSIX strings.h compatibility for Windows MSVC.
 */

#ifndef WIN32_STRINGS_H
#define WIN32_STRINGS_H

#include <string.h>

#define strcasecmp  _stricmp
#define strncasecmp _strnicmp

#endif /* WIN32_STRINGS_H */
