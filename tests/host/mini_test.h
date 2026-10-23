/**
 * @file    mini_test.h
 * @brief   Khung unit test tối giản, không phụ thuộc gì (chạy được trên mọi máy có gcc).
 *          Tên macro giống Unity (TEST_ASSERT_*, RUN_TEST) để sau này có thể đổi sang
 *          Unity chỉ bằng cách thay #include.
 *
 * Mỗi file test:
 *   #include "mini_test.h"
 *   static void test_xxx(void) { TEST_ASSERT_EQUAL(1, f()); }
 *   int main(void) { TEST_BEGIN(); RUN_TEST(test_xxx); return TEST_END(); }
 */
#ifndef MINI_TEST_H
#define MINI_TEST_H

#include <stdio.h>
#include <string.h>

static int mt_failed_ = 0;
static int mt_run_    = 0;
static int mt_cur_failed_ = 0;

void setUp(void);    /* mỗi file test tự định nghĩa (có thể rỗng) */
void tearDown(void);

#define MT_FAIL_(msg, ...)                                                   \
    do {                                                                     \
        printf("  FAIL %s:%d: " msg "\n", __FILE__, __LINE__, __VA_ARGS__);  \
        mt_cur_failed_ = 1;                                                  \
        return;                                                              \
    } while (0)

#define TEST_ASSERT_TRUE(c)   do { if (!(c)) MT_FAIL_("%s", "expected true: " #c); } while (0)
#define TEST_ASSERT_FALSE(c)  do { if (c)    MT_FAIL_("%s", "expected false: " #c); } while (0)

#define TEST_ASSERT_EQUAL(exp, act)                                          \
    do {                                                                     \
        long long e_ = (long long)(exp), a_ = (long long)(act);              \
        if (e_ != a_) MT_FAIL_("%s: expected %lld, got %lld", #act, e_, a_); \
    } while (0)

#define TEST_ASSERT_EQUAL_CHAR(exp, act)                                     \
    do {                                                                     \
        char e_ = (char)(exp), a_ = (char)(act);                             \
        if (e_ != a_) MT_FAIL_("%s: expected '%c', got '%c'", #act, e_, a_); \
    } while (0)

#define TEST_ASSERT_EQUAL_STRING(exp, act)                                   \
    do {                                                                     \
        const char *e_ = (exp), *a_ = (act);                                 \
        if (strcmp(e_, a_) != 0)                                             \
            MT_FAIL_("%s: expected \"%s\", got \"%s\"", #act, e_, a_);       \
    } while (0)

#define RUN_TEST(fn)                                                         \
    do {                                                                     \
        mt_cur_failed_ = 0;                                                  \
        mt_run_++;                                                           \
        setUp();                                                             \
        fn();                                                                \
        tearDown();                                                          \
        printf("%s %s\n", mt_cur_failed_ ? "[FAIL]" : "[ OK ]", #fn);        \
        mt_failed_ += mt_cur_failed_;                                        \
    } while (0)

#define TEST_BEGIN() printf("== %s\n", __FILE__)
#define TEST_END()   (printf("== %d run, %d failed\n", mt_run_, mt_failed_), mt_failed_ ? 1 : 0)

#endif /* MINI_TEST_H */
