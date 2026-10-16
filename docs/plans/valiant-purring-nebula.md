# Plan: Native Windows Port of GNU nano (MSVC + CMake)

## Context
GNU nano is a widely adopted terminal text editor traditionally built on POSIX systems using Autotools and NCurses. On Windows, users typically have to run nano via heavy compatibility layers (Cygwin, MSYS2, Git Bash) or WSL, which require POSIX runtime DLLs or a virtualized Linux environment.

This project implements a **native Windows port of GNU nano** based on the official upstream GNU nano source repository, compiled with **MSVC** using **CMake** and **PDCursesMod**. The result is a fast, standalone `nano.exe` executable with zero external runtime DLL dependencies, full UTF-8 Unicode support, Windows Terminal / ConPTY color support, and clean Windows filesystem path integration.

---

## Architecture Overview

```
                        +---------------------------------------+
                        |           GNU nano core               |
                        |   (src/*.c - upstream unmodified or   |
                        |    minimal portability hooks)         |
                        +---------------------------------------+
                                    |               |
                                    v               v
            +-------------------------------+   +-------------------------------+
            |      PDCursesMod (WinCon)     |   |    win32/ Compatibility Layer |
            | - Curses screen rendering     |   | - win32_compat (CRT, stat,    |
            | - Windows Console / ConPTY    |   |   utime, truncate, stubs)     |
            | - Mouse, colors, wide chars   |   | - win32_path (UTF-8 <-> UTF-16|
            | - Terminal keyboard events    |   |   drive letters, config paths)|
            +-------------------------------+   | - win32_console (Ctrl+C, evs) |
                                                | - POSIX regex (TRE or PCRE2)  |
                                                +-------------------------------+
                                    |               |
                                    v               v
                        +---------------------------------------+
                        |      Windows Console / Windows OS     |
                        +---------------------------------------+
```

