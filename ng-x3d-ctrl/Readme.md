# Next Gen X3D Control Gateway

This controller version is capable of different device types. Each device type can have a set of features.

Current implemented devices with features:
* RF66XX: OUTDOOR_TEMP, TEMP_ACTOR

The features can be used to differ between actors/sensors and their faunctions.

## MQTT definitions

Mqtt topic prefix `device/x3d/<device-id>`. The device id is based on the last 3 bytes of the MAC address and is also used as X3D device id.

Using kebab-case for topics.

Network can be `net-4` or `net-5`.

Destination devices can be addressed via suffix `../<net>/dest/<0..15,...>`. Depending on the command the destination number can be a comma separated list of numbers or only one number.

List of subscribed topics:

* `device/x3d/<device-id>/cmd` -> [MQTT Device Commands](#mqtt-device-commands)
* `device/x3d/<device-id>/<net>/cmd`
* `device/x3d/<device-id>/<net>/dest/<0..15,...>/cmd`

List of publish topics:
* `device/x3d/<device-id>/status`
* `device/x3d/<device-id>/result`
* `device/x3d/<device-id>/<net>/dest/<0..15>/status`

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

## MQTT device commands

### Reset command

* Payload: `reset`

This command restarts the controller and it may update via OTA.

### Outdoor temperature command

Publish outdoor temperature to all actors so that the thermostates can display it.

* Payload: `outdoor-temp <temp>`
  * `<temp>` - temperature in °C with dot as floatingpoint separator, ex.: `-4.5`
* Required destination feature
  * `OUTDOOR_TEMP`
* Status
  * `temp` - device is in temp sending

## MQTT destination device Commands

### Read command

Reads registers from devices and publishes the result to `device/x3d/<device-id>/result` topic.

* Payload: `read <registerHigh> <registerLow>`
  * `<registerHigh>` - register high number
  * `<registerLow>` - register low number
* Status
  * `reading ` - device is in reading
