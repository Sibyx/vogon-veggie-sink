#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "bluetooth.h"
#include "wifi.h"
#include "consumer.h"
#include "mqtt.h"
#include "shared.h"

static const char* TAG = "VOGON_SINK";

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    bleDeviceQueue = xQueueCreate(15, sizeof(ble_advertisement_t *));
    connectionEventGroup = xEventGroupCreate();

    xTaskCreatePinnedToCore(
            mqtt_reconect_task,
    "mqtt_reconect_task",
            configMINIMAL_STACK_SIZE * 8,
            NULL,
            5,
            NULL,
            0
    );

    xTaskCreatePinnedToCore(
            consumer_task,
            "consumer_task",
            configMINIMAL_STACK_SIZE * 8,
            NULL,
            5,
            NULL,
            0
    );

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_start();
    mqtt_start();
    ble_start();
}

