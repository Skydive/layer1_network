#pragma once

#include <cstdint>
#include <linux/if_tun.h>

bool reflect(uint8_t *p, size_t nbytes, const char *dev);
int tun_alloc(char* dev, int devtype = IFF_TUN);
void tun_thread();

inline int tun_fd = -1;