#include "consumer.h"
#include "shared.h"
#include "mqtt.h"

static const char* TAG = "CONSUMER";

#define MANUFACTURER_NAME "VOGON"
#define MANUFACTURER_DATA_LEN 12
#define SINK_ID "b68a4827-3998-409a-99c8-d985918be910"

_Noreturn void consumer_task(void *param) {
    ble_advertisement_t *device;
    char addr_str[18];
    char topic[100];

    while (1) {
        if (xQueueReceive(bleDeviceQueue, &device, portMAX_DELAY) == pdPASS) {
            snprintf(addr_str, sizeof(addr_str), "%02x:%02x:%02x:%02x:%02x:%02x",
                     device->address[0], device->address[1], device->address[2],
                     device->address[3], device->address[4], device->address[5]);

            ESP_LOGD(TAG, "Device found:");
            ESP_LOGD(TAG, "  Address: %s", addr_str);
            ESP_LOGD(TAG, "  Name: %s", device->name);
            ESP_LOGD(TAG, "  Manufacturer data: ");
            ESP_LOG_BUFFER_HEX(TAG, device->manufacturer_data, device->manufacturer_data_len);

            if (device->manufacturer_data_len == MANUFACTURER_DATA_LEN &&
                strcmp(device->name, MANUFACTURER_NAME) == 0) {

                float pressure = *(float*)(device->manufacturer_data);
                float temperature = *(float*)(device->manufacturer_data + 4);
                float humidity = *(float*)(device->manufacturer_data + 8);

                cJSON *root = cJSON_CreateObject();
                cJSON_AddStringToObject(root, "address", addr_str);
                cJSON_AddNumberToObject(root, "pressure", pressure);
                cJSON_AddNumberToObject(root, "temperature", temperature);
                cJSON_AddNumberToObject(root, "humidity", humidity);

                char *message = cJSON_Print(root);
                snprintf(topic, sizeof(topic), "%s/%s/temperature", SINK_ID, addr_str);
                if (message) {
                    esp_mqtt_client_publish(mqttClientHandle, topic, message, 0, 1, 0);
                    free(message);
                }

                cJSON_Delete(root);
            }

            free(device);
        }
    }
}
