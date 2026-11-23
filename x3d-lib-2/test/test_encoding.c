#include <assert.h>
#include <string.h>
#include <stdint.h>
#include "helpers.h"
#include "../src/x3d_proto.h"

/**
 * Tests are incremental. Building test cases on top of other tests deduplicate code, so test fuctions return their data.
 */

x3d_msg_t *test_x3d_proto_init_message(void)
{
    x3d_msg_t *msg = x3d_proto_init_message(1, 0x123456, 2);
    assert(msg != NULL);
    assert(msg->msg_header.pkt_len == 0);
    assert(msg->msg_header.pkt_addr == 0xff);
    assert(msg->msg_header.msg_no == 0);
    assert(msg->msg_header.msg_type == 1);
    assert(msg->msg_header.head_len_u == 0);
    assert(msg->msg_header.device_id == 0x123456);
    assert(msg->msg_header.net == 2);
    assert(msg->header_payload_length == 0);
    assert(msg->payload_length == 0);
    assert(msg->header_magic == 0);
    return msg;
}

x3d_msg_t *test_x3d_proto_prepare_message_header_no_payload_no_magic(void)
{
    x3d_msg_t *msg = test_x3d_proto_init_message();

    uint8_t msg_no = 255; // test rollover
    x3d_proto_prepare_message_header(msg, &msg_no, 1, 0x85, NULL, 0, NULL);

    assert(msg_no == 1);
    assert(msg->msg_header.msg_no == 1);
    assert(msg->msg_header.head_flags == 1);
    assert(msg->header_payload_length == 1);
    assert(msg->header_payload[0] == 0x85);
    assert(msg->msg_header.head_len == 8);
    return msg;
}

x3d_msg_t *test_x3d_proto_prepare_message_header_no_magic()
{
    x3d_msg_t *msg = test_x3d_proto_init_message();

    uint8_t msg_no   = 255; // test rollover
    uint8_t header[] = {0x98, 0x00};
    x3d_proto_prepare_message_header(msg, &msg_no, 1, 0x85, header, sizeof(header), NULL);

    assert(msg_no == 1);
    assert(msg->msg_header.msg_no == 1);
    assert(msg->msg_header.head_flags == 1);
    assert(msg->header_payload_length == 3);
    assert(msg->header_payload[0] == 0x85);
    assert(msg->header_payload[1] == 0x98);
    assert(msg->header_payload[2] == 0x00);
    assert(msg->msg_header.head_len == 10);
    return msg;
}

x3d_msg_t *test_x3d_proto_prepare_message_header_no_payload(void)
{
    x3d_msg_t *msg = test_x3d_proto_init_message();

    uint8_t msg_no    = 255;   // test rollover
    uint16_t magic_id = 65535; // test rollover
    x3d_proto_prepare_message_header(msg, &msg_no, 1, 0x85, NULL, 0, &magic_id);

    assert(msg_no == 1);
    assert(magic_id == 1);
    assert(msg->msg_header.msg_no == 1);
    assert(msg->msg_header.head_flags == 1);
    assert(msg->header_magic == 1);
    assert(msg->header_payload_length == 3);
    assert(msg->header_payload[0] == 0x85);
    assert(msg->header_payload[1] == 0x00);
    assert(msg->header_payload[2] == 0x00);
    assert(msg->msg_header.head_len == 10);
    return msg;
}

x3d_msg_t *test_x3d_proto_prepare_message_header(void)
{
    x3d_msg_t *msg = test_x3d_proto_init_message();

    uint8_t msg_no    = 255;   // test rollover
    uint16_t magic_id = 65535; // test rollover
    uint8_t header[]  = {0x98, 0x00};
    x3d_proto_prepare_message_header(msg, &msg_no, 1, 0x85, header, sizeof(header), &magic_id);

    assert(msg_no == 1);
    assert(magic_id == 1);
    assert(msg->msg_header.msg_no == 1);
    assert(msg->msg_header.head_flags == 1);
    assert(msg->header_magic == 1);
    assert(msg->header_payload_length == 5);
    assert(msg->header_payload[0] == 0x85);
    assert(msg->header_payload[1] == 0x98);
    assert(msg->header_payload[2] == 0x00);
    assert(msg->header_payload[3] == 0x00);
    assert(msg->header_payload[4] == 0x00);
    assert(msg->msg_header.head_len == 12);
    return msg;
}

