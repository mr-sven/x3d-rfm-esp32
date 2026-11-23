#include <assert.h>
#include <stdint.h>
#include "helpers.h"
#include "../src/x3d_proto.h"

/**
 * Packet checksum is tested by receiver, so it will not be part of the test
 */

void test_x3d_proto_parse_verify_msg_to_small(void)
{
    uint8_t buffer[] = {0x0d, 0xFF, 0x01, 0x01, 0x28, 0x56, 0x34, 0x12, 0x02, 0x85, 0xFE, 0xDD, 0x7D, 0xD3};
    x3d_msg_t *msg   = x3d_proto_parse_verify_msg(buffer);

    assert(msg == NULL);
}

void test_x3d_proto_parse_verify_msg_to_large(void)
{
    uint8_t buffer[] = {0x41, 0xFF, 0x01, 0x01, 0x28, 0x56, 0x34, 0x12, 0x02, 0x85, 0xFE, 0xDD, 0x7D, 0xD3};
    x3d_msg_t *msg   = x3d_proto_parse_verify_msg(buffer);

    assert(msg == NULL);
}

void test_x3d_proto_parse_verify_msg_invalid_header_cksum(void)
{
    uint8_t buffer[] = {0x0E, 0xFF, 0x01, 0x01, 0x28, 0x56, 0x34, 0x12, 0x02, 0x85, 0xFE, 0xFF, 0x7D, 0xD3};
    x3d_msg_t *msg   = x3d_proto_parse_verify_msg(buffer);

    assert(msg == NULL);
}

void test_x3d_proto_parse_verify_msg_no_payload_no_header_payload_no_magic(void)
{
    uint8_t buffer[] = {0x0E, 0xFF, 0x01, 0x01, 0x28, 0x56, 0x34, 0x12, 0x02, 0x85, 0xFE, 0xDD, 0x7D, 0xD3};
    x3d_msg_t *msg   = x3d_proto_parse_verify_msg(buffer);

    assert(msg != NULL);
    assert(msg->msg_header.pkt_len == 14);
    assert(msg->msg_header.pkt_addr == 0xff);
    assert(msg->msg_header.msg_no == 1);
    assert(msg->msg_header.msg_type == 1);
    assert(msg->msg_header.head_flags == 1);
    assert(msg->msg_header.head_len == 8);
    assert(msg->msg_header.device_id == 0x123456);
    assert(msg->msg_header.net == 2);
    assert(msg->header_payload_length == 1);
    assert(msg->header_payload[0] == 0x85);
    assert(msg->header_magic == 0);
    assert(msg->payload_length == 0);

    delete_msg(msg);
}

void test_x3d_proto_parse_verify_msg_no_payload_no_magic(void)
{
    uint8_t buffer[] = {0x10, 0xFF, 0x01, 0x01, 0x2A, 0x56, 0x34, 0x12, 0x02, 0x85, 0x98, 0x00, 0xFE, 0x45, 0x0C, 0x4B};
    x3d_msg_t *msg   = x3d_proto_parse_verify_msg(buffer);

    assert(msg != NULL);
    assert(msg->msg_header.pkt_len == 16);
    assert(msg->msg_header.pkt_addr == 0xff);
    assert(msg->msg_header.msg_no == 1);
    assert(msg->msg_header.msg_type == 1);
    assert(msg->msg_header.head_flags == 1);
    assert(msg->msg_header.head_len == 10);
    assert(msg->msg_header.device_id == 0x123456);
    assert(msg->msg_header.net == 2);
    assert(msg->header_payload_length == 3);
    assert(msg->header_payload[0] == 0x85);
    assert(msg->header_payload[1] == 0x98);
    assert(msg->header_payload[2] == 0x00);
    assert(msg->header_magic == 26590); // it could be a magic number
    assert(msg->payload_length == 0);

    delete_msg(msg);
}

