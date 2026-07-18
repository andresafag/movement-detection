#ifndef SHARED_EVENTS_H
#define SHARED_EVENTS_H

#include "esp_event.h"

// Declare your custom event base
ESP_EVENT_DECLARE_BASE(PIR_EVENTS);

// Define your custom event IDs
enum {
    PIR_EVENT_MOTION_DETECTED
};

#endif
