/*
 * Integration-level test for the sensor pipeline.
 *
 * This test wires movement-driver and mqtt-aws together against the host
 * stubs.  The goal is to verify the *contract* between the two
 * components: after the PIR driver signals motion, the application code
 * publishes an event to AWS IoT over MQTT.
 *
 * Because the real driver runs an infinite loop and the real mqtt-aws
 * client needs a broker, this test exercises the public boundary of
 * both components rather than a closed-loop simulation.  The full
 * closed-loop test runs on actual ESP32 hardware in the
 * `tests/integration/` suite.
 */

#include "test_framework.h"
#include "stub_esp_err.h"
#include "stub_freertos.h"
#include "stub_driver_gpio.h"

#include "movement-driver.h"
#include "mqtt-aws.h"

TEST_CASE(pipeline_init_runs_without_error) {
	host_rtos_reset();
	host_gpio_reset();

	init_movement_sensor();
	esp_err_t rv = mqtt_aws_init();

	ASSERT_EQ_INT(ESP_OK, rv);
	/* Driver created the queue and task. */
	ASSERT_EQ_INT(1, g_host_rtos.queues_created);
	ASSERT_EQ_INT(1, g_host_rtos.tasks_created);
}

TEST_CASE(publish_succeeds_after_init) {
	host_rtos_reset();
	host_gpio_reset();

	init_movement_sensor();
	mqtt_aws_init();

	/* Build a payload that matches the IoT rule's expected schema. */
	const char *payload =
		"{\"device_id\":\"esp32-sensor-01\","
		"\"timestamp\":\"2026-07-10T11:00:00Z\","
		"\"event\":\"motion_detected\","
		"\"ttl\":1757000000}";

	esp_err_t rv = mqtt_aws_publish("sensors/motion/esp32-sensor-01",
									payload,
									(int)strlen(payload));
	ASSERT_EQ_INT(ESP_OK, rv);
}

TEST_CASE(multiple_publishes_all_succeed) {
	host_rtos_reset();
	host_gpio_reset();

	init_movement_sensor();
	mqtt_aws_init();

	for (int i = 0; i < 5; i++) {
		esp_err_t rv = mqtt_aws_publish("sensors/motion/esp32-sensor-01",
										"{\"event\":\"motion_detected\"}",
										28);
		ASSERT_EQ_INT(ESP_OK, rv);
	}
}

TEST_MAIN_BEGIN()
	TEST_CASE(pipeline_init_runs_without_error),
	TEST_CASE(publish_succeeds_after_init),
	TEST_CASE(multiple_publishes_all_succeed),
TEST_MAIN_END()
