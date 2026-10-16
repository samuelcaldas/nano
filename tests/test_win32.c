/*
 * test_win32.c - Verification tests for GNU nano Win32 port shims.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>

#include "win32_compat.h"
#include "win32_path.h"
#include "win32_console.h"
#include "dirent.h"
#include "libgen.h"
#include "getopt.h"
#include "glob.h"
#include "wcwidth.h"
#include "regex.h"

static void test_utf8_conversions(void)
{
    const char *orig = "Texto em português: á, é, í, ó, ú, ç, ã, õ. Emoji: 🚀";
    wchar_t *wstr = win32_utf8_to_wchar(orig);
    assert(wstr != NULL);

    char *roundtrip = win32_wchar_to_utf8(wstr);
    assert(roundtrip != NULL);
    assert(strcmp(orig, roundtrip) == 0);

    free(wstr);
    free(roundtrip);
    printf("[PASS] test_utf8_conversions\n");
}

static void test_path_normalization(void)
{
    char p1[] = "C:\\Users\\samuel\\source\\repos\\nano\\file.txt";
    win32_normalize_path(p1);
    assert(strcmp(p1, "C:/Users/samuel/source/repos/nano/file.txt") == 0);

    char dir_buf[] = "C:/Users/samuel/test.txt";
    char *dir = win32_dirname(dir_buf);
    assert(strcmp(dir, "C:/Users/samuel") == 0);

    char base_buf[] = "C:/Users/samuel/test.txt";
    char *base = win32_basename(base_buf);
    assert(strcmp(base, "test.txt") == 0);

    printf("[PASS] test_path_normalization\n");
}

static void test_file_io_and_stat(void)
{
    const char *filename = "test_utf8_arquivo_ãó.txt";
    const char *content = "Linha 1: Olá Mundo!\nLinha 2: Teste de acentuação.\n";

    FILE *fp = win32_fopen(filename, "wb");
    assert(fp != NULL);
    size_t written = fwrite(content, 1, strlen(content), fp);
    assert(written == strlen(content));
    fclose(fp);

    struct stat st;
    int res = win32_stat(filename, &st);
    assert(res == 0);
    assert(st.st_size == (off_t)strlen(content));
    assert(S_ISREG(st.st_mode));
    assert(!S_ISDIR(st.st_mode));

    assert(win32_access(filename, R_OK) == 0);

    /* Test getline */
    fp = win32_fopen(filename, "rb");
    assert(fp != NULL);
    char *line = NULL;
    size_t n = 0;
    ssize_t len = getline(&line, &n, fp);
    assert(len > 0);
    assert(strcmp(line, "Linha 1: Olá Mundo!\n") == 0);
    free(line);
    fclose(fp);

    res = win32_unlink(filename);
    assert(res == 0);

    printf("[PASS] test_file_io_and_stat\n");
}

static void test_globbing(void)
{
    glob_t g;
    int res = glob("syntax/*.nanorc", 0, NULL, &g);
    assert(res == 0);
    assert(g.gl_pathc > 10);
    size_t count = g.gl_pathc;
    globfree(&g);

    printf("[PASS] test_globbing (matched %zu syntax files)\n", count);
}

static void test_wcwidth(void)
{
    /* ASCII: 1 column */
    assert(mk_wcwidth(L'a') == 1);
    assert(mk_wcwidth(L'Z') == 1);

    /* Accented characters: 1 column */
    assert(mk_wcwidth(L'á') == 1);
    assert(mk_wcwidth(L'ç') == 1);

    /* CJK: 2 columns */
    assert(mk_wcwidth(0x4E16) == 2); /* '世' */
    assert(mk_wcwidth(0x754C) == 2); /* '界' */

    printf("[PASS] test_wcwidth\n");
}

static void test_pcre2_posix_regex(void)
{
    regex_t re;
    regmatch_t match[2];
    const char *pattern = "^int[[:blank:]]+([A-Za-z_][0-9A-Za-z_]*)";
    const char *text = "int main(void)";

    int rc = regcomp(&re, pattern, REG_EXTENDED);
    assert(rc == 0);

    rc = regexec(&re, text, 2, match, 0);
    assert(rc == 0);
    assert(match[1].rm_so == 4); /* "main" starts at index 4 */
    assert(match[1].rm_eo == 8); /* "main" ends at index 8 */

    regfree(&re);
    printf("[PASS] test_pcre2_posix_regex\n");
}

int main(void)
{
    printf("=== Running GNU nano Win32 Port Tests ===\n");
    test_utf8_conversions();
    test_path_normalization();
    test_file_io_and_stat();
    test_globbing();
    test_wcwidth();
    test_pcre2_posix_regex();
    printf("=== ALL TESTS PASSED! ===\n");
    return 0;
}
