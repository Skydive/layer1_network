#pragma once

#include <cstdint>

uint16_t crc16(const uint8_t* data_ptr, int length) {
    uint8_t i;
    uint16_t crc = 0xffff;
    while (length--) {
        crc ^= *(uint8_t*)data_ptr++ << 8;
        for (i=0; i < 8; i++)
            crc = crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1;
    }
    return crc & 0xffff;
}