#include <check.h>
#include <stdlib.h>

// asi se puede probar las funciones internas
#include "../src/shared/buffer.c"

#define N(x) (sizeof(x) / sizeof((x)[0]))

START_TEST(test_buffer_misc) {
	struct buffer buf;
	Buffer *b = &buf;
	uint8_t directBuff[6];
	buffer_init(&buf, N(directBuff), directBuff);
	ck_assert_ptr_eq(&buf, b);

	ck_assert_int_eq(true, buffer_can_write(b));
	ck_assert_int_eq(false, buffer_can_read(b));

	size_t wbytes = 0, rbytes = 0;
	uint8_t *ptr = buffer_write_ptr(b, &wbytes);
	ck_assert_uint_eq(6, wbytes);
	// escribo 4 bytes
	uint8_t firstWrite[] = {
		'H',
		'O',
		'L',
		'A',
	};
	memcpy(ptr, firstWrite, sizeof(firstWrite));
	buffer_write_adv(b, sizeof(firstWrite));

	// quedan 2 libres para escribir
	buffer_write_ptr(b, &wbytes);
	ck_assert_uint_eq(2, wbytes);

	// tengo por leer
	buffer_read_ptr(b, &rbytes);
	ck_assert_uint_eq(4, rbytes);

	// leo 3 del buffer
	ck_assert_uint_eq('H', buffer_read(b));
	ck_assert_uint_eq('O', buffer_read(b));
	ck_assert_uint_eq('L', buffer_read(b));

	// queda 1 por leer
	buffer_read_ptr(b, &rbytes);
	ck_assert_uint_eq(1, rbytes);

	// quiero escribir..tendria que seguir habiendo 2 libres
	ptr = buffer_write_ptr(b, &wbytes);
	ck_assert_uint_eq(2, wbytes);

	uint8_t secondWrite[] = {
		' ',
		'M',
	};
	memcpy(ptr, secondWrite, sizeof(secondWrite));
	buffer_write_adv(b, sizeof(secondWrite));

	ck_assert_int_eq(false, buffer_can_write(b));
	buffer_write_ptr(b, &wbytes);
	ck_assert_uint_eq(0, wbytes);

	// tiene que haber 2 + 1 para leer
	ptr = buffer_read_ptr(b, &rbytes);
	ck_assert_uint_eq(3, rbytes);
	ck_assert_ptr_ne(ptr, b->data);

	buffer_compact(b);
	ck_assert_ptr_eq(b->data, buffer_read_ptr(b, &rbytes));
	ck_assert_uint_eq(3, rbytes);
	ck_assert_ptr_eq(b->data + 3, buffer_write_ptr(b, &wbytes));
	ck_assert_uint_eq(3, wbytes);

	uint8_t thirdWrite[] = {
		'U',
		'N',
		'D',
	};
	memcpy(ptr, thirdWrite, sizeof(thirdWrite));
	buffer_write_adv(b, sizeof(thirdWrite));

	buffer_write_ptr(b, &wbytes);
	ck_assert_uint_eq(0, wbytes);
	ck_assert_ptr_eq(b->data, buffer_read_ptr(b, &rbytes));
	buffer_read_adv(b, rbytes);
	buffer_read_ptr(b, &rbytes);
	ck_assert_uint_eq(0, rbytes);
	ck_assert_ptr_eq(b->data, buffer_write_ptr(b, &wbytes));
	ck_assert_uint_eq(6, wbytes);

	buffer_compact(b);
	buffer_read_ptr(b, &rbytes);
	ck_assert_uint_eq(0, rbytes);
	buffer_write_ptr(b, &wbytes);
	ck_assert_uint_eq(N(directBuff), wbytes);
}
END_TEST

Suite *suite(void) {
	Suite *s = suite_create("buffer");
	TCase *tc = tcase_create("buffer");

	tcase_add_test(tc, test_buffer_misc);
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
