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
static uint8_t testClientSource[BUFFER_SIZE];
static uint8_t testClientSink[BUFFER_SIZE];
static size_t testClientSourceLen = 0;
static size_t testClientSourcePos = 0;
static size_t testClientSinkLen = 0;

static uint8_t testOriginSource[BUFFER_SIZE];
static uint8_t testOriginSink[BUFFER_SIZE];
static size_t testOriginSourceLen = 0;
static size_t testOriginSourcePos = 0;
static size_t testOriginSinkLen = 0;

static ssize_t mocked_recv(int fd, void *buf, size_t len, int flags) {
    (void)flags;
    if (fd == FAKE_CLIENT_FD) {
        size_t avail = 0;
        if (testClientSourceLen > testClientSourcePos) {
            avail = testClientSourceLen - testClientSourcePos;
        }
        if (avail == 0) return 0; /* EOF */
        size_t tocopy = (len < avail) ? len : avail;
        memcpy(buf, testClientSource + testClientSourcePos, tocopy);
        testClientSourcePos += tocopy;
        return (ssize_t)tocopy;
    } else if (fd == FAKE_ORIGIN_FD) {
        size_t avail = 0;
        if (testOriginSourceLen > testOriginSourcePos) {
            avail = testOriginSourceLen - testOriginSourcePos;
        }
        if (avail == 0) return 0; /* EOF */
        size_t tocopy = (len < avail) ? len : avail;
        memcpy(buf, testOriginSource + testOriginSourcePos, tocopy);
        testOriginSourcePos += tocopy;
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
        size_t freeSpace = BUFFER_SIZE - testOriginSinkLen;
        size_t tocopy = (len < freeSpace) ? len : freeSpace;
        if (tocopy > 0) {
            memcpy(testOriginSink + testOriginSinkLen, buf, tocopy);
            testOriginSinkLen += tocopy;
            return (ssize_t)tocopy;
        }
        return 0;
    } else if (fd == FAKE_CLIENT_FD) {
        size_t freeSpace = BUFFER_SIZE - testClientSinkLen;
        size_t tocopy = (len < freeSpace) ? len : freeSpace;
        if (tocopy > 0) {
            memcpy(testClientSink + testClientSinkLen, buf, tocopy);
            testClientSinkLen += tocopy;
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

void socksv5_read(struct selector_key* key) { return; };
void socksv5_write(struct selector_key* key) { return; };
void socksv5_block(struct selector_key* key) { return; };
void socksv5_close(struct selector_key* key) { return; };

#define logger_log_message_deferred mock_log

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
    FdSelector sel = selector_new(16);
    ck_assert_ptr_nonnull(sel);

    struct socks5 connection;
    /* initialize fds to fake values */
    connection.client_fd = FAKE_CLIENT_FD;
	connection.origin_fd = FAKE_ORIGIN_FD;
	connection.client.copy = (CopySt){0};
	connection.origin.copy = (CopySt){0};
    connection.client_buffer = (Buffer){0};
    connection.origin_buffer = (Buffer){0};
    buffer_init(&connection.client_buffer, BUFFER_SIZE, connection.client_buffer_data);
    buffer_init(&connection.origin_buffer, BUFFER_SIZE, connection.origin_buffer_data);

    /* Prepare key for arrival (selector passed into copy arrival) */
    struct selector_key key = { .s = sel, .fd = connection.client_fd, .data = &connection };

    /* Call arrival to wire up copy structures and initial interests */
    socksv5_copy_arrival(COPY, &key);

    /* The test: write bytes as if client sent them, call copy_read, then copy_write and verify origin receives them */
    const char *payload = "HELLO_COPY";
    size_t payloadLen = strlen(payload);

    /* Seed the mocked client source buffer so recv on client fd will return payload */
    testClientSourcePos = 0;
    memcpy(testClientSource, payload, payloadLen);
    testClientSourceLen = payloadLen;
    testOriginSinkLen = 0;

    /* Simulate read event on client_fd */
    struct selector_key readKey = { .s = sel, .fd = connection.client_fd, .data = &connection };
    unsigned stateAfterRead = socksv5_copy_read(&readKey);
    (void) stateAfterRead;

    /* Now origin_buffer should contain the payload */
    ck_assert(buffer_can_read(&connection.origin_buffer));
    size_t nread = 0;
    uint8_t *ptr = buffer_read_ptr(&connection.origin_buffer, &nread);
    ck_assert_ptr_nonnull(ptr);
    ck_assert_uint_ge(nread, payloadLen);
    ck_assert_mem_eq(ptr, payload, payloadLen);

    /* Simulate write readiness on origin_fd to send data to origin */
    struct selector_key writeKey = { .s = sel, .fd = connection.origin_fd, .data = &connection };

    unsigned stateAfterWrite = socksv5_copy_write(&writeKey);
    (void) stateAfterWrite;

    /* Verify test_origin_sink captured the forwarded data */
    ck_assert_int_eq((ssize_t)payloadLen, (ssize_t)testOriginSinkLen);
    ck_assert_mem_eq(testOriginSink, payload, payloadLen);

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
    FdSelector sel = selector_new(16);
    ck_assert_ptr_nonnull(sel);

    struct socks5 connection;
    /* initialize fds to fake values */
    connection.client_fd = FAKE_CLIENT_FD;
    connection.origin_fd = FAKE_ORIGIN_FD;
    connection.client.copy = (CopySt){0};
    connection.origin.copy = (CopySt){0};
    connection.client_buffer = (Buffer){0};
    connection.origin_buffer = (Buffer){0};
    buffer_init(&connection.client_buffer, BUFFER_SIZE, connection.client_buffer_data);
    buffer_init(&connection.origin_buffer, BUFFER_SIZE, connection.origin_buffer_data);

    /* Prepare key for arrival (selector passed into copy arrival) */
    struct selector_key key = { .s = sel, .fd = connection.client_fd, .data = &connection };

    /* Call arrival to wire up copy structures and initial interests */
    socksv5_copy_arrival(COPY, &key);

    /* The test: write bytes as if client sent them, call copy_read, then copy_write and verify origin receives them */
    const char *payload = "HELLO_COPY";
    size_t payloadLen = strlen(payload);

    /* Seed the mocked client source buffer so recv on client fd will return payload */
    testClientSourcePos = 0;
    memcpy(testClientSource, payload, payloadLen);
    testClientSourceLen = payloadLen;
    testOriginSinkLen = 0;

    /* Simulate read event on client_fd */
    struct selector_key readKey = { .s = sel, .fd = connection.client_fd, .data = &connection };
    unsigned stateAfterRead = socksv5_copy_read(&readKey);
    (void) stateAfterRead;

    /* Now origin_buffer should contain the payload */
    ck_assert(buffer_can_read(&connection.origin_buffer));
    size_t nread = 0;
    uint8_t *ptr = buffer_read_ptr(&connection.origin_buffer, &nread);
    ck_assert_ptr_nonnull(ptr);
    ck_assert_uint_ge(nread, payloadLen);
    ck_assert_mem_eq(ptr, payload, payloadLen);

    /* Simulate write readiness on origin_fd to send data to origin */
    struct selector_key writeKey = { .s = sel, .fd = connection.origin_fd, .data = &connection };

    unsigned stateAfterWrite = socksv5_copy_write(&writeKey);
    (void) stateAfterWrite;

    /* Verify test_origin_sink captured the forwarded data */
    ck_assert_int_eq((ssize_t)payloadLen, (ssize_t)testOriginSinkLen);
    ck_assert_mem_eq(testOriginSink, payload, payloadLen);

    /* Now simulate data coming back from origin to client */
    const char *response = "WORLD_COPY";
    size_t responseLen = strlen(response);
    testOriginSourcePos = 0;
    memcpy(testOriginSource, response, responseLen);
    testOriginSourceLen = responseLen;
    testClientSinkLen = 0;
    /* Simulate read event on origin_fd */
    struct selector_key originReadKey = { .s = sel, .fd = connection.origin_fd, .data = &connection };
    unsigned stateAfterOriginRead = socksv5_copy_read(&originReadKey);
    (void) stateAfterOriginRead;

    /* Now client_buffer should contain the response */
    ck_assert(buffer_can_read(&connection.client_buffer));
    size_t nreadClient = 0;
    uint8_t *ptrClient = buffer_read_ptr(&connection.client_buffer, &nreadClient);
    ck_assert_ptr_nonnull(ptrClient);
    ck_assert_uint_ge(nreadClient, responseLen);
    ck_assert_mem_eq(ptrClient, response, responseLen);

    /* Simulate write readiness on client_fd to send data to client */
    struct selector_key clientWriteKey = { .s = sel, .fd = connection.client_fd, .data = &connection };
    unsigned stateAfterClientWrite = socksv5_copy_write(&clientWriteKey);
    (void) stateAfterClientWrite;
    /* Verify test_client_sink captured the response data */
    ck_assert_int_eq((ssize_t)responseLen, (ssize_t)testClientSinkLen);
    ck_assert_mem_eq(testClientSink, response, responseLen);

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
    int numberFailed;

    srunner_run_all(sr, CK_NORMAL);
    numberFailed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (numberFailed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* macros already undone after including the unit */
