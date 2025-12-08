#include "list.h"
#include "../src/server/include/users.h"

#include <check.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

START_TEST(users_test_list) {
    users_init();

    UserStatus status;

    status = user_create("alice", "password123", USER_PRIVILEGE_DEFAULT);
    ck_assert_uint_eq(USER_OK, status);

    status = user_create("bob", "securepass", USER_PRIVILEGE_ADMIN);
    ck_assert_uint_eq(USER_OK, status);

    status = user_create("alice", "newpassword", USER_PRIVILEGE_DEFAULT);
    ck_assert_uint_eq(USER_ALREADYEXISTS, status);

    User retrievedUser;
    bool exists = user_exists("alice", &retrievedUser);
    ck_assert_msg(exists, "User 'alice' should exist");
    ck_assert_str_eq(retrievedUser.username, "alice");
    ck_assert_str_eq(retrievedUser.password, "password123");
    ck_assert_uint_eq(retrievedUser.privilege_level, USER_PRIVILEGE_DEFAULT);

    exists = user_exists("charlie", NULL);
    ck_assert_msg(!exists, "User 'charlie' should not exist");

    status = user_authenticate("alice", "password123");
    ck_assert_uint_eq(USER_OK, status);

    status = user_authenticate("alice", "wrongpassword");
    ck_assert_uint_eq(USER_WRONGPASSWORD, status);

    status = user_authenticate("charlie", "somepassword");
    ck_assert_uint_eq(USER_BADUSERNAME, status);

}
END_TEST

Suite *suite(void) {
	Suite *s = suite_create("users");
	TCase *tc = tcase_create("users_tests");

	tcase_add_test(tc, users_test_list);
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

