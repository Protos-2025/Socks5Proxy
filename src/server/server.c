#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include "../shared/include/selector.h"
#include "include/socks5nio.h"
#include "include/defines.h"

#define PORT 1080
#define MAX_PENDING_CONNECTIONS 20
#define SELECTOR_CAPACITY 1024

#define GET_BUFFER_IDX(i, n) (((i) + (n)) == BUFFER_SIZE ? 0 : ((i) + (n)))
#define GET_BUFFER_MAX_IDX_TO_READ(w) ((w) == 0 ? BUFFER_SIZE : (w))

static void signal_handler(const int signal);

typedef struct custom_key {
    uint8_t * buffer;
    uint32_t idx_to_read;
    uint32_t idx_to_write;
    struct custom_key * other_party;
} custom_key;

static int server;
static bool done = false;

int main(const int argc, const char **argv) {
    unsigned port;

    if(argc == 1) {
        port = PORT;
    } else if(argc == 2) {
        char * end = 0;
        const long sl = strtol(argv[1], &end, 10);

        if (end == argv[1]|| '\0' != *end 
           || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
           || sl < 0 || sl > USHRT_MAX) {
            fprintf(stderr, "port should be an integer: %s\n", argv[1]);
            return 1;
        }
        port = sl;
    } else {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    close(STDIN_FILENO);

    const char * error_msg = NULL;
    fd_selector selector = NULL;
    selector_status ss = SELECTOR_SUCCESS;


    // <---------------------------- create proxy server socket ---------------------------->
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(port);

    if ((server = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        error_msg = "unable to create socket";
        goto finally;
    }

    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

    if (bind(server, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        error_msg = "unable to bind socket";
        goto finally;
    }

    if (listen(server, MAX_PENDING_CONNECTIONS) < 0) {
        error_msg = "unable to listen";
        goto finally;
    }

    fprintf(stdout, "Proxy server listening on TCP port %d\n", port);

    signal(SIGTERM, signal_handler);
    signal(SIGINT,  signal_handler);


    // <--------------------------------- configure selector --------------------------------->
    if (selector_fd_set_nio(server) == -1) {
        error_msg = "getting server socket flags";
        goto finally;
    }

    const struct selector_init conf = {
        .signal = SIGALRM,
        .select_timeout = {
            .tv_sec  = 10,
            .tv_nsec = 0,
        },
    };

    if(0 != selector_init(&conf)) {
        error_msg = "initializing selector";
        goto finally;
    }

    selector = selector_new(SELECTOR_CAPACITY);

    if (selector == NULL) {
        error_msg = "unable to create selector";
        goto finally;
    }

    const struct fd_handler socksv5 = {
        .handle_read = socksv5_passive_accept,
        .handle_write = NULL,
        .handle_close = NULL, // nada que liberar
    };

    ss = selector_register(selector, server, &socksv5, OP_READ, NULL);

    if (ss != SELECTOR_SUCCESS) {
        error_msg = "registering fd";
        goto finally;
    }

    // <------------------------------------ execute server ------------------------------------>
    while (!done) {
        error_msg = NULL;
        ss = selector_select(selector);
        if(ss != SELECTOR_SUCCESS) {
            error_msg = "serving";
            goto finally;
        }
    }
    if (error_msg == NULL) {
        error_msg = "closing";
    }

    int ret = 0;

finally:
    if (ss != SELECTOR_SUCCESS) {
        fprintf(stderr, "%s: %s\n", (error_msg == NULL) ? "": error_msg, ss == SELECTOR_IO ? strerror(errno) : selector_error(ss));
        ret = 2;
    } else if (error_msg) {
        perror(error_msg);
        ret = 1;
    }

    if (selector != NULL) {
        selector_destroy(selector);
    }

    selector_close();

    if (server >= 0) {
        close(server);
    }

    return ret;
}

static void signal_handler(const int signal) {
    printf("signal %d, cleaning up and exiting\n", signal);
    done = true;
}
