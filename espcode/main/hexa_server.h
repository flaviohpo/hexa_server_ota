#ifndef __HEXA_SERVER_H_
#define __HEXA_SERVER_H_

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "my_wifi.h"

typedef struct
{
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
}VERSION_ts;

typedef struct 
{
    VERSION_ts version;
    uint32_t size; /// size in bytes.
}FIRMWARE_ts;

#define LOG_OTA "OTA"

esp_err_t hexa_srv_on_header_callback(char* key, char* value);
esp_err_t hexa_srv_on_data_version_callback(char* data, uint32_t data_size, uint32_t pkt_counter);
esp_err_t hexa_srv_on_data_firm_callback(char* data, uint32_t data_size, uint32_t pkt_counter);
esp_err_t hexa_srv_on_finish_firm_callback(void);

esp_err_t hexa_srv_set_version(VERSION_ts* version_ptr, char* version_str);
bool hexa_server_check_version(char* local_version);

#endif // __HEXA_SERVER_H_