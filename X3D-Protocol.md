# Delta Dore X3D Protocol analysis

The bidirectional X3D protocol is used by various DeltaDore devices and also other vendors are building assets.

The Message flow is initiatet by Tydom or thermostat or sensor.

The initiating device sends a loop of 2 up to 5 messages, depending on message content, they contain a counting nibble which counts to zero.
After the message counts to zero, the actor respondes with it's informations.

The Initiating device sends the request every 17.75 ms until count zero.
The responding device waites (count + 1) * 18 ms and responds with it's answer in a loop for every 18 ms.

The protocol is mesh based. The maximum number of actors can be 16 in one mesh network.
The limitation is due to the maximum packet size of 64, on register read and write request, every device gets it's own 16bit data field, so the packet increases by 2 byte each device.

## RF Specifications

- Modulation: FSK PCM
- Frequency: 868.95MHz
- 25 µs bit time
- 40000 baud
- based on Semtech SX1211
- manual CRC
- CCITT whitening enabled in SX1211

The length including payload is whitened using CCITT whitening enabled in SX1211 chipset.
The payload contains some garbage at the end. The documentation of the SX1211 assume to
exclude the length byte from length calculation, but the CRC16 checksum at the end is so placed,
that the length byte is included. Maybe someone read the docs wrong. The garbage after the
checksum contains data from previous larger messages.

## Basic RF Packet

- Preamble: `0xaaaaaaaa`
- Syncword: `0x8169967e`
- Length byte
- Payload
- CRC16(Poly=0x1021,Init=0x0000)

```
                       |--------- len -------|
                       |----- crc -----|
aa aa aa aa 81 69 96 7e <len> <payload> <crc>
```

## General Packet structure

The first byte is always `0xff`.

The second byte contains a rolling message number which is incremented on each new message send by initiator.

The third is a message type byte:

- `0x00`   sensor message (from window detector)
- `0x01`   standard message
- `0x02`   pairing message
- `0x03`   actor identifcation beacon (click/clack)

The fourth byte is a merge of some bit fields and the length of the following message header.
The upper 3 bits may contain some flags, the lower 5 bits contains the length of the following message header.

```
                    |-------- headeLen --------|
ff <msgNo> <msgType> <flags|headeLen> <header?> <payload?>
```

## General Packet Header structure

The first byte contains some bit fields and the length of the header.
The upper 3 bits may contain some flags, the lower 5 bits contains the length of the header.

Known flag `0x20` is set on sensor messages of window open detector and on beacon send continous by Tydom 1.0.

Flags (guessed):

- `0x20`   no response required

The next three bytes contains the device id, in little endian.

The next byte defines a mesh network.
- `0x00`   seems to be no network
- `0x40`   unknown
- `0x80`   network 0 ?
- `0x84`   network 4
- `0x85`   network 5

The last two bytes contains a checksum for the header. It is an Int16 big endian. It's the negative cross sum starting on header len byte.
Based on analyses this is the only value transfered in big endian.

The previous last two bytes may contain some random message id.

```
|---------------------------------- cksum -------------------------|
 <flags|headeLen> <deviceId> <network> <headerPayload> <messageId?> <cksum>
```

## Header Payload

The Header Payload contains a various list of flags and data. Followed a list of payloads and guessed data.

### MsgType 0x00 Sensor message

Received from window actor
```
41 82 01 00 00 00   window was opened
01 82 01 00 00 00   window was closed
00 82 01 00 00 00   window state was not changed
```

Received from Tydom 1.0 with its administrative device id
```
network 00 : 00 82 00 00 00 00 00 00 00 00                          continuously with some delay
network 80 : 01 8A 00 00 FF FF FF 00 00 00 00 2A CD                 at startup
network 40 : 02 82 00 03 08 12 04 00 32 00 00 00 00 00 00 01 04 21  at startup
```

### MsgType 0x01 Standard message

```
05 98 00 <messageId>
05 98 08 00 3E 06 <messageId>     3E 06 = 0x063e = 1598 / 100 = 15.98 °C room temperature
```

### MsgType 0x02 Pairing message

```
85 98 00 <messageId>
```

### MsgType 0x03 Beacon message

```
05 98 00 <messageId>
```

## Message Payload MsgType 0x01 Standard message

The first byte of the message payload is counting byte. The lower nibble is used as downcounter from the initiating device.
After the nibble is zero, the responding device can send responds.

The playload contains a set of bitfields which contains slots for different functions.
Every bitfield is 16 bit LE encoded. As maximum of 16 devices per network, every device has its bit slot.

After the counting byte, three bifields follows.

* Transfer slot, what devices should retransmit the package.
* Transfered slot, what device has retransmitted the package.
* Target slot, which device should take care of the data.

