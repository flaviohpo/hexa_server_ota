/* 
pinos utilizados
TXD2 20
RXD2 19
SET  8
RTS2 41
CTS2 42


I (49026) esp_image: segment 0: paddr=00110020 vaddr=3f400020 size=1ada0h (109984) map
I (49062) esp_image: segment 1: paddr=0012adc8 vaddr=3ffb0000 size=038dch ( 14556) 
I (49066) esp_image: segment 2: paddr=0012e6ac vaddr=40080000 size=0196ch (  6508) 
I (49074) esp_image: segment 3: paddr=00130020 vaddr=400d0020 size=8d350h (578384) map
I (49234) esp_image: segment 4: paddr=001bd378 vaddr=4008196c size=12d60h ( 77152) 
I (49258) esp_image: segment 5: paddr=001d00e0 vaddr=50000000 size=00010h (    16) 

PROBLEMS
- na hora de gravar a primeira parte do firmware na particao OTA esta
dando um problema de acesso em memoria não permitido

*/


#include <string.h>
#include "stdio.h"
#include "stdlib.h"

#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "driver/gpio.h"
#include "driver/uart.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_ota_ops.h"

#include "../version.h"

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      "MaisUmaRede"
#define EXAMPLE_ESP_WIFI_PASS      "m1m1uK1___"
#define EXAMPLE_ESP_MAXIMUM_RETRY  3
#define BLINK_GPIO      2

esp_err_t ota_update_start(uint32_t last_data_length, esp_partition_t** update_partition, esp_ota_handle_t* ota_handle);
esp_err_t ota_update_chunk(uint8_t* data, uint32_t size, esp_ota_handle_t* ota_handle);
esp_err_t ota_update_finish(esp_partition_t** update_partition, esp_ota_handle_t* ota_handle);

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define LOG_OTA             "OTA"
#define LOG_WIFI            "WIFI"
#define URL_VERSION         "http://127.0.0.1:5000/firmware_version"
#define URL_FILE            "http://127.0.0.1:5000/firmware_file"

typedef enum{
    FIRM_VERSION_REQUEST        ,
    FIRM_FILE_REQUEST           ,
}CURRENT_HTTP_REQUEST_tn;

