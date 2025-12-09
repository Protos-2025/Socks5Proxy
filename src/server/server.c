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

#define GET_BUFFER_IDX(i, n) (((i) + (n)) == BUFFER_SIZE ? 0 : ((i) + (n)))
#define GET_BUFFER_MAX_IDX_TO_READ(w) ((w) == 0 ? BUFFER_SIZE : (w))

static void signal_handler(const int signal);
static int interpret_socket_args(struct socks5args args, struct sockaddr_in * addr);

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
    parse_args(argc, (char **) argv, &args);

    close(STDIN_FILENO);

    const char * errorMsg = NULL;
    FdSelector selector = NULL;
    SelectorStatus ss = SELECTOR_SUCCESS;


    // <---------------------------- create proxy server socket ---------------------------->
    struct sockaddr_storage addr;
	int addrlen = 0;

	if ((addrlen = interpret_socket_args(args, (struct sockaddr_in *)&addr)) < 0) {
        errorMsg = "interpreting socket arguments";
        goto finally;
    }

	LOG_DEBUG("Starting server...");

    if ((server = socket(((struct sockaddr_in *)&addr)->sin_family, SOCK_STREAM, 0)) < 0) {
        errorMsg = "unable to create socket";
        goto finally;
    }

    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

    if (bind(server, (struct sockaddr *) &addr, addrlen) < 0) {
        errorMsg = "unable to bind socket";
        goto finally;
    }

    if (listen(server, MAX_PENDING_CONNECTIONS) < 0) {
        errorMsg = "unable to listen";
        goto finally;
    }

    LOG_INFO("Proxy server listening on addr %s:%d (TCP)", args.socks_addr, args.socks_port);

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
    if (ss != SELECTOR_SUCCESS) {
        LOG_FATAL("%s: %s", (errorMsg == NULL) ? "": errorMsg, ss == SELECTOR_IO ? strerror(errno) : selector_error(ss));
        ret = 2;
    } else if (errorMsg) {
        LOG_FATAL("%s", errorMsg);
        ret = 1;
    }

    free_logger();

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

static int interpret_socket_args(struct socks5args args, struct sockaddr_in * addr) {
    int ipv6 = strchr(args.socks_addr, ':') != NULL;

    if (ipv6) {
        struct sockaddr_in6* socks6 = (struct sockaddr_in6*) addr;
        memset(socks6, 0, sizeof(struct sockaddr_in6));
		socks6->sin6_family = AF_INET6;
        socks6->sin6_addr = in6addr_any;
        socks6->sin6_port = htons(args.socks_port);
        if (inet_pton(AF_INET6, args.socks_addr, &socks6->sin6_addr) != 1) {
            LOG_FATAL("Invalid IPv6 address: %s", args.socks_addr);
			return -1;
		}
		return sizeof(struct sockaddr_in6);
	}

    struct sockaddr_in * socks4 = (struct sockaddr_in*) addr;
    memset(socks4, 0, sizeof(struct sockaddr_in));
    socks4->sin_family = AF_INET;
    socks4->sin_addr.s_addr = INADDR_ANY;
    socks4->sin_port = htons(args.socks_port);
    if (inet_pton(AF_INET, args.socks_addr, &socks4->sin_addr) != 1) {
        LOG_FATAL("Invalid IPv4 address: %s", args.socks_addr);
        return -1;
    }

	return sizeof(struct sockaddr_in);
}
