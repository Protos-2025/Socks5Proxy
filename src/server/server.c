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
#include "include/pam.h"
#include "include/defines.h"
#include "include/metrics.h"
#include "include/users.h"
#include "logger.h"
#include "args.h"
#include "metrics.h"

static void signal_handler(const int signal);
static int interpret_socket_args(struct sockaddr_storage * addr_storage, char * addr, unsigned short port);

typedef struct custom_key {
    uint8_t * buffer;
    uint32_t idx_to_read;
    uint32_t idx_to_write;
    struct custom_key * other_party;
} CustomKey;

static bool done = false;

int main(const int argc, const char ** argv) {
    close(STDIN_FILENO);
    int server = -1, pamServer = -1;

    
    // <--------------------------------- configure selector --------------------------------->
    const char * errorMsg = NULL;
    FdSelector selector = NULL;
    SelectorStatus ss = SELECTOR_SUCCESS;

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


    metrics_init();
    users_init();
    logger_init();
    if (logger_register_selector(selector) < 0) {
		errorMsg = "initializing logger";
		goto finally;
    }

    struct socks5args args;
    parse_args(argc, (char **) argv, &args);

    
    // <----------------------------------- create users ----------------------------------->
    for (int i = 0; i < args.nusers; i++) {
        if (user_create((const uint8_t *)args.users[i].name, (const uint8_t *)args.users[i].pass, USER_PRIVILEGE_DEFAULT) != USER_OK) {
            errorMsg = "creating users";
		    goto finally;
        }
    }
    
    
    // <---------------------------- create proxy server socket ---------------------------->
    struct sockaddr_storage addr;
	socklen_t addrlen = 0;

	if ((addrlen = interpret_socket_args(&addr, args.socks_addr, args.socks_port)) < 0) {
        errorMsg = "interpreting socket arguments";
        goto finally;
    }

	LOG_DEBUG("Starting server...");

    if ((server = socket(addr.ss_family, SOCK_STREAM, IPPROTO_TCP)) < 0) {
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

    if (selector_fd_set_nio(server) == -1) {
        errorMsg = "getting server socket flags";
        goto finally;
    }

    LOG_INFO("Proxy server listening on addr %s:%d (TCP)", args.socks_addr, args.socks_port);


	// <---------------------------- create pam server socket ---------------------------->    
    if ((addrlen = interpret_socket_args(&addr, args.mng_addr, args.mng_port)) < 0) {
        errorMsg = "interpreting pam socket arguments";
        goto finally;
    }
    
    LOG_DEBUG("Starting pam server...");

	if ((pamServer = socket(addr.ss_family, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		errorMsg = "unable to create pam socket";
		goto finally;
	}

	setsockopt(pamServer, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

	if (bind(pamServer, (struct sockaddr *) &addr, addrlen) < 0) {
		errorMsg = "unable to bind pam socket";
		goto finally;
	}

	if (listen(pamServer, MAX_PENDING_CONNECTIONS) < 0) {
		errorMsg = "unable to listen on pam server";
		goto finally;
	}

    if (selector_fd_set_nio(pamServer) == -1) {
        errorMsg = "getting pam server socket flags";
        goto finally;
    }

	LOG_INFO("Pam server listening on addr %s:%d (TCP)", args.mng_addr, args.mng_port);


	// <----------------------------------- setup signals ----------------------------------->
    signal(SIGTERM, signal_handler);
    signal(SIGINT,  signal_handler);
	signal(SIGQUIT, signal_handler);
    signal(SIGABRT, signal_handler);


    // <--------------------------------- register handlers --------------------------------->
    const struct fd_handler socksv5 = {
        .handle_read = socksv5_passive_accept,
        .handle_write = NULL,
        .handle_close = NULL, // nada que liberar
    };
    const struct fd_handler pam = {
        .handle_read = pam_passive_accept,
        .handle_write = NULL,
        .handle_close = NULL,
    };

    ss = selector_register(selector, server, &socksv5, OP_READ, NULL);

    if (ss != SELECTOR_SUCCESS) {
        errorMsg = "registering socks fd";
        goto finally;
    }

    ss = selector_register(selector, pamServer, &pam, OP_READ, NULL);

    if (ss != SELECTOR_SUCCESS) {
        errorMsg = "registering pam fd";
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

    users_free();
    free_logger();

    if (selector != NULL) {
        selector_destroy(selector);
    }

    selector_close();

    if (server >= 0) {
        close(server);
    }

    if (pamServer >= 0) {
        close(pamServer);
    }

    return ret;
}

static void signal_handler(const int signal) {
    struct metricSnapshot snapshot;
    get_metrics_snapshot(&snapshot);
    LOG_FATAL("Signal %d, cleaning up and exiting.\nMetrics:\n\tBytes transferred\n\tSent: %zu\n\tReceived: %zu.\n\tTotal connections: %zu", signal, snapshot.totalBytesSent, snapshot.totalBytesReceived, snapshot.totalConnections);
    flush_all_logs();
    free_logger();
    done = true;
}

static int interpret_socket_args(struct sockaddr_storage * addr_storage, char * addr, unsigned short port) {
    int ipv6 = strchr(addr, ':') != NULL;

    if (ipv6) {
        struct sockaddr_in6 * socks6 = (struct sockaddr_in6 *) addr_storage;
        memset(socks6, 0, sizeof(struct sockaddr_in6));
		socks6->sin6_family = AF_INET6;
        socks6->sin6_addr = in6addr_any;
        socks6->sin6_port = htons(port);
        if (inet_pton(AF_INET6, addr, &socks6->sin6_addr) != 1) {
            LOG_FATAL("Invalid IPv6 address: %s", addr);
			return -1;
		}
		return sizeof(struct sockaddr_in6);
	}

    struct sockaddr_in * socks4 = (struct sockaddr_in *) addr_storage;
    memset(socks4, 0, sizeof(struct sockaddr_in));
    socks4->sin_family = AF_INET;
    socks4->sin_addr.s_addr = INADDR_ANY;
    socks4->sin_port = htons(port);
    if (inet_pton(AF_INET, addr, &socks4->sin_addr) != 1) {
        LOG_FATAL("Invalid IPv4 address: %s", addr);
        return -1;
    }

	return sizeof(struct sockaddr_in);
}
