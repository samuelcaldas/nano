/*
 * win32_console.h - Console lifecycle, UTF-8 code page & signal handler for Windows.
 */

#ifndef WIN32_CONSOLE_H
#define WIN32_CONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

void win32_console_init(void);
void win32_console_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* WIN32_CONSOLE_H */
