#ifndef __my_wifi_h
#define __my_wifi_h

#include <string.h>
#include "stdio.h"
#include "stdlib.h"

#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_wifi.h"

#define EXAMPLE_ESP_WIFI_SSID       "MaisUmaRede"
#define EXAMPLE_ESP_WIFI_PASS       "m1m1uK1___"
#define EXAMPLE_ESP_MAXIMUM_RETRY   3

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1
#define LOG_WIFI            "WIFI"

typedef struct
{
    esp_err_t (*HTTP_EVENT_ERROR_callback)();
    esp_err_t (*HTTP_EVENT_ON_CONNECTED_callback)();
    esp_err_t (*HTTP_EVENT_HEADERS_SENT_callback)();
    esp_err_t (*HTTP_EVENT_ON_HEADER_callback)(char* key, char* value);
    esp_err_t (*HTTP_EVENT_ON_DATA_callback)(void* data,uint32_t data_size, uint32_t pkt_counter);
    esp_err_t (*HTTP_EVENT_ON_FINISH_callback)();
    esp_err_t (*HTTP_EVENT_DISCONNECTED_callback)();
}WIFI_CLIENT_USR_CALLBACKS_ts;

esp_err_t my_wifi_client_event_handler(esp_http_client_event_t *evt);
esp_err_t my_wifi_set_callbacks(WIFI_CLIENT_USR_CALLBACKS_ts wifi_callbacks_param);
void my_wifi_init(char* ssid, char* password);

#endif // __my_wifi_h
