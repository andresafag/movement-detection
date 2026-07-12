#include <check.h>
#include "mqtt-aws.h"

START_TEST(test_mqtt_init_returns_ok)
{
    ck_assert_int_eq(mqtt_aws_init(), ESP_OK);
}
END_TEST

START_TEST(test_mqtt_publish_returns_ok)
{
    ck_assert_int_eq(mqtt_aws_publish("sensors/motion/test", "{}", 2), ESP_OK);
}
END_TEST

Suite *mqtt_suite(void)
{
    Suite *s = suite_create("mqtt-aws");
    TCase *tc = tcase_create("core");
    tcase_add_test(tc, test_mqtt_init_returns_ok);
    tcase_add_test(tc, test_mqtt_publish_returns_ok);
    suite_add_tcase(s, tc);
    return s;
}

int main(void)
{
    Suite *s = mqtt_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return failed == 0 ? 0 : 1;
}