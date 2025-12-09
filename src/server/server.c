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
#include "include/metrics.h"
#include "logger.h"
#include "args.h"

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
} CustomKey;

static int server;
static bool done = false;

int main(const int argc, const char **argv) {
	logger_init();
    metrics_init();

    struct socks5args args;
    parse_args(argc, argv, &args);

    close(STDIN_FILENO);

    const char * errorMsg = NULL;
    FdSelector selector = NULL;
    SelectorStatus ss = SELECTOR_SUCCESS;


    // <---------------------------- create proxy server socket ---------------------------->
    LOG_DEBUG("Starting server...");
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;

    if (inet_addr(args.socks_addr) != INADDR_NONE) {
        addr.sin_addr.s_addr = inet_addr(args.socks_addr);
    } else {
        LOG_FATAL("Invalid address provided: %s", args.socks_addr);
        return 1;
    }

    addr.sin_port        = htons(args.socks_port);
    if (inet_pton(AF_INET, args.socks_addr, &addr.sin_addr) != 1) {
        LOG_FATAL("Failed IP conversion for IPv4");
        return 1;
    }

    if ((server = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        errorMsg = "unable to create socket";
        goto finally;
    }

    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

    if (bind(server, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        errorMsg = "unable to bind socket";
        goto finally;
    }

    if (listen(server, MAX_PENDING_CONNECTIONS) < 0) {
        errorMsg = "unable to listen";
        goto finally;
    }

    LOG_INFO("Proxy server listening on TCP port %d", args.socks_port);

    signal(SIGTERM, signal_handler);
    signal(SIGINT,  signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGABRT, signal_handler);
    
    // <--------------------------------- configure selector --------------------------------->
    if (selector_fd_set_nio(server) == -1) {
        errorMsg = "getting server socket flags";
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
        errorMsg = "initializing selector";
        goto finally;
    }

    selector = selector_new(SELECTOR_CAPACITY);

    if (selector == NULL) {
        errorMsg = "unable to create selector";
        goto finally;
    }

    if (logger_register_selector(selector) < 0) {
		errorMsg = "initializing logger";
		goto finally;
    };

    const struct fd_handler socksv5 = {
        .handle_read = socksv5_passive_accept,
        .handle_write = NULL,
        .handle_close = NULL, // nada que liberar
    };

    ss = selector_register(selector, server, &socksv5, OP_READ, NULL);

    if (ss != SELECTOR_SUCCESS) {
        errorMsg = "registering fd";
        goto finally;
    }

    // <------------------------------------ execute server ------------------------------------>
    while (!done) {
        errorMsg = NULL;
        ss = selector_select(selector);
        if(ss != SELECTOR_SUCCESS) {
            errorMsg = "serving";
            goto finally;
        }
    }

    int ret = 0;

finally:
    free_logger();
    if (ss != SELECTOR_SUCCESS) {
        fprintf(stderr, "%s: %s\n", (errorMsg == NULL) ? "": errorMsg, ss == SELECTOR_IO ? strerror(errno) : selector_error(ss));
        ret = 2;
    } else if (errorMsg) {
        perror(errorMsg);
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
    LOG_FATAL("Signal %d, cleaning up and exiting\n", signal);
    flush_all_logs();
    free_logger();
    done = true;
}
