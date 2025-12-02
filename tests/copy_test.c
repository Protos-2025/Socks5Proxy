#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <signal.h>

#include "selector.h"
#include "socks5nio.h"
#include "copy.h"
#include "buffer.h"

#include "../src/server/logger.c"
#include "../src/server/copy.c"

/* Minimal no-op handler for selector registration */
static const struct fd_handler noop_handler = {
    .handle_read = NULL,
    .handle_write = NULL,
    .handle_block = NULL,
    .handle_close = NULL,
};

START_TEST(test_copy_read_write_flow)
{
    struct selector_init conf = { .signal = SIGUSR1, .select_timeout = {0, 0} };
    ck_assert_int_eq(selector_init(&conf), SELECTOR_SUCCESS);

    fd_selector sel = selector_new(16);
    ck_assert_ptr_nonnull(sel);

    /* create two socketpairs: one for client<->server endpoint, another for origin<->server endpoint */
    int client_pair[2];
    int origin_pair[2];
    ck_assert_int_eq(socketpair(AF_UNIX, SOCK_STREAM, 0, client_pair), 0);
    ck_assert_int_eq(socketpair(AF_UNIX, SOCK_STREAM, 0, origin_pair), 0);

    struct socks5 connection;
    /* initialize fds */
    connection.client_fd = client_pair[0];  /* server endpoint for client */
    connection.origin_fd = origin_pair[0];  /* server endpoint for origin */

    /* initialize buffers */
    buffer_init(&connection.client_buffer, BUFFER_SIZE, connection.client_buffer_data);
    buffer_init(&connection.origin_buffer, BUFFER_SIZE, connection.origin_buffer_data);

    /* Register both fds in selector so selector_set_interest will accept them */
    ck_assert_int_eq(selector_register(sel, connection.client_fd, &noop_handler, OP_NOOP, &connection), SELECTOR_SUCCESS);
    ck_assert_int_eq(selector_register(sel, connection.origin_fd, &noop_handler, OP_NOOP, &connection), SELECTOR_SUCCESS);

    /* Prepare key for arrival (selector passed into copy arrival) */
    struct selector_key key = { .s = sel, .fd = connection.client_fd, .data = &connection };

    /* Call arrival to wire up copy structures and initial interests */
    socksv5_copy_arrival(&key);

    /* The test: write bytes as if client sent them, call copy_read, then copy_write and verify origin receives them */
    const char *payload = "HELLO_COPY";
    size_t payload_len = strlen(payload);

    /* Write into the client peer socket (client_pair[1]) so data becomes readable on connection.client_fd */
    ssize_t written = write(client_pair[1], payload, payload_len);
    ck_assert_int_eq((ssize_t)payload_len, written);

    /* Simulate read event on client_fd */
    struct selector_key read_key = { .s = sel, .fd = connection.client_fd, .data = &connection };
    unsigned state_after_read = socksv5_copy_read(&read_key);
    (void) state_after_read; /* we only check buffers and forwarded data below */

    /* Now origin_buffer should contain the payload */
    ck_assert(buffer_can_read(&connection.origin_buffer));
    size_t nread = 0;
    uint8_t *ptr = buffer_read_ptr(&connection.origin_buffer, &nread);
    ck_assert_ptr_nonnull(ptr);
    ck_assert_uint_ge(nread, payload_len);
    ck_assert_mem_eq(ptr, payload, payload_len);

    /* Simulate write readiness on origin_fd to send data to origin */
    struct selector_key write_key = { .s = sel, .fd = connection.origin_fd, .data = &connection };

    unsigned state_after_write = socksv5_copy_write(&write_key);
    (void) state_after_write;

    /* Read from the origin peer socket (origin_pair[1]) the forwarded data */
    char recvbuf[128] = {0};
    ssize_t r = read(origin_pair[1], recvbuf, sizeof(recvbuf));
    ck_assert_int_eq((ssize_t)payload_len, r);
    ck_assert_mem_eq(recvbuf, payload, payload_len);

    /* cleanup fds and selector */
    close(client_pair[0]);
    close(client_pair[1]);
    close(origin_pair[0]);
    close(origin_pair[1]);

    selector_destroy(sel);
    selector_close();
}
END_TEST

Suite *suite(void) {
    Suite *s = suite_create("copy");
    TCase *tc = tcase_create("copy_basic");

    tcase_add_test(tc, test_copy_read_write_flow);
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    SRunner *sr = srunner_create(suite());
    int number_failed;

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
