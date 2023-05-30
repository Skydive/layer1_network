#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>

uint16_t crc16(const uint8_t* data_ptr, int length);

void printbytes(uint8_t *p, size_t nbytes);

void swap(uint8_t *p, uint8_t *q, int nbytes);

static inline void put32(uint8_t *p, uint32_t n) {
  memcpy(p,&n,sizeof(n));
}

static inline void put16(uint8_t *p, uint16_t n) {
  memcpy(p,&n,sizeof(n));
}

static inline void put8(uint8_t *p, uint8_t n) {
  *p = n;
}

static inline uint32_t get32(uint8_t *p) {
  uint32_t n;
  memcpy(&n,p,sizeof(n));
  return n;
}

static inline uint16_t get16(uint8_t *p) {
  uint16_t n;
  memcpy(&n,p,sizeof(n));
  return n;
}

static inline uint8_t get8(uint8_t *p) {
  return *p;
}
