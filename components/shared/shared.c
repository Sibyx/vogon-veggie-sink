#include "shared.h"

EventGroupHandle_t connection_event_group;
QueueHandle_t advertisement_queue;  // For publishing / processing BLE advertisements to MQTT broker
