#include <string.h>
#include "stdio.h"
#include "stdlib.h"

#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "driver/gpio.h"
#include "driver/uart.h"

#include "esp_system.h"

#include "esp_log.h"
#include "my_wifi.h"
#include "hexa_server.h"

#define BLINK_GPIO 2

/**
 * @brief Qualquer http request utilizado no firmware
 * precisa ser definido neste enum e tratado nas
 * funções de wifi_callbacks
 */  
typedef enum
{
  WIFI_REQUEST_NONE                         ,
  WIFI_REQUEST_HEXA_SERVER_GET_FIRM_VERSION ,
  WIFI_REQUEST_HEXA_SERVER_GET_FIRM         ,
}WIFI_REQUEST_tn;

WIFI_REQUEST_tn last_request = WIFI_REQUEST_NONE;

esp_err_t general_HTTP_EVENT_ERROR_callback(void)
{
  return ESP_OK;
}

esp_err_t general_HTTP_EVENT_ON_CONNECTED_callback(void)
{
  return ESP_OK;
}

esp_err_t general_HTTP_EVENT_HEADERS_SENT_callback(void)
{
  return ESP_OK;
}

esp_err_t general_HTTP_EVENT_ON_HEADER_callback(char* key, char* value)
{

  switch(last_request)
  {
    case WIFI_REQUEST_NONE:

    break;

    case WIFI_REQUEST_HEXA_SERVER_GET_FIRM_VERSION:
      hexa_srv_on_header_callback(key, value);
    break;

    case WIFI_REQUEST_HEXA_SERVER_GET_FIRM:
      hexa_srv_on_header_callback(key, value);
    break;

    default:

    break;
  }

  return ESP_OK;
}

esp_err_t general_HTTP_EVENT_ON_DATA_callback(void* data, uint32_t data_size, uint32_t pkt_counter)
{
  switch(last_request)
  {
    case WIFI_REQUEST_NONE:

    break;

    case WIFI_REQUEST_HEXA_SERVER_GET_FIRM_VERSION:
      hexa_srv_on_data_version(data, data_size, pkt_counter);
    break;

    case WIFI_REQUEST_HEXA_SERVER_GET_FIRM:
      hexa_srv_on_data_firm(data, data_size, pkt_counter);
    break;

    default:

    break;
  }

  return ESP_OK;
}

esp_err_t general_HTTP_EVENT_ON_FINISH_callback(void)
{
  hexa_srv_on_finish_firm();
  return ESP_OK;
}

esp_err_t general_HTTP_EVENT_DISCONNECTED_callback(void)
{
  return ESP_OK;
}

WIFI_CLIENT_USR_CALLBACKS_ts wifi_callbacks = 
{
  general_HTTP_EVENT_ERROR_callback         ,
  general_HTTP_EVENT_ON_CONNECTED_callback  ,
  general_HTTP_EVENT_HEADERS_SENT_callback  ,
  general_HTTP_EVENT_ON_HEADER_callback     ,
  general_HTTP_EVENT_ON_DATA_callback       ,
  general_HTTP_EVENT_ON_FINISH_callback     ,
  general_HTTP_EVENT_DISCONNECTED_callback  ,
};

void hexa_srv_get_version(void)
{
    last_request = WIFI_REQUEST_HEXA_SERVER_GET_FIRM_VERSION;
    esp_http_client_config_t client_config = 
    {
        .url = "http://127.0.0.1:5000/firmware_version",
        .event_handler = my_wifi_client_event_handler
    };
    esp_http_client_handle_t client = esp_http_client_init(&client_config);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}

void hexa_srv_get_firmware(void)
{
  last_request = WIFI_REQUEST_HEXA_SERVER_GET_FIRM;
  esp_http_client_config_t client_config = 
  {
      .url = "http://127.0.0.1:5000/firmware_file",
      .event_handler = my_wifi_client_event_handler
  };
  esp_http_client_handle_t client = esp_http_client_init(&client_config);
  esp_http_client_perform(client);
  esp_http_client_cleanup(client);
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_LOGI(LOG_WIFI, "ESP_WIFI_MODE_STA");
    my_wifi_init("MaisUmaRede", "m1m1uK1___");

    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    
    while(1)
    {

        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(50);

        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(50);

    }
}
