# X3D Gateway

## MQTT

Commands can be send to `/device/esp32/<device-id>/cmd` topic.

Status is published to `/device/esp32/<device-id>/status` topic.

Read and write result is published to `/device/esp32/<device-id>/result` topic.

## General status data

* `off` - Submittet as LWT, so when the device is not connected it contains this va
* `idle` - Device is in idle mode

## MQTT commands (WIP)

Implementation is ongoing.

- [x] Device Reset
- [x] Paired devices
- [x] Outdoor temperature
- [x] Start Pairing
- [x] Unpairing
- [x] Device status
- [x] Device status short
- [x] Read Register
- [x] Enable Device
- [x] Disable Device
- [ ] Write Register

## Device Reset (PUB)

Send a message to this topic, restarts the controller and it may update via OTA

* Topic: `/device/esp32/<device-id>/reset`
* Payload: whatever

Status:
* `reset` - device is in reset

## Paired devices (SUB)

These topics are retaining and contains the current data of the paired devices. The `<id>` can be from 0 to 15 and represents the device bit number.

On controller startup, the controller sends NULL to all unused device IDs to remove retaining data.

* Topic: `/device/esp32/<device-id>/4/<id>`
* Topic: `/device/esp32/<device-id>/5/<id>`
* Payload:
```json
{
    "roomTemp": 0,
    "setPoint": 0,
    "setPointDay": 0,
    "setPointNight": 0,
    "setPointDefrost": 0,
    "power": 0,
    "enabled": false,
    "onAir": false,
    "flags": [
        "defrost",
        "timed",
        "heaterOn",
        "heaterStopped",
        "windowOpen",
        "noTempSensor",
        "batteryLow"
    ]
}
```

## Outdoor temperature (PUB)

Publish outdoor temperature to all actors so that the thermostates can display it.

* Topic: `/device/esp32/<device-id>/outdoorTemp`
* Payload: temperature in °C with dot as floatingpoint separator, ex.: `-4.5`

Status:
* `temp` - device is in temp sending

## Start Pairing (PUB)

This topic starts pairing mode.

* Topic: `/device/esp32/<device-id>/pair`
* Payload: `<netNumber> <deviceBit?>`
  * netNumber: 4 or 5, the network number, everything else is ignored
  * deviceBit optional: 0 to 15, bit number of the device to start pair

If deviceBit is submitted, the specified device is switched to pairing mode and is not responding anymore until timeout or device pairing is done.

Response is published in status topic:
* `pairing` - device is in pairing
* `pairing failed` - pairing has failed or no device to pair found
* `pairing success` - new device paired

## Unpairing (PUB)

This topic starts unpairing mode.

* Topic: `/device/esp32/<device-id>/unpair`
* Payload: `<netNumber> <deviceBit>`
  * netNumber: 4 or 5, the network number, everything else is ignored
  * deviceBit: 0 to 15, bit number of the device to unpair

If the number not matches the network bitmask, the request is ignored.

Status:
* `unpairing` - device is in unpairing process

## Device status (PUB)

This topic requests the status of the current devices.

* Topic: `/device/esp32/<device-id>/deviceStatus`
* Payload: 4 or 5, the network number, everything else is ignored

Response is published to the corresponding device status topics.

Status:
* `status` - device is in status reading process

## Device status short (PUB)

This topic requests the status of the current devices. Reads only minimal no. registers from device to reduce RF traffic.

* Topic: `/device/esp32/<device-id>/deviceStatusShort`
* Payload: 4 or 5, the network number, everything else is ignored

Response is published to the corresponding device status topics.

Status:
* `status` - device is in status reading process

## Read Register (PUB)

This topic reads registers from devices asn publishes the result to `/device/esp32/<device-id>/result` topic.

* Topic: `/device/esp32/<device-id>/read`
* Payload: `<netNumber> <targetDevice?> <registerHigh> <registerLow>`
  * netNumber: 4 or 5, the network number, everything else is ignored
  * targetDevice optional: target device mask, if not submittet, all paired devices read
  * registerHigh: register high number
  * registerLow: register low number

If the number not matches the network bitmask, the request is ignored.

Status:
* `reading` - device is in reading

## Enable Device (PUB)

* Topic: `/device/esp32/<device-id>/enable`
* Payload: `<netNumber> <targetDevice> <mode> <params?>`
  * netNumber: 4 or 5, the network number, everything else is ignored
  * targetDevice: target device mask
  * mode: select active mode
  * params optional: list of parameters required for specific mode

If the number not matches the network bitmask, the request is ignored.

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

## Disable Device (PUB)

* Topic: `/device/esp32/<device-id>/disable`
* Payload: `<netNumber> <targetDevice>`
  * netNumber: 4 or 5, the network number, everything else is ignored
  * targetDevice: target device mask

If the number not matches the network bitmask, the request is ignored.

## Legacy MQTT commands

### Write Command

```json
{
    "action": "write",
    "net": 4,
    "target": 1,
    "regHigh": 22,
    "regLow": 17,
    "values": [
        1598
    ]
}
```

* **net** - network number, should be 4 or 5
* **target** - bitmask of the devices that should accept the data
* **regHigh** - register high number
* **regLow** - register low number
* **values** - an int array of register values, if only one element in then all target devices gets the same value, otherwise apply diffrent values to each device

Status:
* `writing` - device is in writing