void test_x3d_proto_write_to_buffer_no_payload_no_header_payload_no_magic(void)
{
    x3d_msg_t *msg = test_x3d_proto_prepare_message_header_no_payload_no_magic();

    uint8_t buffer[64];
    x3d_proto_write_to_buffer(buffer, msg);
    delete_msg(msg);

    uint8_t test_result[] = {0x0E, 0xFF, 0x01, 0x01, 0x28, 0x56, 0x34, 0x12, 0x02, 0x85, 0xFE, 0xDD, 0x7D, 0xD3};
    assert(memcmp(buffer, test_result, test_result[0]) == 0);
}

void test_x3d_proto_write_to_buffer_no_payload_no_magic(void)
{
    x3d_msg_t *msg = test_x3d_proto_prepare_message_header_no_magic();

    uint8_t buffer[64];
    x3d_proto_write_to_buffer(buffer, msg);
    delete_msg(msg);

    uint8_t test_result[] = {0x10, 0xFF, 0x01, 0x01, 0x2A, 0x56, 0x34, 0x12, 0x02, 0x85, 0x98, 0x00, 0xFE, 0x45, 0x0C, 0x4B};
    assert(memcmp(buffer, test_result, test_result[0]) == 0);
}

void test_x3d_proto_write_to_buffer_no_payload_no_header_payload(void)
{
    x3d_msg_t *msg = test_x3d_proto_prepare_message_header_no_payload();

    uint8_t buffer[64];
    x3d_proto_write_to_buffer(buffer, msg);
    delete_msg(msg);

    uint8_t test_result[] = {0x10, 0xFF, 0x01, 0x01, 0x2A, 0x56, 0x34, 0x12, 0x02, 0x85, 0x8C, 0xFE, 0xFD, 0x53, 0x02, 0xEA};
    assert(memcmp(buffer, test_result, test_result[0]) == 0);
}

void test_x3d_proto_write_to_buffer_no_payload(void)
{
    x3d_msg_t *msg = test_x3d_proto_prepare_message_header();

    uint8_t buffer[64];
    x3d_proto_write_to_buffer(buffer, msg);
    delete_msg(msg);

    uint8_t test_result[] = {0x12, 0xFF, 0x01, 0x01, 0x2C, 0x56, 0x34, 0x12, 0x02, 0x85, 0x98, 0x00, 0x8C, 0xFE, 0xFC, 0xBB, 0x16, 0x9A};
    assert(memcmp(buffer, test_result, test_result[0]) == 0);
}

void test_x3d_proto_write_to_buffer(void)
{
    x3d_msg_t *msg = test_x3d_proto_prepare_message_header();

    msg->payload_length = 5;
    msg->payload[0]     = 1;
    msg->payload[1]     = 2;
    msg->payload[2]     = 3;
    msg->payload[3]     = 4;
    msg->payload[4]     = 5;

    uint8_t buffer[64];
    x3d_proto_write_to_buffer(buffer, msg);
    delete_msg(msg);

    uint8_t test_result[] = {0x17, 0xFF, 0x01, 0x01, 0x2C, 0x56, 0x34, 0x12, 0x02, 0x85, 0x98, 0x00, 0x8C, 0xFE, 0xFC, 0xBB, 0x01, 0x02, 0x03, 0x04, 0x05, 0x57, 0xAE};
    assert(memcmp(buffer, test_result, test_result[0]) == 0);
}

void test_x3d_proto_dec_retry(void)
{
    uint8_t buffer[] = {0x17, 0xFF, 0x01, 0x01, 0x2C, 0x56, 0x34, 0x12, 0x02, 0x85, 0x98, 0x00, 0x8C, 0xFE, 0xFC, 0xBB, 0xf9, 0x02, 0x03, 0x04, 0x05, 0x57, 0xAE};
    uint8_t res      = x3d_proto_dec_retry(buffer);
    assert(res == 1);
    assert(buffer[16] == 0xf8); // check only lower 3 bits modified
    assert(buffer[21] == 0xc0); // check valid CRC
    assert(buffer[22] == 0x84); // check valid CRC
}

int main(void)
{
    delete_msg(test_x3d_proto_init_message());

    delete_msg(test_x3d_proto_prepare_message_header_no_payload_no_magic());
    delete_msg(test_x3d_proto_prepare_message_header_no_magic());
    delete_msg(test_x3d_proto_prepare_message_header_no_payload());
    delete_msg(test_x3d_proto_prepare_message_header());

    test_x3d_proto_write_to_buffer_no_payload_no_header_payload_no_magic();
    test_x3d_proto_write_to_buffer_no_payload_no_magic();
    test_x3d_proto_write_to_buffer_no_payload_no_header_payload();
    test_x3d_proto_write_to_buffer_no_payload();
    test_x3d_proto_write_to_buffer();
    // due the fact the payload is always behind header and crc is placed correct, no additional tests needed

    test_x3d_proto_dec_retry();
    return 0;
}
