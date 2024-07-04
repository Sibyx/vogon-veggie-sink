#include <esp_mac.h>
#include "shared.h"
#include "publisher.h"

#define MAC_LEN 18
#define TOPIC_LEN 100

#define PROTOCOL_TYPE_UINT8 1
#define PROTOCOL_TYPE_UINT16 2
#define PROTOCOL_TYPE_UINT32 3
#define PROTOCOL_TYPE_FLOAT 4
#define PROTOCOL_TYPE_DOUBLE 5

static const char *TAG = "PUBLISHER";
const int WIFI_CONNECTED_BIT = BIT0;
const int MQTT_CONNECTED_BIT = BIT1;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            xEventGroupSetBits(connection_event_group, MQTT_CONNECTED_BIT);
            ESP_LOGD(TAG, "MQTT_EVENT_CONNECTED");
            break;
        case MQTT_EVENT_DISCONNECTED:
            xEventGroupClearBits(connection_event_group, MQTT_CONNECTED_BIT);
            ESP_LOGW(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(TAG, "MQTT_EVENT_PUBLISHED");
            break;
        default:
            break;
    }
}

_Noreturn void publisher_task(void *param) {
    // MQTT Setup
    esp_mqtt_client_config_t mqtt_cfg = {
            .broker.address.uri = CONFIG_PUBLISHER_MQTT_BROKER,
    };

    esp_mqtt_client_handle_t mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_client));

    esp_mqtt_client_start(mqtt_client);
    xEventGroupWaitBits(connection_event_group, MQTT_CONNECTED_BIT, false, true, portMAX_DELAY);

    ble_advertisement_t *advertisement;
    char node_addr_str[MAC_LEN];
    char sink_addr_str[MAC_LEN];
    uint8_t sink_addr[6];
    char topic[TOPIC_LEN];

    esp_read_mac(sink_addr, ESP_MAC_EFUSE_FACTORY);
    snprintf(sink_addr_str, sizeof(sink_addr_str), "%02x:%02x:%02x:%02x:%02x:%02x",
             sink_addr[0], sink_addr[1], sink_addr[2], sink_addr[3], sink_addr[4], sink_addr[5]
             );

    while (true) {
        if (xQueueReceive(advertisement_queue, &advertisement, portMAX_DELAY) == pdPASS) {
            snprintf(node_addr_str, sizeof(node_addr_str), "%02x:%02x:%02x:%02x:%02x:%02x",
                     advertisement->address[0], advertisement->address[1], advertisement->address[2],
                     advertisement->address[3], advertisement->address[4], advertisement->address[5]);

            ESP_LOGD(TAG, "Node Device (name=%s; address=%s)", node_addr_str, advertisement->name);
            ESP_LOG_BUFFER_HEX(TAG, advertisement->manufacturer_data, advertisement->manufacturer_data_len);

            if (strcmp(advertisement->name, CONFIG_PUBLISHER_DEVICE_MANUFACTURER_NAME) == 0) {
                uint16_t sensor = (advertisement->manufacturer_data[1] << 8) | advertisement->manufacturer_data[0];
                uint8_t parameter = advertisement->manufacturer_data[2];
                uint8_t type = advertisement->manufacturer_data[3];

                cJSON *root = cJSON_CreateObject();
                cJSON_AddStringToObject(root, "address", node_addr_str);
                cJSON_AddNumberToObject(root, "sensor", sensor);
                cJSON_AddNumberToObject(root, "parameter", parameter);

                const void *data = advertisement->manufacturer_data + 4;

                switch (type) {
                    case PROTOCOL_TYPE_UINT8:
                        cJSON_AddNumberToObject(root, "value", *(uint8_t *)data);
                        break;
                    case PROTOCOL_TYPE_UINT16:
                        cJSON_AddNumberToObject(root, "value", *(uint16_t *)data);
                        break;
                    case PROTOCOL_TYPE_UINT32:
                        cJSON_AddNumberToObject(root, "value", *(uint32_t *)data);
                        break;
                    case PROTOCOL_TYPE_FLOAT:
                        cJSON_AddNumberToObject(root, "value", *(float *)data);
                        break;
                    case PROTOCOL_TYPE_DOUBLE:
                        cJSON_AddNumberToObject(root, "value", *(double *)data);
                        break;
                    default:
                        break;
                }

                char *message = cJSON_Print(root);
                snprintf(topic, sizeof(topic), "vogonveggie/%s/%s/raw", sink_addr_str, node_addr_str);

                if (message) {
                    esp_mqtt_client_publish(mqtt_client, topic, message, 0, 1, 0);
                    free(message);
                }

                cJSON_Delete(root);
            }

            free(advertisement);
        }
    }
}