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

/* Mock recv/send before including implementation so copy.c uses them */
#include <sys/types.h>
#include <errno.h>
#include <assert.h>

enum { FAKE_CLIENT_FD = 5, FAKE_ORIGIN_FD = 6 };

/* Test-side peer buffers that mocked_recv/mock_send will use */
static uint8_t test_client_source[BUFFER_SIZE];
static uint8_t test_client_sink[BUFFER_SIZE];
static size_t test_client_source_len = 0;
static size_t test_client_source_pos = 0;
static size_t test_client_sink_len = 0;

static uint8_t test_origin_source[BUFFER_SIZE];
static uint8_t test_origin_sink[BUFFER_SIZE];
static size_t test_origin_source_len = 0;
static size_t test_origin_source_pos = 0;
static size_t test_origin_sink_len = 0;

static ssize_t mocked_recv(int fd, void *buf, size_t len, int flags) {
    (void)flags;
    if (fd == FAKE_CLIENT_FD) {
        size_t avail = 0;
        if (test_client_source_len > test_client_source_pos) {
            avail = test_client_source_len - test_client_source_pos;
        }
        if (avail == 0) return 0; /* EOF */
        size_t tocopy = (len < avail) ? len : avail;
        memcpy(buf, test_client_source + test_client_source_pos, tocopy);
        test_client_source_pos += tocopy;
        return (ssize_t)tocopy;
    } else if (fd == FAKE_ORIGIN_FD) {
        size_t avail = 0;
        if (test_origin_source_len > test_origin_source_pos) {
            avail = test_origin_source_len - test_origin_source_pos;
        }
        if (avail == 0) return 0; /* EOF */
        size_t tocopy = (len < avail) ? len : avail;
        memcpy(buf, test_origin_source + test_origin_source_pos, tocopy);
        test_origin_source_pos += tocopy;
        return (ssize_t)tocopy;
    } else {
        assert("Unexpected fd in mocked_recv" && 0);
    }
    /* no data for other fds */
    return 0;
}

static ssize_t mocked_send(int fd, const void *buf, size_t len, int flags) {
    (void)flags;
    if (fd == FAKE_ORIGIN_FD) {
        size_t free_space = BUFFER_SIZE - test_origin_sink_len;
        size_t tocopy = (len < free_space) ? len : free_space;
        if (tocopy > 0) {
            memcpy(test_origin_sink + test_origin_sink_len, buf, tocopy);
            test_origin_sink_len += tocopy;
            return (ssize_t)tocopy;
        }
        return 0;
    } else if (fd == FAKE_CLIENT_FD) {
        size_t free_space = BUFFER_SIZE - test_client_sink_len;
        size_t tocopy = (len < free_space) ? len : free_space;
        if (tocopy > 0) {
            memcpy(test_client_sink + test_client_sink_len, buf, tocopy);
            test_client_sink_len += tocopy;
            return (ssize_t)tocopy;
        }
        return 0;
    } else {
        assert("Unexpected fd in mocked_send" && 0);
    }
    return 0;
}

static void mock_log(int level, const char* file, int line, time_t * now, const char* msg, ...) {
    // Do nothing
	(void) level;
	(void)file;
    (void) line;
    (void) now;
    (void) msg;
	return;
}

/* make copy.c use mocks */
#define recv mocked_recv
#define send mocked_send

#define loggerLogMessageDeferred mock_log

#include "../src/server/states/copy.c"

// #undef recv
// #undef send

/**
 * @brief Read data from Client (FD=5)
 * 
 * And write it to the Origin (FD=6) using the copy state handlers.
 */
START_TEST(test_copy_read_write_flow)
{
    fd_selector sel = selector_new(16);
    ck_assert_ptr_nonnull(sel);

    struct socks5 connection;
    /* initialize fds to fake values */
    connection.client_fd = FAKE_CLIENT_FD;
	connection.origin_fd = FAKE_ORIGIN_FD;
	connection.client.copy = (copy_st){0};
	connection.origin_st.copy = (copy_st){0};
    connection.client_buffer = (buffer){0};
    connection.origin_buffer = (buffer){0};
    buffer_init(&connection.client_buffer, BUFFER_SIZE, connection.client_buffer_data);
    buffer_init(&connection.origin_buffer, BUFFER_SIZE, connection.origin_buffer_data);

    /* Prepare key for arrival (selector passed into copy arrival) */
    struct selector_key key = { .s = sel, .fd = connection.client_fd, .data = &connection };

    /* Call arrival to wire up copy structures and initial interests */
    socksv5_copy_arrival(sel, &key);

    /* The test: write bytes as if client sent them, call copy_read, then copy_write and verify origin receives them */
    const char *payload = "HELLO_COPY";
    size_t payload_len = strlen(payload);

    /* Seed the mocked client source buffer so recv on client fd will return payload */
    test_client_source_pos = 0;
    memcpy(test_client_source, payload, payload_len);
    test_client_source_len = payload_len;
    test_origin_sink_len = 0;

    /* Simulate read event on client_fd */
    struct selector_key read_key = { .s = sel, .fd = connection.client_fd, .data = &connection };
    unsigned state_after_read = socksv5_copy_read(&read_key);
    (void) state_after_read;

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

    /* Verify test_origin_sink captured the forwarded data */
    ck_assert_int_eq((ssize_t)payload_len, (ssize_t)test_origin_sink_len);
    ck_assert_mem_eq(test_origin_sink, payload, payload_len);

    /* cleanup selector */
    selector_destroy(sel);
    selector_close();
}
END_TEST

