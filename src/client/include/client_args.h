#ifndef CLIENT_ARGS_H
#define CLIENT_ARGS_H

struct ClientArgs {
    const char* host;
    unsigned short port;
    const char* option;
    const char* user;
    const char* password;
    int usr_count;

    // arguments following the option
    const char* arg1;
    const char* arg2;
    const char* arg3;
};

int parse_client_args(int argc, char* argv[], struct ClientArgs* out);
void print_help(const char* prog);

#endif