Then a data action byte follows.

* `0x01` - read single
* `0x08` - no read/write action
* `0x09` - write single
* `0x11` - read multiple
* `0x19` - write multiple

At next the register address is followed. It is not clear if there is any grouping, there are always two bytes and maybe some bitflags.

Current known / seen registers:
* `0x11 0x51` - Unknown
* `0x14 0x01` - Unknown
* `0x15 0x11` - (RO) Current Room Temp
* `0x15 0x21` - (RO) Current External Temp
* `0x16 0x01` - Unknown
* `0x16 0x11` - (RO) Current Target Temp and status
* `0x16 0x21` - Unknown
* `0x16 0x31` - (WR) Set current Target Temp and mode
* `0x16 0x41` - (RW) On/Off state
* `0x16 0x61` - (RW) Party on time in minutes / Holiday time in minutes starting from current time. (Days - 1) * 1440 + Current Time in Minutes
* `0x16 0x81` - (RW) Freeze Temp
* `0x16 0x91` - (RW) Night Temp and Day Temp
* `0x18 0x01` - Unknown
* `0x19 0x10` - (RO) On time lsb in seconds
* `0x19 0x90` - (RO) On time msb in seconds
* `0x1a 0x04` - Unknown

Then a acknowledge slot follows, where every ack device sets its bit.
Now the 2 byte of data for each device follows or is updated on a read command.

```
<cnt> <tx> <txAck> <target> <action> <register> <targetAck> <dataDevice0> ... <dataDeviceN>
```

### Mesh message flow

```
+---+                  +---+          +---+
| I |                  | 1 |          | 2 |
+---+                  +---+          +---+
  | --- 2-5x --- #1 ---> |              |
  | <------- #2 -------- | ---- #2 ---> |
  |                      | <--- #3 ---- |
  | <------- #4 -------- | ---- #4 ---> |
  |                      | <--- #5 ---- |
  | <------- #6 -------- | ---- #6 ---> |
  |                      | <--- #7 ---- |
```

The counting byte and bitslots are set in the following manner. The register address is represented as `rrrr`, the data for or from device 1 as `xxxx` and for device 2 `yyyy`.
The number of data slots increments with each assigned device.

On response messages the counting byte is used also to identify the sending device. The higher nibble contains the message count and the lower nibble contains the number of the sending device.

TX: All, Read: Device 1
```
#1: 04 0300 0000 0100 01 rrrr 0000 0000
    03 0300 0000 0100 01 rrrr 0000 0000
    02 0300 0000 0100 01 rrrr 0000 0000
    01 0300 0000 0100 01 rrrr 0000 0000
    00 0300 0000 0100 01 rrrr 0000 0000
#2: 10 0300 0100 0100 01 rrrr 0100 xxxx
#3: 11 0300 0300 0100 01 rrrr 0100 xxxx
#4: 20 0300 0300 0100 01 rrrr 0100 xxxx
#5: 21 0300 0300 0100 01 rrrr 0100 xxxx
#6: 30 0300 0300 0100 01 rrrr 0100 xxxx
#7: 31 0300 0300 0100 01 rrrr 0100 xxxx
```

TX: All, Read: Device 2
```
#1: 04 0300 0000 0200 11 rrrr 0000 0000 0000
    03 0300 0000 0200 11 rrrr 0000 0000 0000
    02 0300 0000 0200 11 rrrr 0000 0000 0000
    01 0300 0000 0200 11 rrrr 0000 0000 0000
    00 0300 0000 0200 11 rrrr 0000 0000 0000
#2: 10 0300 0100 0200 11 rrrr 0000 0000 0000
#3: 11 0300 0300 0200 11 rrrr 0200 0000 yyyy
#4: 20 0300 0300 0200 11 rrrr 0200 0000 yyyy
#5: 21 0300 0300 0200 11 rrrr 0200 0000 yyyy
#6: 30 0300 0300 0200 11 rrrr 0200 0000 yyyy
#7: 31 0300 0300 0200 11 rrrr 0200 0000 yyyy
```

TX: All, Read: All
```
#1: 04 0300 0000 0300 11 rrrr 0000 0000 0000
    03 0300 0000 0300 11 rrrr 0000 0000 0000
    02 0300 0000 0300 11 rrrr 0000 0000 0000
    01 0300 0000 0300 11 rrrr 0000 0000 0000
    00 0300 0000 0300 11 rrrr 0000 0000 0000
#2: 10 0300 0100 0300 11 rrrr 0100 xxxx 0000
#3: 11 0300 0300 0300 11 rrrr 0300 xxxx yyyy
#4: 20 0300 0300 0300 11 rrrr 0300 xxxx yyyy
#5: 21 0300 0300 0300 11 rrrr 0300 xxxx yyyy
#6: 30 0300 0300 0300 11 rrrr 0300 xxxx yyyy
#7: 31 0300 0300 0300 11 rrrr 0300 xxxx yyyy
```

