#include "list.h"

#include <check.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

int cmp_int(void * a, void * b) { 
    int aa = *(int *)a;
    int bb = *(int *)b;
	return aa - bb;
}

int cmp_str(void * a, void * b) {
  char * aa = *(char **)a;
  char * bb = *(char **)b;
  return strcmp(aa, bb);
}

START_TEST(test_list_misc) {
// integer list
  List list = list_create(&cmp_int, sizeof(int));
  ck_assert_uint_eq(0, list_get_size(list));

  int elem1 = 1;
  int elem2 = 2;
  int elem3 = 3;

  list = list_add(list, &elem1);
  ck_assert_uint_eq(1, list_get_size(list));

  list = list_add(list, &elem2);
  ck_assert_uint_eq(2, list_get_size(list));

  list = list_add(list, &elem3);
  ck_assert_uint_eq(3, list_get_size(list));

  // remove elem
  list = list_remove(list, &elem1);
  ck_assert_uint_eq(2, list_get_size(list));

  list = list_remove(list, &elem2);
  ck_assert_uint_eq(1, list_get_size(list));

  list = list_remove(list, &elem3);
  ck_assert_uint_eq(0, list_get_size(list));

  list_free(list);
	
}
END_TEST


START_TEST(test_list_iter) {
  List list = list_create(&cmp_str, sizeof(char *));
  ck_assert_uint_eq(0, list_get_size(list));
  char *elem1 = "apple";
  char *elem2 = "banana";  
  char *elem3 = "cherry";

  list = list_add(list, &elem1);
  ck_assert_uint_eq(1, list_get_size(list));
  list = list_add(list, &elem2);
  ck_assert_uint_eq(2, list_get_size(list));
  list = list_add(list, &elem3);
  ck_assert_uint_eq(3, list_get_size(list));

  list_begin_iter(list);
  ck_assert_int_eq(1, list_has_next(list));
  char * iterated = NULL;
  ck_assert_ptr_nonnull(list_get_next(list, &iterated));
  ck_assert_str_eq("apple", iterated);
  ck_assert_int_eq(1, list_has_next(list));

  ck_assert_ptr_nonnull(list_get_next(list, &iterated));
  ck_assert_str_eq("banana", iterated);
  ck_assert_int_eq(1, list_has_next(list));
  ck_assert_ptr_nonnull(list_get_next(list, &iterated));
  ck_assert_str_eq("cherry", iterated);
  ck_assert_int_eq(0, list_has_next(list));


  list_free(list);
	
}
END_TEST

Suite *suite(void) {
	Suite *s = suite_create("list");
	TCase *tc = tcase_create("list_basic");

	tcase_add_test(tc, test_list_misc);
	tcase_add_test(tc, test_list_iter);
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
