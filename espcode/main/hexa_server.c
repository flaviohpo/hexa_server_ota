#include "hexa_server.h"
#include "ctype.h"

static esp_ota_handle_t ota_handle;
static const esp_partition_t *update_partition;
static FIRMWARE_ts firmware;

esp_err_t hexa_srv_on_header_callback(char* key, char* value)
{
    if((key == NULL) || (value == NULL))
    {
        return ESP_FAIL;
    }

    if(strcmp(key, "Content-Length") == 0)
    {
        firmware.size = atoi(value);
    }

    return ESP_OK;
}

esp_err_t hexa_srv_on_data_version(char* data, uint32_t data_size, uint32_t pkt_counter)
{
    char version_aux[32] = {0};
    char* token;
    strncpy(version_aux, data, data_size);
    token = strtok(version_aux, ".");
    firmware.version.major = atoi(token);
    token = strtok(NULL, ".");
    firmware.version.minor = atoi(token);
    token = strtok(NULL, ".");
    firmware.version.patch = atoi(token);
    return ESP_OK;
}

esp_err_t hexa_srv_on_data_firm(char* data, uint32_t data_size, uint32_t pkt_counter)
{
    if(pkt_counter == 0)
    {
        update_partition = esp_ota_get_next_update_partition(NULL);
        ESP_LOGI(LOG_OTA, "Partition label: '%s'", update_partition->label);
        ESP_LOGI(LOG_OTA, "Partition size: '%d'", update_partition->size);
        esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
        if (err != ESP_OK)
        {
            ESP_LOGE(LOG_OTA, "Error With OTA Begin, Cancelling OTA");
            return ESP_FAIL;
        }
        else
        {
            ESP_LOGI(LOG_OTA, "Writing to partition subtype %d at offset 0x%X", update_partition->subtype, update_partition->address);
            ESP_LOGI(LOG_OTA, "File Size: %d", firmware.size);
            if(esp_ota_write(ota_handle, data, data_size) != ESP_OK)
            {
                ESP_LOGE(LOG_OTA, "Linha:%d", __LINE__);
                return ESP_FAIL;
            }
        }
    }
    else
    {
        if(esp_ota_write(ota_handle, data, data_size) != ESP_OK)
        {
            ESP_LOGE(LOG_OTA, "Linha:%d", __LINE__);
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

esp_err_t hexa_srv_on_finish_firm(void)
{
    esp_err_t err = esp_ota_end(ota_handle);
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

    return err;
}