CURRENT_HTTP_REQUEST_tn CurrentRequest = FIRM_VERSION_REQUEST;
char remote_firmware_version[50] = "null";
uint32_t local_version_major;
uint32_t local_version_minor;
uint32_t local_version_patch;
static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(LOG_WIFI, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(LOG_WIFI,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(LOG_WIFI, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = 
    {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
	        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = 
            {
                .capable = true,
                .required = false
            },
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(LOG_WIFI, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(LOG_WIFI, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(LOG_WIFI, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(LOG_WIFI, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}

uint8_t firmware_version_received = 0;
uint32_t last_content_length = 0;
uint32_t last_data_length = 0;
uint32_t packet_counter = 0;
esp_ota_handle_t ota_handle;
const esp_partition_t *update_partition;

esp_err_t client_event_handler(esp_http_client_event_t *evt)
{
    
    char* data_ptr = (char*)evt->data;
    esp_err_t err = ESP_OK;

    switch (evt->event_id)
    {
        case HTTP_EVENT_ERROR:
            printf("HTTP_EVENT_ERROR\n");
        break;

        case HTTP_EVENT_ON_CONNECTED:
            printf("HTTP_EVENT_ON_CONNECTED\n");
        break;

        case HTTP_EVENT_HEADERS_SENT:
            packet_counter = 0;
            printf("HTTP_EVENT_HEADERS_SENT\n");
        break;

        case HTTP_EVENT_ON_HEADER:
            printf("HTTP_EVENT_ON_HEADER: key=%s value=%s\n", evt->header_key, evt->header_value);
            if(strcmp(evt->header_key, "Content-Length") == 0)
            {
                last_content_length = atoi(evt->header_value);
                printf("last_content_length=%d\n", last_content_length);
            }
        break;

        case HTTP_EVENT_ON_DATA:
            switch(CurrentRequest)
            {
                case FIRM_VERSION_REQUEST:
                    strncpy(remote_firmware_version, (char*)evt->data, evt->data_len);
                    printf("Version received=%s\n", remote_firmware_version);
                    
                break;

                case FIRM_FILE_REQUEST:
                    if(packet_counter == 0)
                    {
                        update_partition = esp_ota_get_next_update_partition(NULL);
                        ESP_LOGI(LOG_OTA, "---->Partition label: '%s'", update_partition->label);
                        ESP_LOGI(LOG_OTA, "---->Partition size: '%d'", update_partition->size);
                        esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
                        if (err != ESP_OK)
                        {
                            ESP_LOGE(LOG_OTA, "Error With OTA Begin, Cancelling OTA");
                            return ESP_FAIL;
                        }
                        else
                        {
                            ESP_LOGI(LOG_OTA, "Writing to partition subtype %d at offset 0x%X", update_partition->subtype, update_partition->address);
                            ESP_LOGI(LOG_OTA, "File Size: %d", last_content_length);
                            if(esp_ota_write(ota_handle, evt->data, evt->data_len) != ESP_OK)
                            {
                                ESP_LOGE(LOG_OTA, "Linha:%d", __LINE__);
                                return ESP_FAIL;
                            }
                        }
                    }
                    else
                    {
                        if(esp_ota_write(ota_handle, evt->data, evt->data_len) != ESP_OK)
                        {
                            ESP_LOGE(LOG_OTA, "Linha:%d", __LINE__);
                            return ESP_FAIL;
                        }
                    }
                break;
            }
            //printf("pkt=%d\n", packet_counter);
            printf(".");
            packet_counter++;
        break;

        case HTTP_EVENT_ON_FINISH:
            printf("HTTP_EVENT_ON_FINISH\n");
            switch(CurrentRequest)
            {
                case FIRM_VERSION_REQUEST:
                    firmware_version_received = 1;
                break;

                case FIRM_FILE_REQUEST:
                    err = esp_ota_end(ota_handle);
                    if (err == ESP_OK)
                    {
                        // Seta para que o boot inicie n nova partição
                        if(update_partition == NULL)
                        {
                            ESP_LOGE("OTA", "update_partition == NULL");
                            return ESP_FAIL;
                        }
                        // esta retornando ESP_ERR_OTA_VALIDATE_FAILED
                        err = esp_ota_set_boot_partition(update_partition);
                        
                        if (err == ESP_OK)
                        {
                            const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
                            ESP_LOGI(LOG_OTA, "Next boot partition subtype %d at offset 0x%x", boot_partition->subtype, boot_partition->address);
                            ESP_LOGI(LOG_OTA, "Please Restart System...");
                        }
                        else
                        {
                            ESP_LOGE(LOG_OTA, "Error %d from esp_ota_set_boot_partition()", (uint32_t)err);
                            return ESP_FAIL;
                        }
                    }
                    else
                    {
                        ESP_LOGE(LOG_OTA, "Error %d from esp_ota_end()", (uint32_t)err);
                        return ESP_FAIL;
                    }
                break;
            }
        break;

        case HTTP_EVENT_DISCONNECTED:
            printf("HTTP_EVENT_DISCONNECTED\n");
        break;

        default:
            printf("EVENTO HTTP NAO TRATADO: %d\n", evt->event_id);
        break;
    }

    return ESP_OK;
}

void http_get_firmware_file(void)
{
    CurrentRequest = FIRM_FILE_REQUEST;
    esp_http_client_config_t client_config = 
    {
        .url = URL_FILE,
        .event_handler = client_event_handler
    };
    esp_http_client_handle_t client = esp_http_client_init(&client_config);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}

void http_get_firmware_version(void)
{
    CurrentRequest = FIRM_VERSION_REQUEST;
    esp_http_client_config_t client_config = 
    {
        .url = URL_VERSION,
        .event_handler = client_event_handler
    };
    esp_http_client_handle_t client = esp_http_client_init(&client_config);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}

void get_worldclock_api(void)
{
    esp_http_client_config_t client_config = 
    {
        .url = "http://worldclockapi.com/api/json/utc/now",
        .event_handler = client_event_handler
    };
    esp_http_client_handle_t client = esp_http_client_init(&client_config);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}

void app_main(void)
{
    uint32_t remote_version_major;
    uint32_t remote_version_minor;
    uint32_t remote_version_patch;
    char* token;
    char current_version[50];

    //Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_LOGI(LOG_WIFI, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    //get_worldclock_api();

    gpio_pad_select_gpio(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    printf("FIRMWARE VERSION=%s\n", FIRMWARE_VERSION);

    http_get_firmware_version();

    while(firmware_version_received == 0)
    {
        vTaskDelay(1);
    }

    strcpy(current_version, FIRMWARE_VERSION);
    token = strtok(current_version, ".");
    local_version_major = atoi(token);
    token = strtok(NULL, ".");
    local_version_minor = atoi(token);
    token = strtok(NULL, ".");
    local_version_patch = atoi(token);

    token = strtok(remote_firmware_version, ".");
    remote_version_major = atoi(token);
    token = strtok(NULL, ".");
    remote_version_minor = atoi(token);
    token = strtok(NULL, ".");
    remote_version_patch = atoi(token);

    if(remote_version_major > local_version_major)
    {
        ESP_LOGW(LOG_OTA, "Firmware MAJOR version is outdated. Remote version is %d.", remote_version_major);
        http_get_firmware_file();
    }
    else
    {
        if(remote_version_minor > local_version_minor)
        {
            ESP_LOGW(LOG_OTA, "Firmware MINOR version is outdated. Remote version is %d.", remote_version_minor);
            http_get_firmware_file();
        }
        else
        {
            if(remote_version_minor > local_version_patch)
            {
                ESP_LOGW(LOG_OTA, "Firmware PATCH version is outdated. Remote version is %d.", remote_version_patch);
                http_get_firmware_file();
            }
            else
            {
                ESP_LOGI(LOG_OTA, "Firmware is up to date.");
            }
        }
    }
    
    while(1)
    {

        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(100);

        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(100);

    }
}
