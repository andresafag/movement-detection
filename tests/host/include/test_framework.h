#ifndef HOST_TEST_FRAMEWORK_H
#define HOST_TEST_FRAMEWORK_H

/*
 * Minimal assertion macros so the unit tests don't depend on any specific
 * host-side test framework (Unity, CMocka, GoogleTest, etc.).
 *
 * Each test function returns void; the framework counts failures and
 * exits non-zero if any assertion failed.  Keep it boring on purpose.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int g_test_failures;
extern int g_test_assertions;
extern const char *g_current_test_name;

#define TEST_CASE(name)                                                       \
    static void name(void);                                                   \
    static void run_##name(void) {                                            \
        g_current_test_name = #name;                                          \
        int before = g_test_failures;                                         \
        printf("  RUN   %s\n", #name);                                        \
        name();                                                               \
        if (g_test_failures == before) {                                      \
            printf("  PASS  %s\n", #name);                                    \
        } else {                                                              \
            printf("  FAIL  %s\n", #name);                                    \
        }                                                                     \
    }                                                                         \
    static void name(void)

#define ASSERT_TRUE(cond)                                                     \
    do {                                                                      \
        g_test_assertions++;                                                  \
        if (!(cond)) {                                                        \
            g_test_failures++;                                                \
            fprintf(stderr, "    ASSERT_TRUE failed: %s\n      at %s:%d\n",   \
                    #cond, __FILE__, __LINE__);                               \
            return;                                                           \
        }                                                                     \
    } while (0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ_INT(expected, actual)                                       \
    do {                                                                      \
        g_test_assertions++;                                                  \
        long _e = (long)(expected);                                           \
        long _a = (long)(actual);                                             \
        if (_e != _a) {                                                       \
            g_test_failures++;                                                \
            fprintf(stderr,                                                   \
                    "    ASSERT_EQ_INT failed: expected %ld got %ld\n"        \
                    "      at %s:%d\n",                                       \
                    _e, _a, __FILE__, __LINE__);                              \
            return;                                                           \
        }                                                                     \
    } while (0)

#define ASSERT_EQ_STR(expected, actual)                                       \
    do {                                                                      \
        g_test_assertions++;                                                  \
        const char *_e = (expected);                                          \
        const char *_a = (actual);                                            \
        if (_e == NULL || _a == NULL || strcmp(_e, _a) != 0) {                \
            g_test_failures++;                                                \
            fprintf(stderr,                                                   \
                    "    ASSERT_EQ_STR failed: expected '%s' got '%s'\n"      \
                    "      at %s:%d\n",                                       \
                    _e ? _e : "(null)",                                       \
                    _a ? _a : "(null)",                                       \
                    __FILE__, __LINE__);                                       \
            return;                                                           \
        }                                                                     \
    } while (0)

#define ASSERT_NOT_NULL(ptr)                                                  \
    do {                                                                      \
        g_test_assertions++;                                                  \
        if ((ptr) == NULL) {                                                  \
            g_test_failures++;                                                \
            fprintf(stderr, "    ASSERT_NOT_NULL failed: %s\n      at %s:%d\n", \
                    #ptr, __FILE__, __LINE__);                                \
            return;                                                           \
        }                                                                     \
    } while (0)

/* Every test binary must define exactly one of these. */
#define TEST_MAIN_BEGIN()                                                     \
    int g_test_failures = 0;                                                  \
    int g_test_assertions = 0;                                                \
    const char *g_current_test_name = "(none)";                               \
    int main(void) {

#define TEST_MAIN_END()                                                       \
        printf("\n%s: %d assertions, %d failures\n",                          \
               __FILE__, g_test_assertions, g_test_failures);                 \
        return g_test_failures == 0 ? 0 : 1;                                  \
    }

#endif /* HOST_TEST_FRAMEWORK_H */