TX: All, Write: Device 1
```
#1: 04 0300 0000 0100 09 rrrr 0000 xxxx
    03 0300 0000 0100 09 rrrr 0000 xxxx
    02 0300 0000 0100 09 rrrr 0000 xxxx
    01 0300 0000 0100 09 rrrr 0000 xxxx
    00 0300 0000 0100 09 rrrr 0000 xxxx
#2: 10 0300 0100 0100 09 rrrr 0100 xxxx
#3: 11 0300 0300 0100 09 rrrr 0100 xxxx
#4: 20 0300 0300 0100 09 rrrr 0100 xxxx
#5: 21 0300 0300 0100 09 rrrr 0100 xxxx
#6: 30 0300 0300 0100 09 rrrr 0100 xxxx
#7: 31 0300 0300 0100 09 rrrr 0100 xxxx
```

TX: All, Write: Device 2
```
#1: 04 0300 0000 0200 19 rrrr 0000 0000 yyyy
    03 0300 0000 0200 19 rrrr 0000 0000 yyyy
    02 0300 0000 0200 19 rrrr 0000 0000 yyyy
    01 0300 0000 0200 19 rrrr 0000 0000 yyyy
    00 0300 0000 0200 19 rrrr 0000 0000 yyyy
#2: 10 0300 0100 0200 19 rrrr 0000 0000 yyyy
#3: 11 0300 0300 0200 19 rrrr 0200 0000 yyyy
#4: 20 0300 0300 0200 19 rrrr 0200 0000 yyyy
#5: 21 0300 0300 0200 19 rrrr 0200 0000 yyyy
#6: 30 0300 0300 0200 19 rrrr 0200 0000 yyyy
#7: 31 0300 0300 0200 19 rrrr 0200 0000 yyyy
```

TX: All, Write: All
```
#1: 04 0300 0000 0300 19 rrrr 0000 xxxx yyyy
    03 0300 0000 0300 19 rrrr 0000 xxxx yyyy
    02 0300 0000 0300 19 rrrr 0000 xxxx yyyy
    01 0300 0000 0300 19 rrrr 0000 xxxx yyyy
    00 0300 0000 0300 19 rrrr 0000 xxxx yyyy
#2: 10 0300 0100 0300 19 rrrr 0100 xxxx yyyy
#3: 11 0300 0300 0300 19 rrrr 0300 xxxx yyyy
#4: 20 0300 0300 0300 19 rrrr 0300 xxxx yyyy
#5: 21 0300 0300 0300 19 rrrr 0300 xxxx yyyy
#6: 30 0300 0300 0300 19 rrrr 0300 xxxx yyyy
#7: 31 0300 0300 0300 19 rrrr 0300 xxxx yyyy
```

### Register 15 11/21

Room Temp and External Temp

3E 06 = 0x063e = 1598 / 100 = 15.98 °C

### Register 16 11

Current Target Temp and status

The first byte contains the current target Temperature in 0.5 °C Steps from 0 °C:

* 0x30 = 48 = 24.0 °C
* 0x2f = 47 = 23.5 °C
* 0x26 = 38 = 19.0 °C

The second byte contains a enum set of flags:

* 0x10 = 0b00010000 = Heater current on
* 0x20 = 0b00100000 = Heater disabled
* 0x80 = 0b10000000 = Window Open

### Register 16 31

Writes the current target temperatur and the mode. Currently only seen to be written.

The first byte contains the current target Temperature in 0.5 °C Steps from 0 °C.

The second byte is a enum containing the current mode:
* 0 = Manual mode
* 2 = Defrost mode
* 7 = Automatic mode
* 8 = Timed manual mode (party or holiday)

### Register 16 41

Sets or gets the current on / off state.

* 0x39 0x07 = on
* 0x38 0x07 = off

### Register 16 61

Contains the time in minutes of the Timed manual mode.

For party time it contains the minutes left in the party mode.

For holidays the formular is: (Days - 1) * 1440 + Current Time in Minutes.

### Register 16 81

The first byte contains the defrost temperature: 0x0e = 7 °C.

The second byte is unknown.

### Register 16 91

Store for Day & Night Temp.

The first byte contains the night temperature: 0x26 = 19 °C.

The second byte contains the day temperature: 0x2f = 23.5 °C.

### Register 19 10/90

Contains the overall time in seconds the heater was on as uint32.

* 19 10 contains the lsb
* 19 90 contains the msb

## Message Payload MsgType 0x02 Pairing message

The message payload is similar to message type 1