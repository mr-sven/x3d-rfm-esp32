#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "../src/x3d_pped.h"
#include "helpers.h"

void test_x3d_pped_pairing_req(void)
{
    x3d_context_t ctx = {
            .device_id = 0x123456,
            .magic_id  = 65535, // test rollover
            .msg_no    = 255,   // test rollover
    };
    uint8_t buffer[64];
    x3d_pped_pairing_req(buffer, &ctx);
    assert(ctx.msg_no == 1);
    assert(ctx.magic_id == 1);

    uint8_t test_result[] = {0x17, 0xFF, 0x01, 0x00, 0x0C, 0x56, 0x34, 0x12, 0xC0, 0x85, 0x98, 0x00, 0x8C, 0xFE, 0xFB, 0xFD, 0x32, 0x01, 0xFF, 0xFF, 0xFF, 0x6E, 0x23};
    assert(memcmp(buffer, test_result, test_result[0]) == 0);
}

void test_x3d_pped_parse_pairing_req(void)
{
    uint8_t buffer[] = {0x17, 0xFF, 0x01, 0x00, 0x0C, 0x56, 0x34, 0x12, 0xC0, 0x85, 0x98, 0x00, 0x8C, 0xFE, 0xFB, 0xFD, 0x32, 0x01, 0xFF, 0xFF, 0xFF, 0x6E, 0x23};
    uint8_t retry_cnt;
    uint32_t device_id;
    uint16_t magic_id;

    // TODO: Test all error cases
    int result = x3d_pped_parse_pairing_req(buffer, &device_id, &retry_cnt, &magic_id);
    assert(result == 0);
    assert(device_id == 0x123456);
    assert(retry_cnt == 2);
    assert(magic_id == 1);
}

void test_x3d_pped_pairing_resp(void)
{
    x3d_context_t ctx = {
            .device_id = 0x654321,
            .magic_id  = 65535, // test rollover
            .msg_no    = 255,   // test rollover
    };
    uint8_t buffer[64];
    x3d_pped_pairing_resp(buffer, &ctx, 0x123456);
    assert(ctx.msg_no == 1);
    assert(ctx.magic_id == 1);
    uint8_t test_result[] = {0x17, 0xFF, 0x01, 0x00, 0x0C, 0x21, 0x43, 0x65, 0x04, 0x85, 0x98, 0x00, 0x3C, 0xB4, 0xFD, 0x26, 0x3A, 0x05, 0x56, 0x34, 0x12, 0x63, 0x05};
    assert(memcmp(buffer, test_result, test_result[0]) == 0);
}

void test_x3d_pped_parse_pairing_resp(void)
{
    x3d_context_t ctx = {
            .device_id = 0x123456,
    };

    uint8_t buffer[] = {0x17, 0xFF, 0x01, 0x00, 0x0C, 0x21, 0x43, 0x65, 0x04, 0x85, 0x98, 0x00, 0x3C, 0xB4, 0xFD, 0x26, 0x3A, 0x05, 0x56, 0x34, 0x12, 0x63, 0x05};
    uint8_t retry_cnt;
    uint32_t device_id;
    uint16_t magic_id;

    // TODO: Test all error cases
    int result = x3d_pped_parse_pairing_resp(buffer, &ctx, &device_id, &retry_cnt, &magic_id);
    assert(result == 0);
    assert(device_id == 0x654321);
    assert(retry_cnt == 2);
    assert(magic_id == 1);
}

void test_x3d_pped_register_req(void)
{
    x3d_context_t ctx = {
            .device_id = 0x123456,
            .magic_id  = 65535, // test rollover
            .msg_no    = 255,   // test rollover
    };
    x3d_pped_register_t reg_data = {
            .peer_device_id     = 0x654321,
            .reg                = 1234,
            .reg_payload_length = 2,
            .reg_payload        = (uint8_t[]){0xE1, 0x10},
    };
    uint8_t buffer[64];
    x3d_pped_register_req(buffer, &ctx, &reg_data);

    assert(ctx.msg_no == 1);
    assert(ctx.magic_id == 1);
    uint8_t test_result[] = {0x1B, 0xFF, 0x01, 0x05, 0x0C, 0x56, 0x34, 0x12, 0xC0, 0x05, 0x98, 0x00, 0x8C, 0xFE, 0xFC, 0x7D, 0x32, 0x00, 0x21, 0x43, 0x65, 0xD2, 0x04, 0xE1, 0x10, 0x2C, 0x2B};
    assert(memcmp(buffer, test_result, test_result[0]) == 0);
}

