# Build all unit tests in this directory.
#
# Each test executable links the *real* movement-driver and mqtt-aws source
# against the host stubs and the test framework.

# ---- Movement driver tests ----
add_executable(test_movement_driver
    test_movement_driver.c
)
target_link_libraries(test_movement_driver PRIVATE
    movement_driver
    host_stubs
    m
)
add_test(NAME movement_driver COMMAND test_movement_driver)

# ---- MQTT-AWS tests (currently a stub — verifies it returns ESP_OK) ----
add_executable(test_mqtt_aws
    test_mqtt_aws.c
)
target_link_libraries(test_mqtt_aws PRIVATE
    mqtt_aws
    host_stubs
    m
)
add_test(NAME mqtt_aws COMMAND test_mqtt_aws)

# ---- Integration scenario: driver -> mqtt-aws publish pipeline ----
add_executable(test_sensor_pipeline
    test_sensor_pipeline.c
)
target_link_libraries(test_sensor_pipeline PRIVATE
    movement_driver
    mqtt_aws
    host_stubs
    m
)
add_test(NAME sensor_pipeline COMMAND test_sensor_pipeline)
