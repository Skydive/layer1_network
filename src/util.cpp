#include "util.h"

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

void printbytes(uint8_t *p, size_t nbytes) {
  for (size_t i = 0; i < nbytes; i++) {
    printf("%02x%s",
	   p[i],
	   ((i+1)%16 == 0 || i+1 == nbytes)?"\n":" ");
  }
}

void swap(uint8_t *p, uint8_t *q, int nbytes) {
  for (int i = 0; i < nbytes; i++) {
    uint8_t t = *p; *p = *q; *q = t;
    p++; q++;
  }
}