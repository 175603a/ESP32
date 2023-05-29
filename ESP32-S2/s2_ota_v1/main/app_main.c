#include "app_smartconfig.h"


static const char *TAG = "main";

void printTask(void *pvParameters)
{
    while (1)
    {
        ESP_LOGI(TAG, "Tis is ota version 1.0.2");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_smartconfig();
    xTaskCreate(printTask, "printTask", 2048, NULL, 5, NULL);
}
