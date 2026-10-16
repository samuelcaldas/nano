# GNU nano for Windows (Native Port)

A standalone native Windows port of [GNU nano](https://www.nano-editor.org/) (v8.2) compiled with MSVC using CMake and PDCursesMod.

## Features

- **Standalone Native Binary:** 100% native `nano.exe` (~540 KB). No Cygwin, MSYS2, or external POSIX DLL runtime dependencies.
- **UTF-8 & Unicode:** Full UTF-8 support for accented characters, Portuguese text, CJK characters, and emojis in both terminal display and filesystem paths.
- **Windows Terminal & Console:** ConPTY, ANSI/TrueColor color rendering, mouse navigation, and automatic dynamic terminal window resizing (`KEY_RESIZE`).
- **Syntax Highlighting:** Complete bundled syntax definitions (42+ languages including C, Python, JavaScript, Rust, Go, HTML, CSS, Markdown, YAML, etc.) using PCRE2 POSIX regex.
- **Windows Path Handling:** Native support for Windows drive letters (`C:`, `D:`), UNC paths, forward and backward slashes, and user configuration lookup.

---

## Prerequisites

- Windows 10/11 (x64)
- Visual Studio Build Tools (2019 or 2022) with C++ workload
- CMake (version 3.16+) & Ninja (included with Visual Studio Build Tools)
- Git for Windows

---

## Building

### Option A: Fast build with Ninja

Open **x64 Native Tools Command Prompt for VS** (or invoke `vcvars64.bat`):

```cmd
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Binary is output at `build\nano.exe`.

### Option B: Visual Studio / MSBuild

```cmd
cmake -B build -G "Visual Studio 16 2019" -A x64
cmake --build build --config Release
```

Binary is output at `build\Release\nano.exe`.

---

## Testing

Run the automated port test suite:

```cmd
ctest --test-dir build --output-on-failure
```

Or execute the test binary directly:

```cmd
.\build\test_win32.exe
```

---

## Configuration (`nanorc`)

On Windows, `nano` looks for configuration files in the following order:

1. Path specified via `-f <file>` / `--rcfile=<file>`
2. `%APPDATA%\nano\nanorc`
3. `%USERPROFILE%\.nanorc`

### Example `%USERPROFILE%\.nanorc`:

```nanorc
# Enable line numbers and mouse support
set linenumbers
set mouse

# Enable smooth scrolling and modern keybindings
set jumpyscrolling
set modernbindings

# Include syntax highlighting (replace path with your nano syntax directory)
include "C:/ProgramData/nano/syntax/*.nanorc"
```

To list all loaded syntaxes:

```cmd
nano.exe -z
```
