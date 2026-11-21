#include <assert.h>
#include <stdint.h>
#include <string.h>

extern int16_t calc_header_check(uint8_t *buffer, int header_len);
extern uint16_t apply_sbox(uint16_t value, uint8_t count);
extern uint16_t enc_magic_id(uint16_t magic_id, uint32_t device_id);
extern uint16_t dec_magic_id(uint16_t magic_id, uint32_t device_id);
extern void set_crc(uint8_t *buffer);

void test_calc_header_check(void)
{
    uint8_t buffer[20];
    memset(buffer, 0x12, 20);
    int16_t crc = calc_header_check(buffer, 10);
    assert(crc == (int16_t)0xff82);
}

void test_apply_sbox(void)
{
    assert(apply_sbox(0x1234, 5) == 0x1214);
}

void test_enc_magic_id(void)
{
    assert(enc_magic_id(0x1234, 0x123456) == 0xc4db);
}

void test_dec_magic_id(void)
{
    assert(dec_magic_id(0xc4db, 0x123456) == 0x1234);
}

void test_set_crc(void)
{
    uint8_t buffer[10] = {10, 1, 2, 3, 4, 5, 6, 7, 0, 0};
    set_crc(buffer);
    uint16_t crc = buffer[8] | buffer[9] << 8;
    assert(crc == 0xcfb7);
}

int main(void)
{
    test_calc_header_check();
    test_apply_sbox();
    test_enc_magic_id();
    test_dec_magic_id();
    test_set_crc();
    return 0;
}
