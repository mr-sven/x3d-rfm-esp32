# X3D PPED Sub Protocol

List of known networks

* `0xc0`   Request
* `0x04`   Response

List of known message types

* `0`   Pairing
* `5`   Data

## Header

The header payload is 5 bytes long including magic id.

## Payload

The payload is minimal 5 bytes long.

It contains a retry counter, a unknown byte and three byte target device id. Plus payload data.
The retry counter byte is combined in flags and counter, the counter is limited to the lowest 3 bits.

The Retry flag is commonly 0x30 and for response 0x38 plus the counter, mostly 3.

## Device Pairing

The requestor sends a pairing request message with peer device id `0xffffff`.

### Requestor
```c
// x3d_ctx should be global
x3d_context_t x3d_ctx = {
    .device_id = 0x123456,
    .magic_id  = 1,
    .msg_no    = 1,
};

x3d_pped_pairing_req(tx_buffer, &x3d_ctx);
// 0x17, 0xFF, 0x01, 0x00, 0x0C, 0x56, 0x34, 0x12, 0xC0, 0x85, 0x98, 0x00, 0x8C, 0xFE, 0xFB, 0xFD, 0x32, 0x01, 0xFF, 0xFF, 0xFF, 0x6E, 0x23

TickType_t last_send_time = xTaskGetTickCount();
do
{
    vTaskDelayUntil(&last_send_time, pdMS_TO_TICKS(X3D_MSG_DELAY_MS));
    x3d_transfer(tx_buffer, tx_buffer[0]);
} while (x3d_proto_dec_retry(tx_buffer) > 0);

// wait for receiving data
uint8_t retry_cnt;
uint32_t peer_device_id;
uint16_t peer_magic_id;

if (x3d_pped_parse_pairing_resp(rx_buffer, &x3d_ctx, &peer_device_id, &retry_cnt, &peer_magic_id) == 0)
{
    // store peer_magic_id and peer_device_id as paired peer
}
```

### Responder
```c
// x3d_ctx should be global
x3d_context_t x3d_ctx = {
    .device_id = 0x654321,
    .magic_id  = 1,
    .msg_no    = 1,
};
uint8_t retry_cnt; 
uint32_t peer_device_id;
uint16_t peer_magic_id;

if (x3d_pped_parse_pairing_req(rx_buffer, &peer_device_id, &retry_cnt, &peer_magic_id) == 0)
{
    // store peer_magic_id and peer_device_id as paired peer
    int delay                 = retry_cnt * X3D_MSG_DELAY_MS;
    TickType_t last_send_time = xTaskGetTickCount();

    x3d_pped_pairing_resp(tx_buffer, &x3d_ctx, peer_device_id);
    // 0x17, 0xFF, 0x01, 0x00, 0x0C, 0x21, 0x43, 0x65, 0x04, 0x85, 0x98, 0x00, 0x3C, 0xB4, 0xFD, 0x26, 0x3A, 0x05, 0x56, 0x34, 0x12, 0x63, 0x05

    vTaskDelayUntil(&last_send_time, pdMS_TO_TICKS(delay));
    x3d_transmit();
}
```

## Device Data

The data request may contain a sub register addess and/or additional payload.

The register value contains two known flags:
* `0x8000` - is response
* `0x4000` - is write

### Requestor

Read reg 1234, payload 0x10e1 or sub register 4321.
```c
// x3d_ctx should be global
x3d_context_t x3d_ctx = {
        .device_id = 0x123456,
        .magic_id  = 1,
        .msg_no    = 1,
};

x3d_pped_register_t reg_data = {
        .peer_device_id     = 0x654321,
        .reg                = 1234,
        .reg_payload_length = 2,
        .reg_payload        = (uint8_t[]){0xE1, 0x10},
};

x3d_pped_register_req(tx_buffer, &x3d_ctx, &reg_data);
// 0x1B, 0xFF, 0x01, 0x05, 0x0C, 0x56, 0x34, 0x12, 0xC0, 0x05, 0x98, 0x00, 0x8C, 0xFE, 0xFC, 0x7D, 0x32, 0x00, 0x21, 0x43, 0x65, 0xD2, 0x04, 0xE1, 0x10, 0x2C, 0x2B

// transfer and wait for receiving data

if (x3d_pped_parse_register_resp(rx_buffer, &x3d_ctx, &reg_data) == 0)
{
    if (reg_data.peer_device_id == 0x654321)
    {
        // do something with data
    }
}
```

