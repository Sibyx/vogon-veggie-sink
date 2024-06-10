#pragma once

#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_log.h"

extern int retry_num;

void wifi_start();