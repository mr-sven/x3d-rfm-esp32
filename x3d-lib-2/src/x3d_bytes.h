/**
 * @file x3d_bytes.h
 * @author Sven Fabricius (sven.fabricius@livediesel.de)
 * @brief
 * @version 1.0
 * @date 2026-11-19
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include <stdint.h>

static inline int x3d_read_le_u32(uint32_t *res, uint8_t *buffer, int pBuffer)
{
    *res = buffer[pBuffer++];
    *res |= buffer[pBuffer++] << 8;
    *res |= buffer[pBuffer++] << 16;
    *res |= buffer[pBuffer++] << 24;
    return pBuffer;
}

static inline int x3d_read_le_u16(uint16_t *res, uint8_t *buffer, int pBuffer)
{
    *res = buffer[pBuffer++];
    *res |= buffer[pBuffer++] << 8;
    return pBuffer;
}

static inline int x3d_read_be_i16(int16_t *res, uint8_t *buffer, int pBuffer)
{
    *res = buffer[pBuffer++] << 8;
    *res |= buffer[pBuffer++];
    return pBuffer;
}

static inline int x3d_read_le_u24(uint32_t *res, uint8_t *buffer, int pBuffer)
{
    *res = buffer[pBuffer++];
    *res |= buffer[pBuffer++] << 8;
    *res |= buffer[pBuffer++] << 16;
    return pBuffer;
}

static inline int x3d_write_le_u16(uint16_t val, uint8_t *buffer, int pBuffer)
{
    buffer[pBuffer++] = val & 0xff;
    buffer[pBuffer++] = (val >> 8) & 0xff;
    return pBuffer;
}

static inline int x3d_write_be_i16(int16_t val, uint8_t *buffer, int pBuffer)
{
    buffer[pBuffer++] = (val >> 8) & 0xff;
    buffer[pBuffer++] = val & 0xff;
    return pBuffer;
}

static inline int x3d_write_be_u16(uint16_t val, uint8_t *buffer, int pBuffer)
{
    buffer[pBuffer++] = (val >> 8) & 0xff;
    buffer[pBuffer++] = val & 0xff;
    return pBuffer;
}

static inline int x3d_write_le_u24(uint32_t val, uint8_t *buffer, int pBuffer)
{
    buffer[pBuffer++] = val & 0xff;
    buffer[pBuffer++] = (val >> 8) & 0xff;
    buffer[pBuffer++] = (val >> 16) & 0xff;
    return pBuffer;
}

static inline int x3d_get_highest_bit(uint16_t value)
{
    return sizeof(unsigned int) * 8 - __builtin_clz(value) - 1;
}
