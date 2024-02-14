# Next Gen X3D Control Gateway

This controller version is capable of different device types. Each device type can have a set of features.

Current implemented devices with features:
* RF66XX: OUTDOOR_TEMP, TEMP_ACTOR

The features can be used to differ between actors/sensors and their faunctions.

## MQTT definitions

Mqtt topic prefix `/device/x3d/<device-id>`. The device id is based on the last 3 bytes of the MAC address and is also used as X3D device id.

Using kebab-case for topics.

Network can be `net-4` or `net-5`.

Destination devices can be addressed via suffix `/<net>/dest/<0..15,...>`. Depending on the command the destination number can be a comma separated list of numbers or only one number.

List of commands:

* device based
  * `/device/x3d/<device-id>/reset`
  * `/device/x3d/<device-id>/outdoor-temp`

* network based
  * `/device/x3d/<device-id>/<net>/pair`
  * `/device/x3d/<device-id>/<net>/device-status`
  * `/device/x3d/<device-id>/<net>/device-status-short`
  * `/device/x3d/<device-id>/<net>/read`

* destination based
  * `/device/x3d/<device-id>/<net>/dest/<0..15,...>/read`
  * `/device/x3d/<device-id>/<net>/dest/<0..15,...>/write`
  * `/device/x3d/<device-id>/<net>/dest/<0..15,...>/enable`
  * `/device/x3d/<device-id>/<net>/dest/<0..15,...>/disable`
  * `/device/x3d/<device-id>/<net>/dest/<0..15>/pair`
  * `/device/x3d/<device-id>/<net>/dest/<0..15>/unpair`

List of returns:

* `/device/x3d/<device-id>/status`
* `/device/x3d/<device-id>/result`
* `/device/x3d/<device-id>/<net>/dest/<0..15>/status`

### Status return

`/device/x3d/<device-id>/status`

* `off` - Submittet as LWT, so when the device is not connected it contains this value.
* `start` - Device is powering on.
* `idle` - Device is in idle mode.
* `reset` - Device is resetting.

### Result return

`/device/x3d/<device-id>/result`

### Device status return

`/device/x3d/<device-id>/<net>/dest/<0..15>/status`

### Reset request

Send a message to this topic, restarts the controller and it may update via OTA.

* Topic: `/device/x3d/<device-id>/reset`
* Payload: whatever

### Outdoor temperature request

Publish outdoor temperature to all actors so that the thermostates can display it.

* Topic: `/device/x3d/<device-id>/outdoor-temp`
* Payload: temperature in °C with dot as floatingpoint separator, ex.: `-4.5`
* Required feature: OUTDOOR_TEMP

Status:
* `temp` - device is in temp sending