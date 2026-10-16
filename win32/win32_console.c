/*
 * win32_console.c - Console setup and cleanup for Windows terminal environments.
 */

#include "win32_console.h"
#include <windows.h>
#include <stdio.h>
#ifdef _DEBUG
#include <crtdbg.h>
#endif
#include <stdlib.h>

static UINT original_cp = 0;
static UINT original_output_cp = 0;
static DWORD original_out_mode = 0;
static DWORD original_in_mode = 0;
static int console_initialized = 0;

static void quiet_invalid_parameter_handler(
    const wchar_t *expression,
    const wchar_t *function,
    const wchar_t *file,
    unsigned int line,
    uintptr_t pReserved)
{
    (void)expression;
    (void)function;
    (void)file;
    (void)line;
    (void)pReserved;
}

static BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
{
    switch (ctrl_type) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
        /* Handled gracefully inside PDCurses/nano */
        return TRUE;
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        win32_console_cleanup();
        return FALSE;
    default:
        return FALSE;
    }
}

void win32_console_init(void)
{
    HANDLE hOut;
    HANDLE hIn;
    DWORD mode;

    if (console_initialized)
        return;

    _set_invalid_parameter_handler(quiet_invalid_parameter_handler);

#ifdef _DEBUG
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif

    original_cp = GetConsoleCP();
    original_output_cp = GetConsoleOutputCP();

    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &original_out_mode)) {
        mode = original_out_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT;
        SetConsoleMode(hOut, mode);
    }

    hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hIn != INVALID_HANDLE_VALUE && GetConsoleMode(hIn, &original_in_mode)) {
        mode = original_in_mode | ENABLE_VIRTUAL_TERMINAL_INPUT;
        SetConsoleMode(hIn, mode);
    }

    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
    atexit(win32_console_cleanup);
    console_initialized = 1;
}

void win32_console_cleanup(void)
{
    HANDLE hOut;
    HANDLE hIn;

    if (!console_initialized)
        return;

    if (original_cp)
        SetConsoleCP(original_cp);
    if (original_output_cp)
        SetConsoleOutputCP(original_output_cp);

    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE && original_out_mode)
        SetConsoleMode(hOut, original_out_mode);

    hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hIn != INVALID_HANDLE_VALUE && original_in_mode)
        SetConsoleMode(hIn, original_in_mode);

    console_initialized = 0;
}
