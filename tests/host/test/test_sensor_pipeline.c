#include <check.h>
#include "movement-driver.h"
#include "mqtt-aws.h"

START_TEST(test_pipeline_smoke)
{
    init_movement_sensor();
    ck_assert_int_eq(mqtt_aws_init(), ESP_OK);
}
END_TEST

Suite *pipeline_suite(void)
{
    Suite *s = suite_create("sensor-pipeline");
    TCase *tc = tcase_create("core");
    tcase_add_test(tc, test_pipeline_smoke);
    suite_add_tcase(s, tc);
    return s;
}

int main(void)
{
    Suite *s = pipeline_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return failed == 0 ? 0 : 1;
}