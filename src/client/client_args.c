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
        "Usage: %s [HOST] [PORT] [OPTION] [ARGS...] -u [USR:PASS]\n"
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

static int parse_password(struct ClientArgs* out, char* usr_pass){
     static char buffer[256];
    strncpy(buffer, usr_pass, sizeof(buffer));
    buffer[sizeof(buffer)-1] = '\0';

    char* token = strtok(buffer, ":");
    if (!token) return -1;
    out->user = token;

    token = strtok(NULL, ":");
    if (!token) return -1;
    out->password = token;

    return 0;
}

int parse_client_args(int argc, char* argv[], struct ClientArgs* out) {

    out->usr_count = 0;

    for (int k = 0; k < argc; k++) {
   
        if (strcmp(argv[k], "-u") == 0) {
            if (k + 1 >= argc) {
                fprintf(stderr, "ERROR: -u necesita un usuario.\n");
                return -1;
            }
            if (out->usr_count >= 32) {
                fprintf(stderr, "ERROR: demasiados -u.\n");
                return -1;
            }
            parse_password(out, argv[k+1]);

            argv[k]   = NULL;
            argv[k+1] = NULL;
            break;
        }
    }

    if (argc == 1) {
        fprintf(stderr, "No arguments provided.\n\n");
        print_help(argv[0]);
        return -1;
    }

    if (argc == 2 && strcmp(argv[1], "-h") == 0) {
        print_help(argv[0]);
        return 1;
    }

    if (argc < 5) {
        fprintf(stderr, "ERROR: HOST, OPTION and AUTH must be specified.\n\n");
        print_help(argv[0]);
        return -1;
    }

    int i = 1;

    while (i < argc && argv[i] == NULL) {
        i++;
    }

    if (i >= argc) {
        fprintf(stderr, "ERROR: HOST not found.\n\n");
        print_help(argv[0]);
        return -1;
    }
    out->host = argv[i++];

    while (i < argc && argv[i] == NULL) {
        i++;
    }

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

    // Skip NULL entries
    while (i < argc && argv[i] == NULL) {
        i++;
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

<<<<<<< HEAD
    if (argc - i < nArgs) {
        fprintf(stderr,
            "ERROR: Option '%s' needs %d argument(s), but only %d provided.\n\n",
            out->option, nArgs, argc - i);
=======
    int remaining = 0;
    for (int j = i; j < argc; j++) {
        if (argv[j] != NULL) remaining++;
    }

    if (remaining < n_args) {
        fprintf(stderr,
            "ERROR: Option '%s' needs %d argument(s), but only %d provided.\n\n",
            out->option, n_args, remaining);
>>>>>>> 69db260 (feat(client): added request functions)
        print_help(argv[0]);
        return -1;
    }

<<<<<<< HEAD
    // store extra arguments
    out->arg1 = (nArgs >= 1 ? argv[i] : NULL);
    out->arg2 = (nArgs >= 2 ? argv[i+1] : NULL);
    out->arg3 = (nArgs >= 3 ? argv[i+2] : NULL);
=======
    out->arg1 = NULL;
    out->arg2 = NULL;
    out->arg3 = NULL;

    int arg_count = 0;
    for (int j = i; j < argc && arg_count < n_args; j++) {
        if (argv[j] != NULL) {
            if (arg_count == 0) out->arg1 = argv[j];
            else if (arg_count == 1) out->arg2 = argv[j];
            else if (arg_count == 2) out->arg3 = argv[j];
            arg_count++;
        }
    }
>>>>>>> 69db260 (feat(client): added request functions)

    return 0;
}