/**
 * @brief Read data from Client (FD=5)
 * And write it to the Origin (FD=6) using the copy state handlers.
 * Then read some data from Origin (FD=6) and write it back to Client (FD=5).
 */
START_TEST(test_copy_read_write_read_write_flow) {
    fd_selector sel = selector_new(16);
    ck_assert_ptr_nonnull(sel);

    struct socks5 connection;
    /* initialize fds to fake values */
    connection.client_fd = FAKE_CLIENT_FD;
    connection.origin_fd = FAKE_ORIGIN_FD;
    connection.client.copy = (copy_st){0};
    connection.origin_st.copy = (copy_st){0};
    connection.client_buffer = (buffer){0};
    connection.origin_buffer = (buffer){0};
    buffer_init(&connection.client_buffer, BUFFER_SIZE, connection.client_buffer_data);
    buffer_init(&connection.origin_buffer, BUFFER_SIZE, connection.origin_buffer_data);

    /* Prepare key for arrival (selector passed into copy arrival) */
    struct selector_key key = { .s = sel, .fd = connection.client_fd, .data = &connection };

    /* Call arrival to wire up copy structures and initial interests */
    socksv5_copy_arrival(sel, &key);

    /* The test: write bytes as if client sent them, call copy_read, then copy_write and verify origin receives them */
    const char *payload = "HELLO_COPY";
    size_t payload_len = strlen(payload);

    /* Seed the mocked client source buffer so recv on client fd will return payload */
    test_client_source_pos = 0;
    memcpy(test_client_source, payload, payload_len);
    test_client_source_len = payload_len;
    test_origin_sink_len = 0;

    /* Simulate read event on client_fd */
    struct selector_key read_key = { .s = sel, .fd = connection.client_fd, .data = &connection };
    unsigned state_after_read = socksv5_copy_read(&read_key);
    (void) state_after_read;

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

    /* Verify test_origin_sink captured the forwarded data */
    ck_assert_int_eq((ssize_t)payload_len, (ssize_t)test_origin_sink_len);
    ck_assert_mem_eq(test_origin_sink, payload, payload_len);

    /* Now simulate data coming back from origin to client */
    const char *response = "WORLD_COPY";
    size_t response_len = strlen(response);
    test_origin_source_pos = 0;
    memcpy(test_origin_source, response, response_len);
    test_origin_source_len = response_len;
    test_client_sink_len = 0;
    /* Simulate read event on origin_fd */
    struct selector_key origin_read_key = { .s = sel, .fd = connection.origin_fd, .data = &connection };
    unsigned state_after_origin_read = socksv5_copy_read(&origin_read_key);
    (void) state_after_origin_read;

    /* Now client_buffer should contain the response */
    ck_assert(buffer_can_read(&connection.client_buffer));
    size_t nread_client = 0;
    uint8_t *ptr_client = buffer_read_ptr(&connection.client_buffer, &nread_client);
    ck_assert_ptr_nonnull(ptr_client);
    ck_assert_uint_ge(nread_client, response_len);
    ck_assert_mem_eq(ptr_client, response, response_len);

    /* Simulate write readiness on client_fd to send data to client */
    struct selector_key client_write_key = { .s = sel, .fd = connection.client_fd, .data = &connection };
    unsigned state_after_client_write = socksv5_copy_write(&client_write_key);
    (void) state_after_client_write;
    /* Verify test_client_sink captured the response data */
    ck_assert_int_eq((ssize_t)response_len, (ssize_t)test_client_sink_len);
    ck_assert_mem_eq(test_client_sink, response, response_len);

    /* cleanup selector */
    selector_destroy(sel);
    selector_close();
}

Suite *suite(void) {
    Suite *s = suite_create("copy");
    TCase *tc = tcase_create("copy_basic");

    tcase_add_test(tc, test_copy_read_write_flow);
    tcase_add_test(tc, test_copy_read_write_read_write_flow);
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

/* macros already undone after including the unit */
