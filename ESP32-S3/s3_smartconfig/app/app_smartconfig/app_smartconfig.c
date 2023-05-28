#include "app_smartconfig.h"

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const char *TAG = "smartconfig_example";


static void smartconfig_example_task(void * parm);

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "Scan done");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        ESP_LOGI(TAG, "Found channel");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        ESP_LOGI(TAG, "Got SSID and password");

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;
        uint8_t ssid[33] = { 0 };
        uint8_t password[65] = { 0 };
        uint8_t rvd_data[33] = { 0 };

        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true) {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));
        ESP_LOGI(TAG, "SSID:%s", ssid);
        ESP_LOGI(TAG, "PASSWORD:%s", password);
        if (evt->type == SC_TYPE_ESPTOUCH_V2) {
            ESP_ERROR_CHECK( esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)) );
            ESP_LOGI(TAG, "RVD_DATA:");
            for (int i=0; i<33; i++) {
                printf("%02x ", rvd_data[i]);
            }
            printf("\n");
        }

        ESP_ERROR_CHECK( esp_wifi_disconnect() );
        ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
        esp_wifi_connect();
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}

/// @brief wifi_smartconfig
/// @param  void
/// @return void
/// @note   必须调用ESP_ERROR_CHECK( nvs_flash_init() );初始化nvs_flash
void wifi_smartconfig(void)
{
    ESP_ERROR_CHECK(esp_netif_init());//创建一个 LwIP 核心任务，并初始化 LwIP 相关工作。
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());//创建一个系统事件任务，并初始化应用程序事件的回调函数。在此情况下，该回调函数唯一的动作就是将事件中继到应用程序任务中。
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();// 创建有 TCP/IP 堆栈的默认网络接口实例绑定 station 或 AP。
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );// 创建 Wi-Fi 驱动程序任务，并初始化 Wi-Fi 驱动程序。

    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

static void smartconfig_example_task(void * parm)
{
    EventBits_t uxBits;
    wifi_config_t myconfig; 

    // 获取wifi配置信息，如果配置过，就直接连接wifi
    esp_wifi_get_config(ESP_IF_WIFI_STA, &myconfig);
    if (strlen((char*)myconfig.sta.ssid) > 0)
    {
        ESP_LOGI(TAG, "alrealy set, SSID is :%s,start connect", myconfig.sta.ssid);
        if(esp_wifi_connect() == ESP_OK)
        {
            vTaskDelete(NULL);//删除配网任务
        }
        else
        {
            ESP_LOGI(TAG, "connect errpr, SSID is :%s,start smarconfig", myconfig.sta.ssid);
            goto smartconfig;
        }
    }
    // 如果没有配置过，就进行配网操作
    else
    {
smartconfig:
        ESP_LOGI(TAG, "have no set, start to config");
        ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS) );
        smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
        ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );
        while (1) {
            uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
            if(uxBits & CONNECTED_BIT) {
                ESP_LOGI(TAG, "WiFi Connected to ap");
            }
            if(uxBits & ESPTOUCH_DONE_BIT) {
                ESP_LOGI(TAG, "smartconfig over");
                esp_smartconfig_stop();
                vTaskDelete(NULL);
            }
        }
    }
}


// void app_main(void)
// {
//     ESP_ERROR_CHECK( nvs_flash_init() );
//     initialise_wifi();
// }
