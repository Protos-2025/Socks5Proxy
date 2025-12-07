#include "logger.h"

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <check.h>

#include "../src/server/logger.c"

START_TEST(logger_test_misc) {
	logger_init();

	time_t timeInfo = mktime(&(struct tm){
		.tm_year = 2022 - 1900,
		.tm_mon = 2,
		.tm_mday = 21,
		.tm_hour = 15,
		.tm_min = 42,
		.tm_sec = 16,
	});

	logger_log_message_deferred(LOGGER_INFO, "my_file.c", 42, &timeInfo, "Test message %d", 1);
	logger_log_message_deferred(LOGGER_ERROR, "other_file.c", 43, &timeInfo, "Test message %d", 2);
	queue_begin_iter(logQueue);

	char * msg = NULL;
	queue_iter_next(logQueue, &msg);
	ck_assert_ptr_nonnull(msg);
	ck_assert_str_eq(msg, "[INFO] [my_file.c:42 @ 2022-03-21 15:42:16] Test message 1\n");

	queue_iter_next(logQueue, &msg);
	ck_assert_ptr_nonnull(msg);
	ck_assert_str_eq(msg, "[ERROR] [other_file.c:43 @ 2022-03-21 15:42:16] Test message 2\n");
	free_logger();
}
END_TEST

START_TEST(logger_big_log_msg) {
	logger_init();

	time_t timeInfo = mktime(&(struct tm){
		.tm_year = 2022 - 1900,
		.tm_mon = 2,
		.tm_mday = 21,
		.tm_hour = 15,
		.tm_min = 42,
		.tm_sec = 16,
	});

	char buffer[MAX_LOG_SIZE];
	for (int i = 0; i < MAX_LOG_SIZE - 1; i++) {
		buffer[i] = 'A';
	}
	buffer[MAX_LOG_SIZE - 1] = '\0';

	logger_log_message_deferred(LOGGER_ERROR, "big_error.c", 100, &timeInfo, "%s", buffer);
	queue_begin_iter(logQueue);
	char * msg = NULL;
	queueIterNext(logQueue, &msg);
	ck_assert_ptr_nonnull(msg);
	ck_assert_msg(strncmp(msg, "[ERROR] [big_error.c:100 @ 2022-03-21 15:42:16] ", 48) == 0, "Log message prefix mismatch");
	ck_assert_int_eq(strlen(msg), MAX_LOG_SIZE - 1);
	ck_assert_msg(strcmp(msg + strlen(msg) - 4, "...\n") == 0, "Log message should end with ellipsis");
	free_logger();
}

Suite *suite(void) {
	Suite *s = suite_create("logger");
	TCase *tc = tcase_create("logger_basic");

	tcase_add_test(tc, logger_test_misc);
	tcase_add_test(tc, logger_big_log_msg);

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
