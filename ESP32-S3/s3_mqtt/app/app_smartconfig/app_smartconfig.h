//防止重复包含
#ifndef __APP_SMARTCONFIG_H__
#define __APP_SMARTCONFIG_H__

// ============================ 头文件 ============================
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"

// ============================ 宏定义 ============================
#define DEFAULT_SCAN_LIST_SIZE 10

// ============================ 函数声明 ===========================
void wifi_smartconfig(void);

#endif /* __APP_SMARTCONFIG_H__ */