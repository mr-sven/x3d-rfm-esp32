/**
 * @file x3d_handler.c
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief
 * @version 0.1
 * @date 2023-12-06
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "rfm.h"
#include "x3d.h"
#include "x3d_handler.h"

#define X3D_DEFAULT_MSG_READ_RETRY          2
#define X3D_DEFAULT_MSG_RETRY               4
#define X3D_TEMP_MSG_RETRY                  1

uint32_t x3d_device_id;
uint8_t x3d_buffer[64];
uint8_t x3d_msg_no = 1;
uint16_t x3d_msg_id = 1;

static inline int no_of_devices(uint16_t mask)
{
    return __builtin_popcount(mask);
}

static inline int get_lowest_zerobit(uint16_t value)
{
    return __builtin_ctz(~value);
}

void x3d_processor(uint8_t *buffer)
{
    //ESP_LOG_BUFFER_HEX_LEVEL(TAG, buffer, buffer[0], ESP_LOG_INFO);
    /*
     * It is possible to make all checks by hand:
     *  * message number
     *  * message type
     *  * message id
     *  * device id
     *  * header checksum
     * It is easier and faster to do a memcmp over the whole message header, because it will always returned 1 to 1 by the responding device.
     */
    // get length of the request message header
    uint8_t check_length = (x3d_buffer[X3D_IDX_HEADER_LEN] & X3D_HEADER_LENGTH_MASK) + X3D_IDX_HEADER_LEN;

    // if header does not match, ignore
    if (memcmp(x3d_buffer, buffer, check_length) != 0)
    {
        return;
    }

    /*
     * I don't know how the data processing is done in deltadore devices, here is how I do it.
     * Every mesh device has its own bits in bitfields and 16bit data response slots, so we can assume the following.
     * As long as the retry value is greater than the current one, we can bitwise or the whole payload except the retry value.
     * In general all mesh device update their payloads so the the highest retry value packet should contain all data.
     * But if the Mesh is cut in half, so that not all devices can communicate to all and only the station has the whole connectivity, it could
     * be possible that some data is missing, so the following process continiously updates the data.
     *
     * There may be a gap, for example in pairing process the paring pin is also a shared 16bit field, but if more than one device is in pairing,
     * then all devices return the same retry count value, so the message with the overlapping pin should be ignored.
     */

    // payload index is the end of the header check
    uint8_t payload_index = check_length;

    // check if retry count lower than the received
    if (x3d_buffer[payload_index] >= buffer[payload_index])
    {
        return;
    }

    // update retry count
    x3d_buffer[payload_index] = buffer[payload_index];

    // step over retry count, walk to the end
    for (payload_index++; payload_index < x3d_buffer[0]; payload_index++)
    {
        // orwise bytes to the buffer
        x3d_buffer[payload_index] |= buffer[payload_index];
    }
}

void x3d_set_device_id(uint32_t device_id)
{
    x3d_device_id = device_id;
}

uint8_t x3d_prepare_message(uint8_t network, x3d_msg_type_t msg_type, uint8_t flags, uint8_t status, uint8_t *ext_header, int ext_header_len)
{
    x3d_init_message(x3d_buffer, x3d_device_id, 0x80 | network);
    return x3d_prepare_message_header(x3d_buffer, &x3d_msg_no, msg_type, flags, status, ext_header, ext_header_len, x3d_enc_msg_id(&x3d_msg_id, x3d_device_id));
}

void x3d_transmit(void)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    do
    {
        x3d_set_crc(x3d_buffer);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(X3D_MSG_DELAY_MS));
        rfm_transfer(x3d_buffer, x3d_buffer[0]);
    } while (x3d_dec_retry(x3d_buffer) > 0);
    rfm_receive();
}

/***********************************************
 * X3D pairing message handler
 */

int x3d_pairing_proc(x3d_pairing_data_t *data)
{
    uint8_t ext_header[] = {0x98, 0x00};
    uint8_t payload_index = x3d_prepare_message(data->network, X3D_MSG_TYPE_PAIRING, 0, 0x85, ext_header, sizeof(ext_header));

    // get first free slot
    uint8_t target_device_no = get_lowest_zerobit(data->transfer);
    uint8_t target_slot = target_device_no;

    // generate ack mask from slot number
    uint16_t ack_mask = 1 << target_slot;

    // add unknown high nibble flag
    if (no_of_devices(data->transfer) > 0 && target_slot & 0x01)
    {
        target_slot |= 0x10;
    }

    x3d_set_message_retrans(x3d_buffer, payload_index, X3D_DEFAULT_MSG_RETRY, data->transfer);
    x3d_set_pairing_data(x3d_buffer, payload_index, target_slot, 0, X3D_PAIR_STATE_OPEN);

    // transfer buffer
    x3d_transmit();

    // wait to process responses
    vTaskDelay(pdMS_TO_TICKS(5000));

    uint16_t pin = x3d_get_pairing_pin(x3d_buffer, payload_index);
    if (pin != 0)
    {
        payload_index = x3d_prepare_message(data->network, X3D_MSG_TYPE_PAIRING, 0, 0x85, ext_header, sizeof(ext_header));
        x3d_set_message_retrans(x3d_buffer, payload_index, 4, data->transfer);
        x3d_set_pairing_data(x3d_buffer, payload_index, target_slot, pin, X3D_PAIR_STATE_PINNED);

        // transfer buffer
        x3d_transmit();

        // wait to process responses
        vTaskDelay(pdMS_TO_TICKS((no_of_devices(data->transfer) + 1) * 4 * X3D_MSG_DELAY_MS));

        if ((x3d_get_retrans_ack(x3d_buffer, payload_index) & ack_mask) == ack_mask)
        {
            // update device mask
            data->transfer = data->transfer | ack_mask;
            return target_device_no;
        }
    }
    return -1;
}

