# hexa_server
A firmware server for ESP-32 clients to update their own firmware.

# endpoints
get firmware version: used to compare with the current device firmware version,
    if the current firmware is a lower version, the firmware will be updated.
get firmware binary: downloads the .bin file to replace the current firmware.