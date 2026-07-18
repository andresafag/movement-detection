#ifndef MQTT_AWS_H
#define MQTT_AWS_H

#include "esp_event.h" 

void init_mqtt(void);
void mqtt_send_payload(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

#endif
