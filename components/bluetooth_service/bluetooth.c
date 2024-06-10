#include "bluetooth.h"
#include "shared.h"

static const char* TAG = "BLE";

static void ble_advertisement_report_callback(struct ble_gap_event *event, void *arg) {
    if (event->type == BLE_GAP_EVENT_DISC) {
        //ESP_LOGI(TAG, "BLE advertisement received");
        ble_advertisement_t *bleAdvertisement = (ble_advertisement_t *)malloc(sizeof(ble_advertisement_t));

        // Copy bleAdvertisement address
        memcpy(bleAdvertisement->address, event->disc.addr.val, 6);

        // Parse advertisement data
        uint8_t *adv_data = event->disc.data;
        uint8_t adv_data_len = event->disc.length_data;
        bool vogonFound = false;

        for (int i = 0; i < adv_data_len; ) {
            uint8_t length = adv_data[i];
            uint8_t type = adv_data[i + 1];

            if (type == BLE_HS_ADV_TYPE_COMP_NAME) {
                // Device name
                int name_len = length - 1;
                if (name_len > MAX_DEVICE_NAME_LENGTH - 1) {
                    name_len = MAX_DEVICE_NAME_LENGTH - 1;
                }
                memcpy(bleAdvertisement->name, &adv_data[i + 2], name_len);
                bleAdvertisement->name[name_len] = '\0';

                if (strcmp(bleAdvertisement->name, "VOGON") == 0) {
                    vogonFound = true;
                } else {
                    break;
                }
            } else if (type == BLE_HS_ADV_TYPE_MFG_DATA && vogonFound) {
                // Manufacturer data
                bleAdvertisement->manufacturer_data_len = length - 1;
                if (bleAdvertisement->manufacturer_data_len > MAX_MANUFACTURER_DATA_LENGTH) {
                    bleAdvertisement->manufacturer_data_len = MAX_MANUFACTURER_DATA_LENGTH;
                }
                memcpy(bleAdvertisement->manufacturer_data, &adv_data[i + 2], bleAdvertisement->manufacturer_data_len);
            }

            i += length + 1;
        }
        // Add the bleAdvertisement to the queue
        if (vogonFound) {
            if (xQueueSend(bleDeviceQueue, &bleAdvertisement, portMAX_DELAY) != pdPASS) {
                ESP_LOGE(TAG, "Failed to send to queue");
                free(bleAdvertisement);
            }
        } else {
            free(bleAdvertisement);
        }
    }
}

static int ble_gap_event_handler(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_DISC:
            ble_advertisement_report_callback(event, arg);
            break;
        default:
            break;
    }
    return 0;
}

static void ble_app_on_sync(void) {
    ble_addr_t addr;
    uint8_t own_addr_type;

    ble_hs_id_infer_auto(0, &own_addr_type);
    ble_hs_id_copy_addr(own_addr_type, addr.val, NULL);
    ESP_LOGI(TAG, "Device Address: %02x:%02x:%02x:%02x:%02x:%02x",
             addr.val[0], addr.val[1], addr.val[2], addr.val[3], addr.val[4], addr.val[5]);

    struct ble_gap_disc_params disc_params;
    memset(&disc_params, 0, sizeof(disc_params));
    disc_params.passive = 1;
    ble_gap_disc(own_addr_type, BLE_HS_FOREVER, &disc_params, ble_gap_event_handler, NULL);
}

static void ble_host_task(void *param) {
    nimble_port_run();
}

void ble_start() {
    nimble_port_init();
    ble_hs_cfg.sync_cb = ble_app_on_sync;
    nimble_port_freertos_init(ble_host_task);
}