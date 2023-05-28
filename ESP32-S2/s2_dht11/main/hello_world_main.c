/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp32s2/rom/ets_sys.h"
#include "driver/gpio.h"
#include "unistd.h"

//@brief: dht11_read
//@param: temp 温度
//@param: humi 湿度
//@return: 0 成功
//@return: -1 失败
int dht11_read(int16_t *temp, int16_t *humi)
{
    //定义变量
    uint8_t buf[5] = {0};
    uint8_t i = 0;
    uint8_t j = 0;
    //开始信号
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_4, 0);
    vTaskDelay(20 / portTICK_PERIOD_MS);//拉低20ms
    gpio_set_level(GPIO_NUM_4, 1);
    ets_delay_us(40);//拉高40us
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_INPUT);
    //等待DHT11响应
    while (gpio_get_level(GPIO_NUM_4) == 0)
    {
        ets_delay_us(80);//DHT拉低80us
    }
    while (gpio_get_level(GPIO_NUM_4) == 1)
    {
        ets_delay_us(80);//DHT拉高80us
    }
    //读取数据
    for (j = 0; j < 5; j++)
    {
        for (i = 0; i < 8; i++)
        {
            while (gpio_get_level(GPIO_NUM_4) == 0)
            {
                ets_delay_us(50);//DHT拉低50us之后传输1位数据
            }
            if (gpio_get_level(GPIO_NUM_4) == 1)
            {
                buf[j] |= (1 << (7 - i));
                while (gpio_get_level(GPIO_NUM_4) == 1);
            }
        }
    }
    //校验数据
    if (buf[0] + buf[1] + buf[2] + buf[3] == buf[4])
    {
        *temp = buf[2];
        *humi = buf[0];
        return 0;
    }
    else
    {
        return -1;
    }
}



//dht11_task
void dht11_task(void *pvParameters)
{
    //读取DHT11温湿度
    while (1)
    {
        //读取温湿度
        int16_t temp = 0;
        int16_t humi = 0;
        dht11_read(&temp, &humi);
        //打印温湿度
        printf("temp:%d humi:%d\n", temp, humi);
        //延时1s
        usleep(10000);
    }
}

void app_main(void)
{
    printf("Hello world!\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    //创建任务
    xTaskCreate(&dht11_task, "dht11", 2048, NULL, 5, NULL);
    // esp_restart();
}
