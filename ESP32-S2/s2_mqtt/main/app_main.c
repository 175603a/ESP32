/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "app_smartconfig.h"
#include "app_ota.h"
#include "cJSON.h"

static const char *TAG = "MQTT_EXAMPLE";

#define MQTT_MESSAGE_QUEUE_LENGTH 5
#define BROKER_URL "mqtt://120.48.147.218:1883"

void mqtt_message_receive_task(void *param);

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    QueueHandle_t mqttmessageQueue; // 消息队列
    mqttmessageQueue = (QueueHandle_t)handler_args; // 从参数中获取消息队列句柄
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        // msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        if(xQueueSend(mqttmessageQueue, &event, portMAX_DELAY) == pdPASS) // 发送消息到队列
        {
            ESP_LOGI(TAG, "Send message to queue success");
        }
        else
        {
            ESP_LOGI(TAG, "Send message to queue failed");
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void mqtt_app_start(void)
{
    QueueHandle_t mqttmessageQueue; // 消息队列
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
    };
#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.broker.address.uri, "FROM_STDIN") == 0)
    {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128)
        {
            int c = fgetc(stdin);
            if (c == '\n')
            {
                line[count] = '\0';
                break;
            }
            else if (c > 0 && c < 127)
            {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.broker.address.uri = line;
        printf("Broker url: %s\n", line);
    }
    else
    {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    mqttmessageQueue = xQueueCreate(5, sizeof(esp_mqtt_event_t));
    if (mqttmessageQueue == NULL)
    {
        printf("Failed to create message queue!\n");
        // 队列创建失败的处理逻辑
    }
    xTaskCreate(mqtt_message_receive_task, "mqtt_message_receive_task", 4096, mqttmessageQueue, 2, NULL); // 创建MQTT消息处理任务
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, mqttmessageQueue);
    esp_mqtt_client_start(client);
}
// 创建MQTT消息处理任务
// 任务2接收消息
void mqtt_message_receive_task(void *param)
{
    QueueHandle_t mqttmessageQueue; // 消息队列
    esp_mqtt_event_handle_t receivedMessage;
    mqttmessageQueue = (QueueHandle_t)param;

    while (1)
    {
        if (xQueueReceive(mqttmessageQueue, &receivedMessage, portMAX_DELAY) == pdTRUE)
        {
            char messageBuffer[256]; // 假设消息最大长度为 256
            strncpy(messageBuffer, (char *)receivedMessage->data, receivedMessage->data_len);
            messageBuffer[receivedMessage->data_len] = '\0'; // 添加字符串结束符

            printf("DATA=%s\r\n", messageBuffer);

            cJSON *root = cJSON_Parse(messageBuffer);
            if (root == NULL)
            {
                printf("JSON parsing error!\n");
            }

            // 解析 command 字段
            cJSON *command = cJSON_GetObjectItem(root, "command");
            if (command != NULL && command->type == cJSON_Object)
            {
                // 解析 type 字段
                cJSON *type = cJSON_GetObjectItem(command, "type");
                if (type != NULL && type->type == cJSON_String)
                {
                    const char *type_value = type->valuestring;
                    if (strcmp(type_value, "ota_update") == 0)
                    {
                        // 解析 content 字段
                        cJSON *content = cJSON_GetObjectItem(command, "content");
                        if (content != NULL && content->type == cJSON_Object)
                        {
                            // 解析 path 字段
                            cJSON *path = cJSON_GetObjectItem(content, "path");
                            if (path != NULL && path->type == cJSON_String)
                            {
                                const char *path_value = path->valuestring;
                                //向mqtt服务器发送响应
                                // esp_mqtt_client_publish(client, "/topic/qos0", "ota_update recved", 0, 0, 0);
                                app_ota(path_value); // 执行 OTA 更新操作
                            }
                        }
                    }
                }
            }

            cJSON_Delete(root);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        else
        {
            printf("xQueueReceive failed!\n");
            continue; // 继续下一次循环，避免解析空消息
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_smartconfig();
}
