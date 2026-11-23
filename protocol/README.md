# Delta Dore X3D Protocol analysis

The bidirectional X3D protocol is used by various DeltaDore devices and also other vendors are building assets.

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

The first byte contains the length, including the length byte.

> This odd behavior makes it impossible to use the SX1211 inbuild crc check feature.

The second byte is `0xff` is is properbly an address field, but not used by X3D.

The third byte contains a rolling message number which is incremented on each new message send by initiator. Rollover is from 255 to 1, so zero is skipped.

The fourth is a message type byte. The type differs based on device sub protocol type.

The sixth byte is a merge of some bit fields and the length of the following message header.
The upper 3 bits may contain some flags, the lower 5 bits contains the length of the following message header.

After the header a payload can follow.

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

The next byte defines a target network. Based on device sub protocol type, each network has it's own meaning.

The last two bytes contains a checksum for the header. It is an Int16 big endian. It's the negative cross sum starting on header len byte.
Based on analyses this is the only value transfered in big endian.

The previous last two bytes may contain a encrypted magic id to prevent reply attacks.

```
|---------------------------------- cksum -------------------------|
 <flags|headeLen> <deviceId> <network> <headerPayload> <magicId?> <cksum>
```

### Header magic id

The magic id is based on a rolling 16 bit integer but omits zero, so 0xffff rolled over to 0x0001.
The number is encrypted using the 3 byte device id.

On the target device the number gets decrypted and compared to the last one, if the message id is lower or maximum 32 above the current number,
the device accepts or responds. Otherwise the message get relayed but is not taken care. The magic id check is reset after a specific time.


### Header Payload

The Header Payload contains a various list of flags and data.

The minimum length is maybe 3, the first byte contains some status flags.

## X3D sub protocol

The Payload, Header Payload and Commnication Flows differ on each type of device.

In general it differs first at protocol type. In the tydom firmware and tydom app are the following types defined:

* [x3d_rm](X3D_RM.md)
* x3d_rmlp
* x3d_rmloop
* x3d_ppa
* x3d_pp
* [x3d_pped](X3D_PPED.md)
* x3d_pps

These types define how communication is done (pairing, reading, writing) and also if it's master/slave or peer2peer or other modes.

The next level is a profile. That defines parameter to read or write.

* thermic
* thermicES
* shThermic
* temperature
* shutter
* shutterBrushless
* shutterActivHome
* shutterProjected
* light
* alarm
* sensor
* meter
* meterOnDemand
* meter_TyWatt5200
* meter_TyWatt5300
* rt2012_meas
* rt2012
* rt2012_noOutTemp
* pod
* multi
* generic
* detector
* remote
* minor1000
* typassAtl
* typassSaunier
* x3d_th
* x3d_meter
* podBelM
* emic
* diag
* gen
* opening
* boilerDrive
* controller
* unknown

### Known devices and protocols

* CALYBOX 1020 WT
  * proto: x3d_pped
  * profile: rt2012_noOutTemp

* RF 6600 FP
  * proto: x3d_rm
  * profile: thermic
