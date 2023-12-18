# X3D Gateway

## MQTT Commands

Commands can be send to `/device/esp32/<device-id>/cmd` topic.

Status is published to `/device/esp32/<device-id>/status` topic.

Read and write result is published to `/device/esp32/<device-id>/result` topic.

## Planned MQTT commands (WIP)

Implementation is ongoing.

- [x] Paired device mask
- [x] Outdoor temperature
- [ ] Start Pairing
- [ ] Unpairing
- [x] Device status

### READ Paired device mask

This topic is retaining and contains the current bitmask of paired devices. Is set on startup or on pairing or unpairing.

* Topic: `/device/esp32/<device-id>/netMask4`
* Topic: `/device/esp32/<device-id>/netMask5`
* Payload: 16bit number bitmask

### WRITE Outdoor temperature

Publish outdoor temperature to all actors to that the thermostates can display it.

* Topic: `/device/esp32/<device-id>/outdoorTemp`
* Payload: temperature in °C with dot as floatingpoint separator, ex.: `-4.5`

### WRITE Start Pairing

This topic starts pairing mode.

* Topic: `/device/esp32/<device-id>/pair`
* Payload: 4 or 5, the network number, everything else is ignored

Response is published in status topic:
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

Response is published in result topic:
```json
{
    "action": "deviceStatus",
    "values": [
        {
            "net": 4,
            "device": 0,
            "roomTemp": 19.5,
            "enabled": true,
            "setPoint": 20.0,
            "defrost": false,
            "timed": false,
            "heaterOn": false,
            "heaterStopped": false,
            "windowOpen": false,
            "noTempSensor": true,
            "batteryLow": false
        },
        {
            "net": 4,
            "device": 1,
            "roomTemp": 19.5,
            "enabled": false,
            "setPoint": 20.0,
            "defrost": false,
            "timed": false,
            "heaterOn": false,
            "heaterStopped": false,
            "windowOpen": false,
            "noTempSensor": true,
            "batteryLow": false
        }
    ]
}
```

## Current MQTT commands

### General status data

* `off` - Submittet as LWT, so when the device is not connected it contains this va
* `idle` - Device is in idle mode


### Pairing Command

```json
{
    "action": "pair",
    "net": 4
}
```

* **net** - network number, should be 4 or 5

Status:
* `pairing` - device is in pairing
* `pairing failed` - pairing has failed or no device to pair found
* `pairing success` - new device paired

### Unpairing Command

```json
{
    "action": "unpair",
    "net": 4,
    "target": 1
}
```

* **net** - network number, should be 4 or 5
* **target** - zero based number of the device to unpair

### Read Command

```json
{
    "action": "read",
    "net": 4,
    "target": 1,
    "regHigh": 22,
    "regLow": 17
}
```

* **net** - network number, should be 4 or 5
* **target** - optional bitmask of the devices that should return their data
* **regHigh** - register high number
* **regLow** - register low number

Status:
* `reading` - device is in reading

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
