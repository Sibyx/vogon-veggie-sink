#pragma once

#include "mqtt_client.h"
#include "esp_log.h"

#define MQTT_BROKER_URI "mqtt://192.168.0.1"

extern esp_mqtt_client_handle_t mqttClientHandle;

void mqtt_start(void);
_Noreturn void mqtt_reconect_task(void *param);