/***********************************************
 * X3D unpairing handler
 */

void x3d_unpairing_proc(x3d_unpairing_data_t *data)
{
    uint8_t ext_header[] = {0x98, 0x00};
    uint8_t payload_index = x3d_prepare_message(data->network, X3D_MSG_TYPE_STANDARD, 0, 0x05, ext_header, sizeof(ext_header));
    x3d_set_message_retrans(x3d_buffer, payload_index, X3D_DEFAULT_MSG_RETRY, data->transfer);
    uint16_t target_mask = (1 << (data->target & 0x0f));

    x3d_set_unpair_device(x3d_buffer, payload_index, target_mask);

    // transfer buffer
    x3d_transmit();

    // wait to process responses
    vTaskDelay(pdMS_TO_TICKS(no_of_devices(data->transfer) * X3D_DEFAULT_MSG_RETRY * X3D_MSG_DELAY_MS));

    // remove device from transfer mask
    data->transfer &= ~(target_mask);
}

/***********************************************
 * X3D register read handler
 */

x3d_standard_msg_payload_t * x3d_reading_proc(x3d_read_data_t *data)
{
    uint8_t ext_header[] = {0x98, 0x00};
    uint8_t payload_index = x3d_prepare_message(data->network, X3D_MSG_TYPE_STANDARD, 0, 0x05, ext_header, sizeof(ext_header));
    x3d_set_message_retrans(x3d_buffer, payload_index, X3D_DEFAULT_MSG_READ_RETRY, data->transfer);
    x3d_set_register_read(x3d_buffer, payload_index, data->target, data->register_high, data->register_low);

    // transfer buffer
    x3d_transmit();

    // wait to process responses
    vTaskDelay(pdMS_TO_TICKS(no_of_devices(data->transfer) * X3D_DEFAULT_MSG_RETRY * X3D_MSG_DELAY_MS));

    return (x3d_standard_msg_payload_t *)&x3d_buffer[payload_index + 1];
}

/***********************************************
 * X3D register write handler
 */

x3d_standard_msg_payload_t * x3d_writing_proc(x3d_write_data_t *data)
{
    uint8_t ext_header[] = {0x98, 0x00};
    uint8_t payload_index = x3d_prepare_message(data->network, X3D_MSG_TYPE_STANDARD, 0, 0x05, ext_header, sizeof(ext_header));
    x3d_set_message_retrans(x3d_buffer, payload_index, X3D_DEFAULT_MSG_RETRY, data->transfer);
    x3d_set_register_write(x3d_buffer, payload_index, data->target, data->register_high, data->register_low, data->values);

    // transfer buffer
    x3d_transmit();

    // wait to process responses
    vTaskDelay(pdMS_TO_TICKS(no_of_devices(data->transfer) * X3D_DEFAULT_MSG_RETRY * X3D_MSG_DELAY_MS));

    return (x3d_standard_msg_payload_t *)&x3d_buffer[payload_index + 1];
}

/***********************************************
 * X3D temp message handler
 */

void x3d_temp_proc(x3d_temp_data_t *data)
{
    uint8_t ext_header[] = {0x98, 0x08, data->outdoor, data->temp & 0xff, (data->temp >> 8) & 0xff};
    uint8_t payload_index = x3d_prepare_message(data->network, X3D_MSG_TYPE_STANDARD, 0, 0x05, ext_header, sizeof(ext_header));
    x3d_set_message_retrans(x3d_buffer, payload_index, X3D_TEMP_MSG_RETRY, data->transfer);
    x3d_set_ping_device(x3d_buffer, payload_index, data->target);

    // transfer buffer
    x3d_transmit();

    // wait to process responses
    vTaskDelay(pdMS_TO_TICKS(no_of_devices(data->transfer) * X3D_DEFAULT_MSG_RETRY * X3D_MSG_DELAY_MS));
}