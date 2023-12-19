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
- [x] Read Register
- [ ] Write Register

### WRITE Device Reset

Send a message to this topic, restarts the controller and it may update via OTA

* Topic: `/device/esp32/<device-id>/reset`
* Payload: whatever

### READ Paired devices

These topics are retaining and contains the current data of the paired devices. The `<id>` can be from 0 to 15 and represents the device bit number.

On controller startup, the controller sends NULL to all unused device IDs to remove retaining data.

* Topic: `/device/esp32/<device-id>/4/<id>`
* Topic: `/device/esp32/<device-id>/5/<id>`
* Payload:
```json
{
    "roomTemp": 0,
    "setPoint": 0,
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

### WRITE Outdoor temperature

Publish outdoor temperature to all actors to that the thermostates can display it.

* Topic: `/device/esp32/<device-id>/outdoorTemp`
* Payload: temperature in °C with dot as floatingpoint separator, ex.: `-4.5`

### WRITE Start Pairing

This topic starts pairing mode.

* Topic: `/device/esp32/<device-id>/pair`
* Payload: 4 or 5, the network number, everything else is ignored

Response is published in status topic:
* `pairing` - device is in pairing
* `pairing failed` - pairing has failed or no device to pair found
* `pairing success` - new device paired

### WRITE Unpairing

This topic starts pairing mode.

* Topic: `/device/esp32/<device-id>/unpair`
* Payload: `<netNumber> <deviceBit>`
  * netNumber: 4 or 5, the network number, everything else is ignored
  * deviceBit: 0 to 15, bit number of the device to unpair

If the number not matches the network bitmask, the request is ignored.

### WRITE Device status

This topic requests the status of the current devices.

* Topic: `/device/esp32/<device-id>/deviceStatus`
* Payload: 4 or 5, the network number, everything else is ignored

Response is published to the corresponding device status topics.

### WRITE Read Register

This topic starts pairing mode.

* Topic: `/device/esp32/<device-id>/read`
* Payload: `<netNumber> <targetDevice?> <registerHigh> <registerLow>`
  * netNumber: 4 or 5, the network number, everything else is ignored
  * targetDevice optional: target device mask, if not submittet, all paired devices read
  * registerHigh: register high number
  * registerLow: register low number

If the number not matches the network bitmask, the request is ignored.

Status:
* `reading` - device is in reading

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
