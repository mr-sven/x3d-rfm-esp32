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

* `device/x3d/<device-id>/cmd` -> [MQTT Device commands](#mqtt-device-commands)
* `device/x3d/<device-id>/<net>/cmd` -> [MQTT Network commands](#mqtt-network-commands)
* `device/x3d/<device-id>/<net>/dest/<0..15,...>/cmd` -> [MQTT destination Device commands](#mqtt-destination-device-commands)

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

## MQTT Device commands

Topic:
* `device/x3d/<device-id>/cmd`

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

## MQTT Network commands

Topic:
* `device/x3d/<device-id>/net-4/cmd`
* `device/x3d/<device-id>/net-5/cmd`

### Pair command

The specified device is switched to pairing mode and is not responding anymore until timeout or device pairing is done.

* Payload: `pair <device-type>`
  * `<device-type>` - type of the device to pair
    * `rf66xx` RF 66XX temp actor
* Status
  * `pairing` - device is in pairing
  * `pairing failed` - pairing has failed or no device to pair found
  * `pairing success` - new device paired

### Device status command

This topic requests the status of the current devices.

* Payload: `device-status`
* Status
  * `status` - device is in status reading process

Response is published to the corresponding device status topics.

### Device status short command

This topic requests the status of the current devices. Reads only minimal no. registers from device to reduce RF traffic.

* Payload: `device-status-short`
* Status
  * `status` - device is in status reading process

Response is published to the corresponding device status topics.

## MQTT destination Device commands

Topic:
* `device/x3d/<device-id>/net-4/dest/<0..15,...>/cmd`
* `device/x3d/<device-id>/net-5/dest/<0..15,...>/cmd`

### Pair command

The specified device is switched to pairing mode and is not responding anymore until timeout or device pairing is done.

* Payload: `pair`
* Status
  * `pairing` - device is in pairing

### Unpair command

Unpairs the specified devices.

* Payload: `unpair`
* Status
  * `unpairing` - device is in unpairing

### Read command

Reads registers from devices and publishes the result to `device/x3d/<device-id>/result` topic.

* Payload: `read <registerHigh> <registerLow>`
  * `<registerHigh>` - register high number
  * `<registerLow>` - register low number
* Status
  * `reading` - device is in reading

### Write command

> TODO

Writes registers to devices.

* Payload: `write <registerHigh> <registerLow>   `
  * `<registerHigh>` - register high number
  * `<registerLow>` - register low number
* Status
  * `writing` - device is in reading

### Enable command

> TODO

* Payload: `enable <mode> <params?>`
  * `<mode>` - select active mode
  * `<params>` - optional: list of parameters required for specific mode

Possible mode values:

* `day` - activates day mode with configured temp, no params
* `night` - activates night mode with configured temp, no params
* `defrost` - activates defrost mode with configured temp, no params
* `custom` - activates custom temp mode, param: temp
  * param temperature in °C with dot as floatingpoint separator, ex.: `19.5`
* `timed` - activates timed temp mode, param: temp and time
  * param temperature in °C with dot as floatingpoint separator, ex.: `19.5`
  * param time in minutes

The actor does not differ between Party and Holiday mode. In the end its the time in minutes to stay in that mode.

The modes `day`, `night` and `defrost` will only be activated if full device status read the specified devices temperature parameters.

### Disable command

Unpairs the specified devices.

* Payload: `disable`