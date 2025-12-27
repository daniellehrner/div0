#ifndef TEST_NIBBLES_H
#define TEST_NIBBLES_H

// nibbles_from_bytes tests
void test_nibbles_from_bytes_empty(void);
void test_nibbles_from_bytes_single(void);
void test_nibbles_from_bytes_multiple(void);

// nibbles_to_bytes tests
void test_nibbles_to_bytes_empty(void);
void test_nibbles_to_bytes_single(void);
void test_nibbles_to_bytes_multiple(void);

// nibbles_to_bytes_alloc tests
void test_nibbles_to_bytes_alloc_works(void);

// nibbles_common_prefix tests
void test_nibbles_common_prefix_none(void);
void test_nibbles_common_prefix_partial(void);
void test_nibbles_common_prefix_full(void);
void test_nibbles_common_prefix_different_lengths(void);

// nibbles_slice tests
void test_nibbles_slice_full(void);
void test_nibbles_slice_partial(void);
void test_nibbles_slice_view(void);
void test_nibbles_slice_empty(void);
void test_nibbles_slice_out_of_bounds(void);

// nibbles_copy tests
void test_nibbles_copy_works(void);
void test_nibbles_copy_empty(void);

// nibbles_cmp tests
void test_nibbles_cmp_equal(void);
void test_nibbles_cmp_less(void);
void test_nibbles_cmp_greater(void);
void test_nibbles_cmp_prefix(void);

// nibbles_equal tests
void test_nibbles_equal_works(void);

// nibbles utility tests
void test_nibbles_is_empty(void);
void test_nibbles_get(void);

// roundtrip tests
void test_nibbles_roundtrip(void);

#endif // TEST_NIBBLES_H
