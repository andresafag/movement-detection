#include <check.h>
#include "movement-driver.h"

START_TEST(test_init_movement_sensor_smoke)
{
    init_movement_sensor();
    ck_assert_int_eq(1, 1);
}
END_TEST

Suite *movement_suite(void)
{
    Suite *s = suite_create("movement-driver");
    TCase *tc = tcase_create("core");
    tcase_add_test(tc, test_init_movement_sensor_smoke);
    suite_add_tcase(s, tc);
    return s;
}

int main(void)
{
    Suite *s = movement_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return failed == 0 ? 0 : 1;
}