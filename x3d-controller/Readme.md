# X3D Gateway

## MQTT Commands

Commands can be send to `/device/esp32/<device-id>/cmd` topic.

Status is published to `/device/esp32/<device-id>/status` topic.

Read and write result is published to `/device/esp32/<device-id>/result` topic.

### General status data

* `off` - Submittet as LWT, so when the device is not connected it contains this va
* `idle` - Device is in idle mode

### Info Command

Command:
```json
{
    "action": "info"
}
```

Result:
```json
{
    "action": "info",
    "net_4_mask": 3,
    "net_5_mask": 0
}
```

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

### Outdoor Temp Command

```json
{
    "action": "outdoorTemp",
    "temp": 20.0
}
```

* **temp** - float, temperature in °C

Status:
* `temp` - device is in temp sending


### Status Command

Command:
```json
{
    "action": "status"
}
```

Result:
```json
{
    "action": "status",
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

Status:
* `status` - device is in status read mode