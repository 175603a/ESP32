#include <stdio.h>
#include "esp_system.h"
#include "spi_flash_mmap.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include <stdio.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "string.h"

#define DHT11_PIN 17 // 定义DHT11的引脚


#define uint8 unsigned char


static void InputInitial(void) // 设置端口为输入
{
    esp_rom_gpio_pad_select_gpio(DHT11_PIN);
    gpio_set_direction(DHT11_PIN, GPIO_MODE_INPUT);
}

static void OutputHigh(void) // 输出1
{
    esp_rom_gpio_pad_select_gpio(DHT11_PIN);
    gpio_set_direction(DHT11_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_PIN, 1);
}

static void OutputLow(void) // 输出0
{
    esp_rom_gpio_pad_select_gpio(DHT11_PIN);
    gpio_set_direction(DHT11_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_PIN, 0);
}



typedef struct
{
    uint8_t humi_int;  // 湿度的整数部分
    uint8_t humi_deci; // 湿度的小数部分
    uint8_t temp_int;  // 温度的整数部分
    uint8_t temp_deci; // 温度的小数部分
    uint8_t check_sum; // 校验和
} DHT11_Data_TypeDef;

DHT11_Data_TypeDef DHT11_Data;

/*
 * 从DHT11读取一个字节，MSB先行
 */
static uint8_t Read_Byte(void)
{
    uint8_t i, temp = 0;

    for (i = 0; i < 8; i++)
    {
        /*每bit以50us低电平标置开始，轮询直到从机发出 的50us 低电平 结束*/
        while (gpio_get_level(DHT11_PIN) == 0)
            ;

        /*DHT11 以26~28us的高电平表示“0”，以70us高电平表示“1”，
         *通过检测 x us后的电平即可区别这两个状 ，x 即下面的延时
         */
        esp_rom_delay_us(40); // 延时x us 这个延时需要大于数据0持续的时间即可

        if (gpio_get_level(DHT11_PIN) == 1) /* x us后仍为高电平表示数据“1” */
        {
            /* 等待数据1的高电平结束 */
            while (gpio_get_level(DHT11_PIN) == 1)
                ;

            temp |= (uint8_t)(0x01 << (7 - i)); // 把第7-i位置1，MSB先行
        }
        else // x us后为低电平表示数据“0”
        {
            temp &= (uint8_t) ~(0x01 << (7 - i)); // 把第7-i位置0，MSB先行
        }
    }
    return temp;
}
/*
 * 一次完整的数据传输为40bit，高位先出
 * 8bit 湿度整数 + 8bit 湿度小数 + 8bit 温度整数 + 8bit 温度小数 + 8bit 校验和
 */
uint8_t Read_DHT11(DHT11_Data_TypeDef *DHT11_Data)
{
    /*输出模式*/
    /*主机拉低*/
    OutputLow();
    /*延时18ms*/
    vTaskDelay(18);

    /*总线拉高 主机延时30us*/
    OutputHigh();
    esp_rom_delay_us(30); // 延时30us

    /*主机设为输入 判断从机响应信号*/
    InputInitial();

    /*判断从机是否有低电平响应信号 如不响应则跳出，响应则向下运行*/
    if (gpio_get_level(DHT11_PIN) == 0)
    {
        /*轮询直到从机发出 的80us 低电平 响应信号结束*/
        while (gpio_get_level(DHT11_PIN) == 0)
        {
        }

        /*轮询直到从机发出的 80us 高电平 标置信号结束*/
        while (gpio_get_level(DHT11_PIN) == 1)
        {
        }
        /*开始接收数据*/
        DHT11_Data->humi_int = Read_Byte();

        DHT11_Data->humi_deci = Read_Byte();

        DHT11_Data->temp_int = Read_Byte();

        DHT11_Data->temp_deci = Read_Byte();

        DHT11_Data->check_sum = Read_Byte();

        /*读取结束，引脚改为输出模式*/
        /*主机拉高*/
        OutputHigh();

        /*检查读取的数据是否正确*/
        if (DHT11_Data->check_sum == DHT11_Data->humi_int + DHT11_Data->humi_deci + DHT11_Data->temp_int + DHT11_Data->temp_deci)
            return 1;
        else
            return 0;
    }
    else
    {
        return 0;
    }
}

// 创建读取温湿度的任务
void DHT11_Task(void *pvParameters)
{
    char dht11_buff[50] = {0};

    while (1)
    {
        vTaskDelay(2000);
        if (Read_DHT11(&DHT11_Data))
        {
            sprintf(dht11_buff, "Temp=%.2f--Humi=%.2f%%RH \r\n", DHT11_Data.temp_int + DHT11_Data.temp_deci / 10.0, DHT11_Data.humi_int + DHT11_Data.humi_deci / 10.0);
            printf("%s", dht11_buff);
        }
        else
        {
            printf("DHT11 Read Error!\r\n");
        }
        vTaskDelay(300); // 延时300毫秒
    }
}
void app_main()
{
    // 创建读取温湿度的任务
    xTaskCreate(DHT11_Task, "DHT11_Task", 1024 * 2, NULL, 10, NULL);
}