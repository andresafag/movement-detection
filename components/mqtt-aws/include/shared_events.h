#ifndef SHARED_EVENTS_H
#define SHARED_EVENTS_H

#include "esp_event.h"

// Declarar la base del evento
ESP_EVENT_DECLARE_BASE(PIR_EVENTS);

// Definir el ID del evento
enum {
    PIR_EVENT_MOTION_DETECTED
};

#endif