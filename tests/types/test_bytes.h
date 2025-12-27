#ifndef TEST_BYTES_H
#define TEST_BYTES_H

// bytes test declarations
void test_bytes_init_empty(void);
void test_bytes_from_data_works(void);
void test_bytes_append_works(void);
void test_bytes_append_byte_works(void);
void test_bytes_clear_works(void);
void test_bytes_free_works(void);
void test_bytes_arena_backed(void);
void test_bytes_arena_no_realloc(void);

#endif // TEST_BYTES_H