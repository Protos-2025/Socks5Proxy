#include "logger.h"

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <check.h>

#include "../src/server/logger.c"

START_TEST(logger_test_misc) {
	loggerInit();

	time_t time_info = mktime(&(struct tm){
		.tm_year = 2022 - 1900,
		.tm_mon = 2,
		.tm_mday = 21,
		.tm_hour = 15,
		.tm_min = 42,
		.tm_sec = 16,
	});

	loggerLogMessageDeferred(LOGGER_INFO, "my_file.c", 42, &time_info, "Test message %d", 1);
	loggerLogMessageDeferred(LOGGER_ERROR, "other_file.c", 43, &time_info, "Test message %d", 2);
	queueBeginIter(logQueue);

	char * msg = NULL;
	queueIterNext(logQueue, &msg);
	ck_assert_ptr_nonnull(msg);
	ck_assert_str_eq(msg, "[INFO] [my_file.c:42 @ 2022-03-21 15:42:16] Test message 1");

	queueIterNext(logQueue, &msg);
	ck_assert_ptr_nonnull(msg);
	ck_assert_str_eq(msg, "[ERROR] [other_file.c:43 @ 2022-03-21 15:42:16] Test message 2");
	freeLogger();
}
END_TEST

START_TEST(logger_big_log_msg) {
	loggerInit();

	time_t time_info = mktime(&(struct tm){
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

	loggerLogMessageDeferred(LOGGER_ERROR, "big_error.c", 100, &time_info, "%s", buffer);
	queueBeginIter(logQueue);
	char * msg = NULL;
	queueIterNext(logQueue, &msg);
	ck_assert_ptr_nonnull(msg);
	ck_assert_msg(strncmp(msg, "[ERROR] [big_error.c:100 @ 2022-03-21 15:42:16] ", 48) == 0, "Log message prefix mismatch");
	ck_assert_int_eq(strlen(msg), MAX_LOG_SIZE - 1);
	ck_assert_msg(strcmp(msg + strlen(msg) - 3, "...") == 0, "Log message should end with ellipsis");
	freeLogger();
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
	int number_failed;

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
