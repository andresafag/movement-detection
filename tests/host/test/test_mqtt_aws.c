/*
 * Unit tests for components/mqtt-aws/mqtt-aws.c
 *
 * The mqtt-aws component is currently a stub returning ESP_OK from both
 * init and publish.  These tests pin down that contract so the eventual
 * real implementation has a clear behavioural target.
 */

#include "test_framework.h"
#include "stub_esp_err.h"

#include "mqtt-aws.h"

#include <string.h>

TEST_CASE(init_returns_esp_ok) {
    esp_err_t rv = mqtt_aws_init();
    ASSERT_EQ_INT(ESP_OK, rv);
}

TEST_CASE(publish_returns_esp_ok) {
    esp_err_t rv = mqtt_aws_publish("sensors/motion/esp32-sensor-01",
                                    "{\"event\":\"motion_detected\"}",
                                    (int)strlen("{\"event\":\"motion_detected\"}"));
    ASSERT_EQ_INT(ESP_OK, rv);
}

TEST_CASE(publish_handles_zero_length_payload) {
    esp_err_t rv = mqtt_aws_publish("sensors/motion/test", "", 0);
    ASSERT_EQ_INT(ESP_OK, rv);
}

TEST_CASE(publish_handles_null_payload_with_zero_length) {
    esp_err_t rv = mqtt_aws_publish("sensors/motion/test", NULL, 0);
    ASSERT_EQ_INT(ESP_OK, rv);
}

TEST_MAIN_BEGIN()
    TEST_CASE(init_returns_esp_ok),
    TEST_CASE(publish_returns_esp_ok),
    TEST_CASE(publish_handles_zero_length_payload),
    TEST_CASE(publish_handles_null_payload_with_zero_length),
TEST_MAIN_END()
