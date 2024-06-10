#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define MAX_DEVICE_NAME_LENGTH 31
#define MAX_MANUFACTURER_DATA_LENGTH 31

typedef struct {
    uint8_t address[6];
    char name[MAX_DEVICE_NAME_LENGTH];
    uint8_t manufacturer_data[MAX_MANUFACTURER_DATA_LENGTH];
    uint8_t manufacturer_data_len;
} ble_advertisement_t;

extern QueueHandle_t bleDeviceQueue;
extern EventGroupHandle_t connectionEventGroup;