#include "my_wifi.h"

static bool callbacks_not_null = 0;
static int s_retry_num = 0;
static EventGroupHandle_t s_wifi_event_group;

static void my_wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    } 
    else 
    {
        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
        {
            if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) 
            {
                esp_wifi_connect();
                s_retry_num++;
                ESP_LOGI(LOG_WIFI, "retry to connect to the AP");
            } 
            else 
            {
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            }
            ESP_LOGI(LOG_WIFI,"connect to the AP fail");
        } 
        else 
        {
            if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
            {
                ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
                ESP_LOGI(LOG_WIFI, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
                s_retry_num = 0;
                xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            }
        }
    }
}

static void my_wifi_init_sta(char* ssid, char* password)
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
                                                        &my_wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &my_wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = 
    {
        .sta = 
        {
	        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = 
            {
                .capable = true,
                .required = false
            },
        },
    };

    memcpy(wifi_config.sta.ssid, ssid, strlen(ssid));
    memcpy(wifi_config.sta.password, password, strlen(password));

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
    if (bits & WIFI_CONNECTED_BIT) 
    {
        ESP_LOGI(LOG_WIFI, "connected to ap SSID:%s", ssid);
    } 
    else 
    {
        if (bits & WIFI_FAIL_BIT) 
        {
            ESP_LOGI(LOG_WIFI, "Failed to connect to SSID:%s", ssid);
        } 
        else 
        {
            ESP_LOGE(LOG_WIFI, "UNEXPECTED EVENT");
        }
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}

static WIFI_CLIENT_USR_CALLBACKS_ts wifi_callbacks = {NULL};

esp_err_t my_wifi_set_callbacks(WIFI_CLIENT_USR_CALLBACKS_ts wifi_callbacks_param)
{
    if((wifi_callbacks_param.HTTP_EVENT_ERROR_callback == NULL) ||
       (wifi_callbacks_param.HTTP_EVENT_ON_CONNECTED_callback == NULL) ||
       (wifi_callbacks_param.HTTP_EVENT_HEADERS_SENT_callback == NULL) ||
       (wifi_callbacks_param.HTTP_EVENT_ON_HEADER_callback == NULL) ||
       (wifi_callbacks_param.HTTP_EVENT_ON_DATA_callback == NULL) ||
       (wifi_callbacks_param.HTTP_EVENT_ON_FINISH_callback == NULL) ||
       (wifi_callbacks_param.HTTP_EVENT_DISCONNECTED_callback == NULL))
    {
        return ESP_FAIL;
    }
    callbacks_not_null = true;
    memcpy(&wifi_callbacks, &wifi_callbacks_param, sizeof(WIFI_CLIENT_USR_CALLBACKS_ts));
    return ESP_OK;
}

esp_err_t my_wifi_client_event_handler(esp_http_client_event_t *evt)
{
    static uint32_t packet_counter = 0;

    if(callbacks_not_null == 0)
    {
        return ESP_FAIL;
    }

    switch (evt->event_id)
    {
        case HTTP_EVENT_ERROR:
            printf("HTTP_EVENT_ERROR\n");
            wifi_callbacks.HTTP_EVENT_ERROR_callback();
        break;

        case HTTP_EVENT_ON_CONNECTED:
            printf("HTTP_EVENT_ON_CONNECTED\n");
            wifi_callbacks.HTTP_EVENT_ON_CONNECTED_callback();
        break;

        case HTTP_EVENT_HEADERS_SENT:
            packet_counter = 0;
            printf("HTTP_EVENT_HEADERS_SENT\n");
            wifi_callbacks.HTTP_EVENT_HEADERS_SENT_callback();
        break;

        case HTTP_EVENT_ON_HEADER:
            printf("HTTP_EVENT_ON_HEADER: key=%s value=%s\n", evt->header_key, evt->header_value);
            wifi_callbacks.HTTP_EVENT_ON_HEADER_callback(evt->header_key, evt->header_value);
        break;

        case HTTP_EVENT_ON_DATA:
            printf("HTTP_EVENT_ON_DATA");
            wifi_callbacks.HTTP_EVENT_ON_DATA_callback(evt->data, evt->data_len, packet_counter);
            packet_counter++;
        break;

        case HTTP_EVENT_ON_FINISH:
            printf("HTTP_EVENT_ON_FINISH\n");
            wifi_callbacks.HTTP_EVENT_ON_FINISH_callback();
        break;

        case HTTP_EVENT_DISCONNECTED:
            printf("HTTP_EVENT_DISCONNECTED\n");
            wifi_callbacks.HTTP_EVENT_DISCONNECTED_callback();
        break;

        default:
            printf("EVENTO HTTP NAO TRATADO: %d\n", evt->event_id);
        break;
    }

    return ESP_OK;
}

void my_wifi_init(char* ssid, char* password)
{
    ESP_LOGI(LOG_WIFI, "ESP_WIFI_MODE_STA");
    my_wifi_init_sta(ssid, password);
}