### Key Architectural Decisions
1. **Source Strategy:** Track upstream GNU nano (pinned to the latest stable release tag, e.g., `v8.2`) on an `upstream` branch, and develop Windows port changes on `master` (or a dedicated worktree). Upstream modifications are kept minimal and isolated behind standard preprocessor checks (`#ifdef _WIN32`).
2. **Terminal & UI Layer:** Use **PDCursesMod** configured for the Windows Console (`wincon`) backend. PDCursesMod handles screen buffer management, wide character display, mouse clicks, and terminal key decoding natively without requiring termios or terminfo.
3. **Build System:** A modern `CMakeLists.txt` replaces Autotools for the Windows build, using `FetchContent` to retrieve and build pinned versions of PDCursesMod and POSIX Regex (TRE) as static libraries linked into `nano.exe`.
4. **POSIX-to-Win32 Shims (`win32/`):**
   - `win32_compat.h` / `win32_compat.c`: Adapts missing CRT calls (`truncate`, `chmod`, `utime`, file attributes, error codes).
   - `win32_path.h` / `win32_path.c`: Translates UTF-8 paths to wide `wchar_t` paths for Windows APIs, handles drive letters (`C:\`), UNC paths, and config directory resolution (`%APPDATA%\nano\nanorc` and `%USERPROFILE%\.nanorc`).
   - `win32_console.h` / `win32_console.c`: Console signal handler (`SetConsoleCtrlHandler`) dispatching Ctrl+C and window events gracefully without unsafe signal interrupts.
   - `regex`: Embed a lightweight POSIX-compliant regex engine (TRE) providing `regex.h` for syntax highlighting and search/replace.

---

## Phased Implementation Plan

### Phase 1: Upstream Setup and Workspace Initialization
1. Initialize Git repository in `D:\Users\samuel\source\repos\nano`.
2. Add official GNU nano remote (`https://git.savannah.gnu.org/git/nano.git` or GitHub mirror `https://github.com/mirror/nano`).
3. Fetch and pin the latest stable release tag (e.g. `v8.2`).
4. Establish `upstream` tracking branch and create the port branch (`master`).
5. Set up `.gitignore` for CMake build artifacts (`build/`, `out/`, `CMakeUserPresets.json`, `*.exe`, `*.lib`).

### Phase 2: CMake Build System and Dependencies
1. Create `CMakeLists.txt`:
   - Enforce C11 standard and MSVC `/utf-8` compiler flags.
   - Configure `FetchContent` for **PDCursesMod** (`wincon` backend, wide-char/UTF-8 enabled).
   - Configure `FetchContent` for **TRE** (or PCRE2 POSIX) for POSIX `regex.h` compliance.
2. Create `cmake/config.h.cmake.in`:
   - Generate `config.h` defining macros expected by GNU nano (`HAVE_CURSES_H`, `HAVE_NCURSES_H`, `ENABLE_UTF8`, `ENABLE_COLOR`, `ENABLE_NANORC`, `ENABLE_JUSTIFY`, `ENABLE_HISTORIES`).
   - Define portability constants and include `win32/win32_compat.h` when on `_WIN32`.
3. Verify CMake configuration generates cleanly using Ninja or MSBuild under `vcvars64.bat`.

### Phase 3: Win32 Compatibility & Path Shims
1. Implement `win32/win32_compat.h` and `win32/win32_compat.c`:
   - Map POSIX types: `mode_t`, `uid_t`, `gid_t`, `pid_t`, `ssize_t`.
   - Provide safe implementations of `ftruncate`, `truncate`, `fsync` (`_commit`), `getuid` / `geteuid` (stubs), `getpwuid` (stubs).
   - Provide file stat and timestamp shims (`_wstat64`, `_wutime64`).
2. Implement `win32/win32_path.h` and `win32/win32_path.c`:
   - Bidirectional UTF-8 `<->` UTF-16 path conversions via `MultiByteToWideChar` / `WideCharToMultiByte`.
   - Windows path normalization (handling both `/` and `\` separators, drive letters `C:`).
   - Discovery functions for nanorc configuration files:
     1. `%APPDATA%\nano\nanorc`
     2. `%USERPROFILE%\.nanorc`
     3. Local directory `.nanorc`
3. Implement `win32/win32_console.h` and `win32/win32_console.c`:
   - Console event handler (`SetConsoleCtrlHandler`) catching `CTRL_C_EVENT` and `CTRL_CLOSE_EVENT` and setting safe event flags for the main loop.
   - Safe terminal cleanup hook to restore console state on abrupt exit.

### Phase 4: Core GNU Nano Integration & Compilation
1. Configure source inclusion in `CMakeLists.txt` for `src/*.c` (e.g. `src/nano.c`, `src/winio.c`, `src/files.c`, `src/search.c`, `src/cut.c`, `src/color.c`, `src/prompt.c`, `src/move.c`, `src/text.c`, `src/utils.c`).
2. Adjust conditional compilation points in nano sources where platform-specific POSIX behaviors differ:
   - Terminal resizing: rely on PDCurses `KEY_RESIZE` instead of POSIX `SIGWINCH`.
   - Terminal raw mode: delegate to PDCurses `raw()` / `cbreak()` instead of POSIX `termios`.
   - File locking: adapt or gracefully bypass POSIX `fcntl(F_SETLK)` with Windows `LockFileEx` or no-op.
3. Successfully compile and link `nano.exe`.

### Phase 5: Functional Verification & Polish
1. **Interactive TUI Testing:**
   - Launch `nano.exe` in Windows Terminal, PowerShell, and legacy Command Prompt.
   - Verify screen dimensions, window resizing, and status bar rendering.
2. **Keyboard & Shortcut Verification:**
   - Test critical nano shortcuts: `Ctrl+O` (WriteOut), `Ctrl+X` (Exit), `Ctrl+W` (Search), `Ctrl+K`/`Ctrl+U` (Cut/Uncut), `Ctrl+G` (Help).
   - Verify Alt key sequences / Meta key combinations via PDCurses.
3. **File I/O & Line Endings:**
   - Test opening, modifying, and saving files with CRLF (`\r\n`) and LF (`\n`). Verify line ending preservation without file corruption.
   - Test files on various drive letters (`C:\...`, `D:\...`) and paths with spaces.
4. **Unicode & UTF-8:**
   - Test typing, displaying, and navigating multi-byte characters (accented characters, Portuguese text, CJK characters, emojis).
5. **Syntax Highlighting & nanorc:**
   - Verify loading syntax files from bundled or user nanorc directory.
   - Verify regex matching in syntax highlighting and interactive search/replace.

---

## Critical Files

| File Path | Description |
|---|---|
| `CMakeLists.txt` | Top-level CMake build script, toolchain configuration, dependency fetch rules |
| `cmake/config.h.cmake.in` | Feature configuration template replacing GNU Autotools config.h |
| `win32/win32_compat.h` | Header declaring Windows POSIX CRT compatibility functions and types |
| `win32/win32_compat.c` | Implementation of stat, truncate, file attributes, and CRT shims |
| `win32/win32_path.h` | Header for UTF-8/UTF-16 path handling and config discovery |
| `win32/win32_path.c` | Path conversion and nanorc lookup implementation |
| `win32/win32_console.h` | Header for Windows console control event handling |
| `win32/win32_console.c` | Console control handler implementation |
| `src/winio.c` / `src/nano.c` | Core nano source files interfacing with curses and the main event loop |

---

## Verification & Testing Guide

### 1. Build Verification
```powershell
# Set up MSVC environment
& "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

# Configure and build via CMake
cmake -B build -G "Visual Studio 16 2019" -A x64
cmake --build build --config Release

# Verify standalone binary and inspect DLL dependencies (should have NO Cygwin/MSYS DLLs)
dumpbin /dependents build\Release\nano.exe
```

### 2. End-to-End Functionality Checklist
- [ ] **Startup & Exit:** Run `.\build\Release\nano.exe test.txt`. Type text, press `Ctrl+O` to write, `Ctrl+X` to exit. Confirm `test.txt` contains expected content.
- [ ] **Line Endings:** Open a file with LF endings and save; verify no unsolicited conversion to CRLF. Open a CRLF file and save; verify CRLF is preserved.
- [ ] **Console Resizing:** Resize the terminal window while editing; verify the editor repaints correctly without crash or layout corruption.
- [ ] **Unicode / Accents:** Type Portuguese characters (`á, ç, ã, é, ó`) and multi-byte UTF-8 strings. Confirm cursor position and character rendering align accurately.
- [ ] **Regex & Syntax Highlighting:** Configure a syntax rule in `%USERPROFILE%\.nanorc` and open a source file. Confirm keyword coloring and regex search (`Ctrl+W`) work properly.
