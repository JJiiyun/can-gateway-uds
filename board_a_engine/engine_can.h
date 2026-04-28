#ifndef ENGINE_CAN_H
#define ENGINE_CAN_H

#include <stdint.h>

#define ENGINE_CAN_ID        0x100
#define ENGINE_CAN_DLC       8

static inline void CAN_PutU16LE(uint8_t* buf, uint8_t index, uint16_t value)
{
    buf[index] = value & 0xFF;
    buf[index + 1] = (value >> 8) & 0xFF;
}

static inline uint16_t CAN_GetU16LE(uint8_t* buf, uint8_t index)
{
    return ((uint16_t)buf[index + 1] << 8) | buf[index];
}

#endif