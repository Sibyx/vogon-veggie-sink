#pragma once

#include "freertos/FreeRTOS.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "cJSON.h"

_Noreturn void publisher_task(void *param);