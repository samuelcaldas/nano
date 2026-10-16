cmake_minimum_required(VERSION 3.16)
project(nano VERSION 8.2 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8 /W3 /wd4005 /D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_DEPRECATE)
endif()

include(FetchContent)

# 1. Dependency: PCRE2 (POSIX regex support)
message(STATUS "Fetching PCRE2...")
set(PCRE2_BUILD_PCRE2_8 ON CACHE BOOL "" FORCE)
set(PCRE2_BUILD_PCRE2_16 OFF CACHE BOOL "" FORCE)
set(PCRE2_BUILD_PCRE2_32 OFF CACHE BOOL "" FORCE)
set(PCRE2_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(PCRE2_BUILD_PCRE2GREP OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(PCRE2_SUPPORT_UNICODE ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    pcre2
    GIT_REPOSITORY https://github.com/PCRE2Project/pcre2.git
    GIT_TAG        pcre2-10.44
)
FetchContent_MakeAvailable(pcre2)

# 2. Dependency: PDCursesMod (WinCon backend)
message(STATUS "Fetching PDCursesMod...")
FetchContent_Declare(
    pdcursesmod
    GIT_REPOSITORY https://github.com/Bill-Gray/PDCursesMod.git
    GIT_TAG        v4.5.4
)
FetchContent_GetProperties(pdcursesmod)
if(NOT pdcursesmod_POPULATED)
    FetchContent_Populate(pdcursesmod)
endif()

# Build PDCursesMod WinCon static library directly for maximum isolation & speed
file(GLOB PDCURSES_COMMON_SRCS "${pdcursesmod_SOURCE_DIR}/pdcurses/*.c")
file(GLOB PDCURSES_WINCON_SRCS "${pdcursesmod_SOURCE_DIR}/wincon/pdc*.c")
set(PDCURSES_SRCS ${PDCURSES_COMMON_SRCS} ${PDCURSES_WINCON_SRCS})

add_library(pdcurses STATIC ${PDCURSES_SRCS})
target_compile_definitions(pdcurses PUBLIC
    PDC_WIDE
    PDC_FORCE_UTF8
    _CRT_SECURE_NO_WARNINGS
)
target_include_directories(pdcurses PUBLIC
    ${pdcursesmod_SOURCE_DIR}
    ${pdcursesmod_SOURCE_DIR}/wincon
)
target_link_libraries(pdcurses PUBLIC user32 advapi32 winmm)

# Generate revision.h if not present
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/revision.h")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/src/revision.h" "#ifndef NANO_REVISION_H\n#define NANO_REVISION_H\n#define REVISION \"v8.2-windows\"\n#endif\n")
endif()

# Generate config.h
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.h.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
    @ONLY
)

# GNU nano sources
set(NANO_SRCS
    src/browser.c
    src/chars.c
    src/color.c
    src/cut.c
    src/files.c
    src/global.c
    src/help.c
    src/history.c
    src/move.c
    src/nano.c
    src/prompt.c
    src/rcfile.c
    src/search.c
    src/text.c
    src/utils.c
    src/winio.c
)

# Win32 compatibility sources
set(WIN32_PORT_SRCS
    win32/win32_compat.c
    win32/win32_path.c
    win32/win32_console.c
    win32/getopt.c
    win32/wcwidth.c
    win32/glob.c
)

add_executable(nano ${NANO_SRCS} ${WIN32_PORT_SRCS})

target_include_directories(nano PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/win32
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${pdcursesmod_SOURCE_DIR}
    ${pcre2_SOURCE_DIR}/src
    ${pcre2_BINARY_DIR}
)

target_compile_definitions(nano PRIVATE
    HAVE_CONFIG_H
    _WIN32
)

target_link_libraries(nano PRIVATE
    pdcurses
    pcre2-posix-static
    pcre2-8-static
    user32
    advapi32
    shell32
    winmm
)

# Tests
enable_testing()
add_executable(test_win32
    tests/test_win32.c
    ${WIN32_PORT_SRCS}
)
target_include_directories(test_win32 PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/win32
    ${pcre2_SOURCE_DIR}/src
    ${pcre2_BINARY_DIR}
)
target_compile_definitions(test_win32 PRIVATE _WIN32 HAVE_CONFIG_H)
target_link_libraries(test_win32 PRIVATE
    pcre2-posix-static
    pcre2-8-static
    user32
    advapi32
)
add_test(NAME test_win32 COMMAND test_win32)

# Copy syntax files to build directory
add_custom_target(copy_syntax ALL
    ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/syntax
    ${CMAKE_CURRENT_BINARY_DIR}/syntax
)
add_dependencies(nano copy_syntax)

# Install rules
install(TARGETS nano RUNTIME DESTINATION bin)
install(DIRECTORY syntax/ DESTINATION share/nano/syntax FILES_MATCHING PATTERN "*.nanorc")
