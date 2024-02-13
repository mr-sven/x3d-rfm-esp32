# Next Gen X3D Control Gateway



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
  * `/device/x3d/<device-id>/<net>/dest/<0..15>/unpair`

List of returns:

* `/device/x3d/<device-id>/status`
* `/device/x3d/<device-id>/result`
* `/device/x3d/<device-id>/<net>/dest/<0..15>/status`