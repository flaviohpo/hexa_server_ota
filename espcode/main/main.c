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

    get_worldclock_api();

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