void test_x3d_proto_parse_verify_msg_no_payload_no_header_payload(void)
{
    uint8_t buffer[] = {0x10, 0xFF, 0x01, 0x01, 0x2A, 0x56, 0x34, 0x12, 0x02, 0x85, 0x8C, 0xFE, 0xFD, 0x53, 0x02, 0xEA};
    x3d_msg_t *msg   = x3d_proto_parse_verify_msg(buffer);

    assert(msg != NULL);
    assert(msg->msg_header.pkt_len == 16);
    assert(msg->msg_header.pkt_addr == 0xff);
    assert(msg->msg_header.msg_no == 1);
    assert(msg->msg_header.msg_type == 1);
    assert(msg->msg_header.head_flags == 1);
    assert(msg->msg_header.head_len == 10);
    assert(msg->msg_header.device_id == 0x123456);
    assert(msg->msg_header.net == 2);
    assert(msg->header_payload_length == 3);
    assert(msg->header_payload[0] == 0x85);
    assert(msg->header_payload[1] == 0x8C);
    assert(msg->header_payload[2] == 0xFE);
    assert(msg->header_magic == 1);
    assert(msg->payload_length == 0);

    delete_msg(msg);
}

void test_x3d_proto_parse_verify_msg_no_payload(void)
{
    uint8_t buffer[] = {0x12, 0xFF, 0x01, 0x01, 0x2C, 0x56, 0x34, 0x12, 0x02, 0x85, 0x98, 0x00, 0x8C, 0xFE, 0xFC, 0xBB, 0x16, 0x9A};
    x3d_msg_t *msg   = x3d_proto_parse_verify_msg(buffer);

    assert(msg != NULL);
    assert(msg->msg_header.pkt_len == 18);
    assert(msg->msg_header.pkt_addr == 0xff);
    assert(msg->msg_header.msg_no == 1);
    assert(msg->msg_header.msg_type == 1);
    assert(msg->msg_header.head_flags == 1);
    assert(msg->msg_header.head_len == 12);
    assert(msg->msg_header.device_id == 0x123456);
    assert(msg->msg_header.net == 2);
    assert(msg->header_payload_length == 5);
    assert(msg->header_payload[0] == 0x85);
    assert(msg->header_payload[1] == 0x98);
    assert(msg->header_payload[2] == 0x00);
    assert(msg->header_payload[3] == 0x8C);
    assert(msg->header_payload[4] == 0xFE);
    assert(msg->header_magic == 1);
    assert(msg->payload_length == 0);

    delete_msg(msg);
}

void test_x3d_proto_parse_verify_msg(void)
{
    uint8_t buffer[] = {0x17, 0xFF, 0x01, 0x01, 0x2C, 0x56, 0x34, 0x12, 0x02, 0x85, 0x98, 0x00, 0x8C, 0xFE, 0xFC, 0xBB, 0x01, 0x02, 0x03, 0x04, 0x05, 0x57, 0xAE};
    x3d_msg_t *msg   = x3d_proto_parse_verify_msg(buffer);

    assert(msg != NULL);
    assert(msg->msg_header.pkt_len == 23);
    assert(msg->msg_header.pkt_addr == 0xff);
    assert(msg->msg_header.msg_no == 1);
    assert(msg->msg_header.msg_type == 1);
    assert(msg->msg_header.head_flags == 1);
    assert(msg->msg_header.head_len == 12);
    assert(msg->msg_header.device_id == 0x123456);
    assert(msg->msg_header.net == 2);
    assert(msg->header_payload_length == 5);
    assert(msg->header_payload[0] == 0x85);
    assert(msg->header_payload[1] == 0x98);
    assert(msg->header_payload[2] == 0x00);
    assert(msg->header_payload[3] == 0x8C);
    assert(msg->header_payload[4] == 0xFE);
    assert(msg->header_magic == 1);
    assert(msg->payload_length == 5);
    assert(msg->payload[0] == 1);
    assert(msg->payload[1] == 2);
    assert(msg->payload[2] == 3);
    assert(msg->payload[3] == 4);
    assert(msg->payload[4] == 5);

    delete_msg(msg);
}

int main(void)
{
    test_x3d_proto_parse_verify_msg_to_small();
    test_x3d_proto_parse_verify_msg_to_large();
    test_x3d_proto_parse_verify_msg_invalid_header_cksum();
    test_x3d_proto_parse_verify_msg_no_payload_no_header_payload_no_magic();
    test_x3d_proto_parse_verify_msg_no_payload_no_magic();
    test_x3d_proto_parse_verify_msg_no_payload_no_header_payload();
    test_x3d_proto_parse_verify_msg_no_payload();
    test_x3d_proto_parse_verify_msg();
    return 0;
}
