#ifndef MQTT_AWS_H
  #define MQTT_AWS_H

  #include "esp_err.h"

  esp_err_t mqtt_aws_init(void);
  esp_err_t mqtt_aws_publish(const char *topic, const char *payload, int payload_len);

  #endif
