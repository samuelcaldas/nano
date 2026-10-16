/*
 * getopt.c - Clean, standalone getopt and getopt_long implementation for Windows.
 */

#include "getopt.h"
#include <stdio.h>
#include <string.h>

char *optarg = NULL;
int   optind = 1;
int   opterr = 1;
int   optopt = '?';

static char *next_char = NULL;

int getopt(int argc, char *const argv[], const char *optstring)
{
    char c;
    const char *opt_ptr;

    if (optind >= argc || argv[optind] == NULL)
        return -1;

    if (next_char == NULL || *next_char == '\0') {
        if (argv[optind][0] != '-' || argv[optind][1] == '\0')
            return -1;

        if (argv[optind][1] == '-' && argv[optind][2] == '\0') {
            optind++;
            return -1;
        }

        next_char = argv[optind] + 1;
    }

    c = *next_char++;
    opt_ptr = strchr(optstring, c);

    if (c == ':' || opt_ptr == NULL) {
        optopt = c;
        if (opterr && *optstring != ':')
            fprintf(stderr, "%s: unknown option -- '%c'\n", argv[0], c);
        return '?';
    }

    if (opt_ptr[1] == ':') {
        if (*next_char != '\0') {
            optarg = next_char;
            optind++;
        } else if (optind + 1 < argc) {
            optind++;
            optarg = argv[optind++];
        } else {
            optopt = c;
            if (opterr && *optstring != ':')
                fprintf(stderr, "%s: option requires an argument -- '%c'\n", argv[0], c);
            return (*optstring == ':') ? ':' : '?';
        }
        next_char = NULL;
    } else {
        if (*next_char == '\0') {
            optind++;
            next_char = NULL;
        }
    }

    return c;
}

int getopt_long(int argc, char *const argv[], const char *optstring,
                const struct option *longopts, int *longindex)
{
    const char *arg;
    const char *eq;
    size_t name_len;
    int match = -1;
    int i;

    if (optind >= argc || argv[optind] == NULL)
        return -1;

    arg = argv[optind];
    if (arg[0] != '-' || arg[1] != '-')
        return getopt(argc, argv, optstring);

    if (arg[2] == '\0') {
        optind++;
        return -1;
    }

    arg += 2;
    eq = strchr(arg, '=');
    name_len = eq ? (size_t)(eq - arg) : strlen(arg);

    for (i = 0; longopts && longopts[i].name != NULL; i++) {
        if (strncmp(longopts[i].name, arg, name_len) == 0 &&
            longopts[i].name[name_len] == '\0') {
            match = i;
            break;
        }
    }

    if (match == -1) {
        if (opterr)
            fprintf(stderr, "%s: unrecognized option '--%s'\n", argv[0], arg);
        optind++;
        return '?';
    }

    if (longindex)
        *longindex = match;

    optind++;

    if (longopts[match].has_arg == required_argument) {
        if (eq) {
            optarg = (char *)(eq + 1);
        } else if (optind < argc) {
            optarg = argv[optind++];
        } else {
            if (opterr)
                fprintf(stderr, "%s: option '--%s' requires an argument\n",
                        argv[0], longopts[match].name);
            return '?';
        }
    } else if (longopts[match].has_arg == optional_argument) {
        if (eq)
            optarg = (char *)(eq + 1);
        else
            optarg = NULL;
    } else {
        if (eq) {
            if (opterr)
                fprintf(stderr, "%s: option '--%s' doesn't allow an argument\n",
                        argv[0], longopts[match].name);
            return '?';
        }
        optarg = NULL;
    }

    if (longopts[match].flag) {
        *longopts[match].flag = longopts[match].val;
        return 0;
    }

    return longopts[match].val;
}
