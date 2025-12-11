#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "client_args.h"

#define VERSION "1.0.0"

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
        "Usage: %s [OPTIONS] [COMMAND] [ARGS]\n"
        "\n"
        "Options:\n"
        "   -h                                       Print help and exit.\n"
        "   -v                                       Print version info and exit.\n"
        "   -L HOST                                  Specify remote host (default: 127.0.0.1).\n"
        "   -P PORT                                  Specify connection port (default: 8080).\n"
        "   -u USER:PASS                             Authentication credentials.\n"
        "\n"
        "Commands:\n"
        "   users                                    Request list of users.\n"
        "   metrics                                  Get server metrics.\n"
        "   add-user <username> <password> <role>    Add a user.\n"
        "   remove-user <username>                   Remove a user.\n"
        "   change-Password <username> <password>    Change password.\n"
        "   change-role <username> <role>            Change role.\n"
        "\n",
        prog
    );
}

static int need_args(const char* opt) {
    if (strcmp(opt, "users") == 0) return 0;
    if (strcmp(opt, "metrics") == 0) return 0;
    if (strcmp(opt, "remove-user") == 0) return 1;
    if (strcmp(opt, "change-role") == 0) return 2;
    if (strcmp(opt, "change-Password") == 0) return 2;
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
    out->port = 8080; // default port
    out->host = "127.0.0.1"; // default host

    if (argc == 1) {
        fprintf(stderr, "Error: No arguments provided.\n\n");
        print_help(argv[0]);
        return -1;
    }

    // Pre-Procesar optional flags
    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "-h") == 0) {
            print_help(argv[0]);
            return 1;
        }
        else if (strcmp(argv[i], "-v") == 0) {
            printf("Version: %s\n", VERSION);
            return 1;
        }
        else if (strcmp(argv[i], "-L") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ERROR: -L requires a host address.\n");
                return -1;
            }
            out->host = argv[i+1];
            argv[i] = NULL;
            argv[i+1] = NULL;
            i += 2;
        }
        else if (strcmp(argv[i], "-P") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ERROR: -P requires a port number.\n");
                return -1;
            }
            if (!is_number(argv[i+1])) {
                fprintf(stderr, "ERROR: -P requires a numeric port.\n");
                return -1;
            }
            long p = strtol(argv[i+1], NULL, 10);
            if (p < 1 || p > 65535) {
                fprintf(stderr, "ERROR: invalid port %s\n", argv[i+1]);
                return -1;
            }
            out->port = (unsigned short)p;
            argv[i] = NULL;
            argv[i+1] = NULL;
            i += 2;
        }
        else if (strcmp(argv[i], "-u") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ERROR: -u needs USER:PASS.\n");
                return -1;
            }
            if (out->usr_count >= 32) {
                fprintf(stderr, "ERROR: too many -u.\n");
                return -1;
            }
            if (parse_password(out, argv[i+1]) < 0) {
                fprintf(stderr, "ERROR: -u format must be USER:PASS.\n");
                return -1;
            }
            argv[i] = NULL;
            argv[i+1] = NULL;
            i += 2;
        }
        else {
            i++;
        }
    }

    // Search COMMAND
    i = 1;
    while (i < argc && argv[i] == NULL) {
        i++;
    }

    if (i >= argc) {
        fprintf(stderr, "ERROR: Missing COMMAND.\n\n");
        print_help(argv[0]);
        return -1;
    }

    out->option = argv[i++];

    int nArgs = need_args(out->option);
    if (nArgs < 0) {
        fprintf(stderr, "ERROR: Unknown command '%s'.\n\n", out->option);
        print_help(argv[0]);
        return -1;
    }

    // Count remaining arguments
    int remaining = 0;
    for (int j = i; j < argc; j++) {
        if (argv[j] != NULL) remaining++;
    }

    if (remaining < nArgs) {
        fprintf(stderr,
            "ERROR: Command '%s' needs %d argument(s), but only %d provided.\n\n",
            out->option, nArgs, remaining);
        print_help(argv[0]);
        return -1;
    }

    // Asignar arguments
    out->arg1 = NULL;
    out->arg2 = NULL;
    out->arg3 = NULL;

    int argCount = 0;
    for (int j = i; j < argc && argCount < nArgs; j++) {
        if (argv[j] != NULL) {
            if (argCount == 0) out->arg1 = argv[j];
            else if (argCount == 1) out->arg2 = argv[j];
            else if (argCount == 2) out->arg3 = argv[j];
            argCount++;
        }
    }

    return 0;
}