void test_x3d_pped_parse_register_req(void)
{
    x3d_context_t ctx = {
            .device_id = 0x654321,
    };
    uint8_t buffer[] = {0x1B, 0xFF, 0x01, 0x05, 0x0C, 0x56, 0x34, 0x12, 0xC0, 0x05, 0x98, 0x00, 0x8C, 0xFE, 0xFC, 0x7D, 0x32, 0x00, 0x21, 0x43, 0x65, 0xD2, 0x04, 0xE1, 0x10, 0x2C, 0x2B};

    x3d_pped_register_t reg_data;
    int result = x3d_pped_parse_register_req(buffer, &ctx, &reg_data);
    assert(result == 0);
    assert(reg_data.retry_cnt == 2);
    assert(reg_data.peer_device_id == 0x123456);
    assert(reg_data.magic_id == 1);
    assert(reg_data.reg == 1234);
    assert(reg_data.reg_payload_length == 2);
    assert(memcmp(reg_data.reg_payload, (uint8_t[]){0xE1, 0x10}, reg_data.reg_payload_length) == 0);
}

void test_x3d_pped_register_resp()
{
    x3d_context_t ctx = {
            .device_id = 0x654321,
            .magic_id  = 65535, // test rollover
            .msg_no    = 255,   // test rollover
    };
    x3d_pped_register_t reg_data = {
            .peer_device_id     = 0x123456,
            .reg                = 1234,
            .reg_payload_length = 6,
            .reg_payload        = (uint8_t[]){0xE1, 0x10, 0x01, 0x02, 0x03, 0x04},
    };
    uint8_t buffer[64];
    x3d_pped_register_resp(buffer, &ctx, &reg_data);
    // printCharInHexadecimal(buffer, buffer[0]);

    assert(ctx.msg_no == 1);
    assert(ctx.magic_id == 1);
    uint8_t test_result[] = {0x1F, 0xFF, 0x01, 0x05, 0x0C, 0x21, 0x43, 0x65, 0x04, 0x05, 0x98, 0x00, 0x3C, 0xB4, 0xFD, 0xA6, 0x3A, 0x00, 0x56, 0x34, 0x12, 0xD2, 0x84, 0xE1, 0x10, 0x01, 0x02, 0x03, 0x04, 0xD1, 0x83};
    assert(memcmp(buffer, test_result, test_result[0]) == 0);
}

void test_x3d_pped_parse_register_resp(void)
{
    x3d_context_t ctx = {
            .device_id = 0x123456,
    };
    uint8_t buffer[] = {0x1F, 0xFF, 0x01, 0x05, 0x0C, 0x21, 0x43, 0x65, 0x04, 0x05, 0x98, 0x00, 0x3C, 0xB4, 0xFD, 0xA6, 0x3A, 0x00, 0x56, 0x34, 0x12, 0xD2, 0x84, 0xE1, 0x10, 0x01, 0x02, 0x03, 0x04, 0xD1, 0x83};

    x3d_pped_register_t reg_data;
    int result = x3d_pped_parse_register_resp(buffer, &ctx, &reg_data);
    assert(result == 0);
    assert(reg_data.retry_cnt == 2);
    assert(reg_data.peer_device_id == 0x654321);
    assert(reg_data.magic_id == 1);
    assert(reg_data.reg == 1234);
    assert(reg_data.reg_payload_length == 6);
    assert(memcmp(reg_data.reg_payload, (uint8_t[]){0xE1, 0x10, 0x01, 0x02, 0x03, 0x04}, reg_data.reg_payload_length) == 0);
}

int main(void)
{
    test_x3d_pped_pairing_req();
    test_x3d_pped_parse_pairing_req();
    test_x3d_pped_pairing_resp();
    test_x3d_pped_parse_pairing_resp();
    test_x3d_pped_register_req();
    test_x3d_pped_parse_register_req();
    test_x3d_pped_register_resp();
    test_x3d_pped_parse_register_resp();
    return 0;
}
