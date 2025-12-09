#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "client_args.h"

static int is_number(const char* s) {
    if (s == NULL || *s == '\0')
        return 0;

    for (int i = 0; s[i]; i++)
        if (!isdigit((unsigned char)s[i]))
            return 0;

    return 1;
}

void print_help(const char* prog) {
    fprintf(stderr,
        "Usage: %s [HOST] [PORT] [OPTION] [ARGS...]\n"
        "\n"
        "If PORT is not specified, default 4242 is used.\n"
        "\n"
        "Commands:\n"
        "   -h,                                      Print help and exit.\n"
        "   users                                    Request list of users.\n"
        "   add-user <username> <password> <role>    Add a user.\n"
        "   remove-user <username>                   Remove a user.\n"
        "   change-password <username> <password>    Change password.\n"
        "   change-rol <username> <role>             Change role.\n"
        "   metrics                                  Get server metrics.\n"
        "\n",
        prog
    );
}

static int need_args(const char* opt) {
    if (strcmp(opt, "users") == 0) return 0;
    if (strcmp(opt, "metrics") == 0) return 0;
    if (strcmp(opt, "remove-user") == 0) return 1;
    if (strcmp(opt, "change-rol") == 0) return 2;
    if (strcmp(opt, "change-password") == 0) return 2;
    if (strcmp(opt, "add-user") == 0) return 3;
    return -1; // unknown
}

int parse_client_args(int argc, char* argv[], struct ClientArgs* out) {

    if (argc == 1) {
        fprintf(stderr, "No arguments provided.\n\n");
        print_help(argv[0]);
        return -1;
    }

    if (argc == 2 && strcmp(argv[1], "-h") == 0) {
        print_help(argv[0]);
        return 1;
    }

    if (argc < 3) {
        fprintf(stderr, "ERROR: HOST and OPTION must be specified.\n\n");
        print_help(argv[0]);
        return -1;
    }

    int i = 1;

    // host
    out->host = argv[i++];

    // port (optional)
    if (i < argc && is_number(argv[i])) {
        long p = strtol(argv[i], NULL, 10);
        if (p < 1 || p > 65535) {
            fprintf(stderr, "ERROR: invalid port %s\n", argv[i]);
            return -1;
        }
        out->port = (unsigned short)p;
        i++;
    } else {
        out->port = 4242;
    }

    if (i >= argc) {
        fprintf(stderr, "ERROR: Missing OPTION.\n\n");
        print_help(argv[0]);
        return -1;
    }

    out->option = argv[i++];

    int nArgs = need_args(out->option);
    if (nArgs < 0) {
        fprintf(stderr, "ERROR: Unknown option '%s'.\n\n", out->option);
        print_help(argv[0]);
        return -1;
    }

    if (argc - i < nArgs) {
        fprintf(stderr,
            "ERROR: Option '%s' needs %d argument(s), but only %d provided.\n\n",
            out->option, nArgs, argc - i);
        print_help(argv[0]);
        return -1;
    }

    // store extra arguments
    out->arg1 = (nArgs >= 1 ? argv[i] : NULL);
    out->arg2 = (nArgs >= 2 ? argv[i+1] : NULL);
    out->arg3 = (nArgs >= 3 ? argv[i+2] : NULL);

    return 0;
}