#include "freertos/FreeRTOS.h"
#include <esp_netif.h>
#include <esp_event.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "bluetooth.h"
#include "shared.h"
#include "publisher.h"

static const char *TAG = "MAIN";

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
        xEventGroupClearBits(connection_event_group, BIT0);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
        xEventGroupSetBits(connection_event_group, BIT0);
    }
}

void app_main(void) {
    // NVS flash for some reason (required by WiFi and MQTT)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Connectivity event group
    connection_event_group = xEventGroupCreate();

    // Init TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // WiFi Setup
    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    assert(netif);

    esp_err_t err = esp_netif_set_hostname(netif, "vogon-veggie-sink");
    if (err != ESP_OK) {
        ESP_LOGE("hostname", "Failed to set hostname: %s", esp_err_to_name(err));
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
            .sta = {
                    .ssid = CONFIG_PUBLISHER_WIFI_SSID,
                    .password = CONFIG_PUBLISHER_WIFI_PASSWORD,
            },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    ESP_ERROR_CHECK(esp_wifi_start());
    xEventGroupWaitBits(connection_event_group, BIT0, false, true, portMAX_DELAY);

    // BLE advertisement processing queue
    advertisement_queue = xQueueCreate(15, sizeof(ble_advertisement_t *));

    xTaskCreatePinnedToCore(
            publisher_task,
            "publisher_task",
            configMINIMAL_STACK_SIZE * 8,
            NULL,
            15,
            NULL,
            0
    );

    xEventGroupWaitBits(connection_event_group, BIT1, false, true, portMAX_DELAY);

    ble_start();
}

