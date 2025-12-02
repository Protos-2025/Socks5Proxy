#include "queue.h"

#include <check.h>
#include <stdlib.h>
#include <stdbool.h>

START_TEST(test_queue_misc) {
	Queue queue = createQueue(NULL, NULL, sizeof(int), 0);
	ck_assert_uint_eq(0, queueSize(queue));
	ck_assert_ptr_null(queuePeek(queue, NULL));

	int val1 = 42;
	enqueue(queue, &val1);
	ck_assert_uint_eq(1, queueSize(queue));
	int peeked = 0;
	ck_assert_ptr_nonnull(queuePeek(queue, &peeked));
	ck_assert_int_eq(42, peeked);

	int val2 = 84;
	enqueue(queue, &val2);
	ck_assert_uint_eq(2, queueSize(queue));
	peeked = 0;
	ck_assert_ptr_nonnull(queuePeek(queue, &peeked));
	ck_assert_int_eq(42, peeked);

	int dequeued = 0;
	ck_assert_ptr_nonnull(dequeue(queue, &dequeued));
	ck_assert_int_eq(42, dequeued);
	ck_assert_uint_eq(1, queueSize(queue));
	peeked = 0;
	ck_assert_ptr_nonnull(queuePeek(queue, &peeked));
	ck_assert_int_eq(84, peeked);
	ck_assert_ptr_nonnull(dequeue(queue, &dequeued));
	ck_assert_int_eq(84, dequeued);
	ck_assert_uint_eq(0, queueSize(queue));
	ck_assert_ptr_null(dequeue(queue, &dequeued));

	freeQueue(queue);
}
END_TEST

START_TEST(test_queue_with_capacity) {
	Queue queue = createQueue(NULL, NULL, sizeof(int), 2);
	ck_assert_uint_eq(0, queueSize(queue));

	int val1 = 1;
	int val2 = 2;
	int val3 = 3;

	enqueue(queue, &val1);
	enqueue(queue, &val2);
	ck_assert_uint_eq(2, queueSize(queue));

	enqueue(queue, &val3); // This should evict val1
	ck_assert_uint_eq(2, queueSize(queue));

	int dequeued = 0;
	ck_assert_ptr_nonnull(dequeue(queue, &dequeued));
	ck_assert_int_eq(2, dequeued);
	ck_assert_ptr_nonnull(dequeue(queue, &dequeued));
	ck_assert_int_eq(3, dequeued);
	ck_assert_uint_eq(0, queueSize(queue));

	freeQueue(queue);
}
END_TEST

START_TEST(test_queue_iter) {
	Queue queue = createQueue(NULL, NULL, sizeof(int), 0);
	ck_assert_uint_eq(0, queueSize(queue));
	ck_assert_int_eq(0, queueHasNextIter(queue));
	queueBeginIter(queue);
	ck_assert_int_eq(0, queueHasNextIter(queue));

	int val1 = 10;
	int val2 = 20;
	int val3 = 30;

	enqueue(queue, &val1);
	queueBeginIter(queue);
	ck_assert_int_eq(1, queueHasNextIter(queue));
	enqueue(queue, &val2);
	enqueue(queue, &val3);
	ck_assert_uint_eq(3, queueSize(queue));
	ck_assert_int_eq(1, queueHasNextIter(queue));

	queueBeginIter(queue);
	int iterated = 0;

	ck_assert_ptr_nonnull(queueIterNext(queue, &iterated));
	ck_assert_int_eq(10, iterated);

	ck_assert_ptr_nonnull(queueIterNext(queue, &iterated));
	ck_assert_int_eq(20, iterated);

	ck_assert_ptr_nonnull(queueIterNext(queue, &iterated));
	ck_assert_int_eq(30, iterated);

	ck_assert_int_eq(0, queueHasNextIter(queue));
	ck_assert_ptr_null(queueIterNext(queue, &iterated));

	freeQueue(queue);
}
END_TEST

Suite *suite(void) {
	Suite *s = suite_create("queue");
	TCase *tc = tcase_create("queue_basic");

	tcase_add_test(tc, test_queue_misc);
	tcase_add_test(tc, test_queue_with_capacity);
	tcase_add_test(tc, test_queue_iter);
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
