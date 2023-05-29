#include "app_smartconfig.h"


static const char *TAG = "main";

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_smartconfig();
}
