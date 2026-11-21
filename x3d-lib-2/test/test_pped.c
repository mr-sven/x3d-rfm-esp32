#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "../src/x3d_pped.h"
#include "helpers.h"

void test_x3d_pped_pairing_req(void)
{
    uint8_t msg_no    = 255;   // test rollover
    uint16_t magic_id = 65535; // test rollover
    uint8_t buffer[64];
    x3d_pped_pairing_req(buffer, 0x123456, &msg_no, &magic_id);
    assert(msg_no == 1);
    assert(magic_id == 1);

    uint8_t test_result[] = {0x17, 0xFF, 0x01, 0x00, 0x0C, 0x56, 0x34, 0x12, 0xC0, 0x85, 0x98, 0x00, 0x8C, 0xFE, 0xFB, 0xFD, 0x32, 0x01, 0xFF, 0xFF, 0xFF, 0x6E, 0x23};
    assert(memcmp(buffer, test_result, test_result[0]) == 0);
}

void test_x3d_pped_pairing_resp(void)
{
    uint8_t msg_no    = 255;   // test rollover
    uint16_t magic_id = 65535; // test rollover
    uint8_t buffer[64];
    x3d_pped_pairing_resp(buffer, 0x123456, &msg_no, &magic_id, 0x654321);
    assert(msg_no == 1);
    assert(magic_id == 1);

    uint8_t test_result[] = {0x17, 0xFF, 0x01, 0x00, 0x0C, 0x56, 0x34, 0x12, 0x04, 0x85, 0x98, 0x00, 0x8C, 0xFE, 0xFC, 0xB9, 0x3A, 0x05, 0x21, 0x43, 0x65, 0x3E, 0xD1};
    assert(memcmp(buffer, test_result, test_result[0]) == 0);
}

void test_x3d_pped_is_valid_pairing_req(void)
{
    uint8_t buffer[] = {0x17, 0xFF, 0x01, 0x00, 0x0C, 0x56, 0x34, 0x12, 0xC0, 0x85, 0x98, 0x00, 0x8C, 0xFE, 0xFB, 0xFD, 0x32, 0x01, 0xFF, 0xFF, 0xFF, 0x6E, 0x23};
    uint8_t retry_cnt;
    uint32_t device_id;

    // TODO: Test all error cases
    int result = x3d_pped_is_valid_pairing_req(buffer, &device_id, &retry_cnt);
    assert(result == 0);
    assert(device_id == 0x123456);
    assert(retry_cnt == 2);
}

int main(void)
{
    test_x3d_pped_pairing_req();
    test_x3d_pped_pairing_resp();
    test_x3d_pped_is_valid_pairing_req();
    return 0;
}
