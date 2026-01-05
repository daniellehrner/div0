#ifndef TEST_ARENA_H
#define TEST_ARENA_H

void test_arena_alloc_basic(void);
void test_arena_alloc_aligned(void);
void test_arena_realloc(void);
void test_arena_reset(void);
void test_arena_alloc_large(void);
void test_arena_alloc_large_alignment(void);
void test_arena_alloc_large_freed_on_reset(void);
void test_arena_alloc_large_multiple(void);

#endif // TEST_ARENA_H