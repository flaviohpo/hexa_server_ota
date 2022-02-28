# hexa_server
A firmware server for ESP-32 clients to update their own firmware.

# Versioning system
## Semantic Versioning 2.0.0
Given a version number MAJOR.MINOR.PATCH, increment the:
MAJOR version when you make incompatible API changes,
MINOR version when you add functionality in a backwards compatible manner, and
PATCH version when you make backwards compatible bug fixes.
Additional labels for pre-release and build metadata are available as extensions to the MAJOR.MINOR.PATCH format.
https://semver.org/

# endpoints
get firmware version: used to compare with the current device firmware version,
    if the current firmware is a lower version, the firmware will be updated.
get firmware binary: downloads the .bin file to replace the current firmware.