### Responder
```c
x3d_context_t x3d_ctx = {
        .device_id = 0x654321,
};

x3d_pped_register_t reg_data;
if(x3d_pped_parse_register_req(rx_buffer, &x3d_ctx, &reg_data) == 0)
{
    if (reg_data.peer_device_id == 0x123456)
    {
        // do something with data
        if (reg_data.reg == 1234 && reg_data.reg_payload_length == 2)
        {
            uint16_t sub_reg;
            x3d_read_le_u16(&sub_reg, reg_data.reg_payload, 0);
            if (sub_reg == 0x4321 && !reg_data.is_write)
            {
                free(reg_data.reg_payload);

                uint8_t data[]              = {0xE1, 0x10, 0x01, 0x02, 0x03, 0x04};
                reg_data.reg_payload_length = sizeof(data);
                reg_data.reg_payload        = malloc(reg_data.reg_payload_length);
                memcpy(reg_data.reg_payload, data, reg_data.reg_payload_length);
                x3d_pped_register_resp(tx_buffer, &x3d_ctx, &reg_data);
                // 0x1F, 0xFF, 0x01, 0x05, 0x0C, 0x21, 0x43, 0x65, 0x04, 0x05, 0x98, 0x00, 0x3C, 0xB4, 0xFD, 0xA6, 0x3A, 0x00, 0x56, 0x34, 0x12, 0xD2, 0x84, 0xE1, 0x10, 0x01, 0x02, 0x03, 0x04, 0xD1, 0x83
            }            
        }
    }
}
```

## Calybox 1020 WT Register

| requestor      | calybox | info |
|:---------------|:---- ---|:-----|
| 11 00 ff ff    | 11 80 03 00 01 e5 01 05 | 17: 3 Phase? e5(229) Voltage? |
| 10 00          | 10 80 30 30 00 3f 00 00 00 00 00 00 00 00 00 00 00 02 00 00 00 | 16: ???  2 = number heaters? |
| 11 04 01 00    | 11 84 01 00 39 07 | 1041-1: global modes? |
| 11 04 02 00    | 11 84 02 00 3f 07 39 07 | 1041-2: global modes? |
| 11 01 01 00    | 11 81 01 00 f6 22 | 273-1: zone mode ? |
| 11 01 02 00    | 11 81 02 00 ff 27 f6 22 | 273-2: zone modes ? |
| 12 01 01 00    | 12 81 01 00 1c 08 | 274-1: 2076, 20.76 room temp |
| 12 01 02 00    | 12 81 02 00 00 80 1c 08 | 274-2: 0x8000 = no temp, 2076, 20.76 room temp |
| 10 02 00 04 f0 | 10 82 00 04 10 6b 16 1a ff ff ff ff b5 2d 05 00 09 31 03 00 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff | date + heating / cooling / hot water / outlets / other |
| 10 03 00 04 00 | 10 83 00 04 00 6b 16 1a 9a 4d 01 00 ff ff ff ff 84 9e 00 00 42 c2 00 00 a9 82 00 00 | date + ELEC_TOTAL |

* Reg 16 - only requested once on pairing
  * Content unknown, fixed config?
* Reg 17 - only requested once on pairing
  * Content unknown, fixed config?
* Reg 273 - Zone mode
* Reg 274 - Room temperature
* Reg 528 - Date + ELEC_TOTAL
* Reg 784 - Date + heating / cooling / hot water / outlets / other
* Reg 785 - Set zone modes
  * Write only
* Reg 1041 - Global mode
  * Read/Write
* Reg 1553

Following messages are send by switching functions in app:

| requestor   | calybox | info |
|:------------|:--------|:-----|
| 11 44 01 00 38 07       | ??? | 1041: turn off whole heating |
| 11 43 01 00 f1 07       | ??? | 785-1: heater 1 power safe |
| 11 43 01 00 f6 07       | ??? | 785-1: heater 1 combined mode |
| 11 43 01 00 f4 07       | ??? | 785-1: heater 1 defrost |
| 11 43 02 00 00 00 f1 07 | ??? | 785-2: heater 2 power safe |
| 11 43 02 00 00 00 f6 07 | ??? | 785-2: heater 2 combined mode |
| 11 43 02 00 00 00 f4 07 | ??? | 785-2: heater 2 defrost |
