# X3D Gateway

## MQTT Commands

Commands can be send to `/device/esp32/<device-id>/cmd` topic.

Status is published to `/device/esp32/<device-id>/status` topic.

Read and write result is published to `/device/esp32/<device-id>/result` topic.

### General status data

* `off` - Submittet as LWT, so when the device is not connected it contains this va
* `idle` - Device is in idle mode

### Pairing Command

```json
{
    "action": "pair",
    "net": 4,
    "noOfDevices": 0
}
```

* **net** - network number, should be 4 or 5
* **noOfDevices** - number of already paired devices

Status:
* `pairing` - device is in pairing
* `pairing failed` - pairing has failed or no device to pair found
* `pairing success` - new device paired

### Unpairing Command

```json
{
    "action": "unpair",
    "net": 4,
    "transfer": 1,
    "target": 1
}
```

* **net** - network number, should be 4 or 5
* **transfer** - bitmask of the devices that should tranfer the message, should always all paired devices, except you want to analyze mesh structure.
* **target** - bitmask of the device to unpair

### Read Command

```json
{
    "action": "read",
    "net": 4,
    "transfer": 1,
    "target": 1,
    "regHigh": 22,
    "regLow": 17
}
```

* **net** - network number, should be 4 or 5
* **transfer** - bitmask of the devices that should tranfer the message, should always all paired devices, except you want to analyze mesh structure.
* **target** - bitmask of the devices that should return their data
* **regHigh** - register high number
* **regLow** - register low number

Status:
* `reading` - device is in reading

### Write Command

```json
{
    "action": "write",
    "net": 4,
    "transfer": 1,
    "target": 1,
    "regHigh": 22,
    "regLow": 17,
    "values": [
        1598
    ]
}
```

* **net** - network number, should be 4 or 5
* **transfer** - bitmask of the devices that should tranfer the message, should always all paired devices, except you want to analyze mesh structure.
* **target** - bitmask of the devices that should accept the data
* **regHigh** - register high number
* **regLow** - register low number
* **values** - an int array of register values, if only one element in then all target devices gets the same value, otherwise apply diffrent values to each device

Status:
* `writing` - device is in writing