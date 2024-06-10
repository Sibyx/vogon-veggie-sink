#include "mqtt.h"
#include "shared.h"

static const char* TAG = "MQTT_SERVICE";

esp_mqtt_client_handle_t mqttClientHandle;

static void mqtt_event_handler_cb(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;

    int msg_id;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            xEventGroupSetBits(connectionEventGroup, BIT1);
            break;
        case MQTT_EVENT_DISCONNECTED:
            xEventGroupClearBits(connectionEventGroup, BIT1);
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

_Noreturn void mqtt_reconect_task(void *param) {
    while (true) {
        // Wait for WiFi connection
        xEventGroupWaitBits(connectionEventGroup, BIT0, false, true, portMAX_DELAY);

        // If MQTT is disconnected, restart the MQTT client
        if (!(xEventGroupGetBits(connectionEventGroup) & BIT1)) {
            ESP_LOGI(TAG, "Reconnecting to MQTT...");
            esp_mqtt_client_stop(mqttClientHandle);
            esp_mqtt_client_start(mqttClientHandle);
        }

        // Small delay to prevent busy loop
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void mqtt_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
            .broker.address.uri = MQTT_BROKER_URI,
    };

    mqttClientHandle = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqttClientHandle, ESP_EVENT_ANY_ID, mqtt_event_handler_cb, mqttClientHandle);

    xEventGroupWaitBits(connectionEventGroup, BIT0, false, true, portMAX_DELAY);

    esp_mqtt_client_start(mqttClientHandle);
}