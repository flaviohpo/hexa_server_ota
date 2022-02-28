# hexa_server
A firmware server for ESP-32 clients to update their own firmware.
Basically the ESP32 reads from the server the current firmware version and if needed, it will update itself to the lastest version of the firmware asking the server to download the last firmware.

# Versioning system
## Semantic Versioning 2.0.0
Given a version number MAJOR.MINOR.PATCH, increment the:
MAJOR version when you make incompatible API changes,
MINOR version when you add functionality in a backwards compatible manner, and
PATCH version when you make backwards compatible bug fixes.
Additional labels for pre-release and build metadata are available as extensions to the MAJOR.MINOR.PATCH format.
https://semver.org/

# endpoints
/firmware_version: used to compare with the current device firmware version,
    if the current firmware is a lower version, the firmware will be updated.
    
/firmware_file: downloads the .bin file to replace the current firmware.

# limitations
1. Only one firmware.
2. No security.
3. No logging.
4. No error handling.
