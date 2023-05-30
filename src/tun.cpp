// TUN interface template...
// https://matthewarcus.wordpress.com/2013/05/18/fun-with-tun/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <linux/if.h>
#include <linux/if_tun.h>

#include <string>
#include "tun.h"
#include "util.h"
#include "zmq_analyse.h"

// Some handy macros to help with error checking
#define CHECKAUX(e,s)                            \
 ((e)? \
  (void)0: \
  (fprintf(stderr, "'%s' failed at %s:%d - %s\n", \
           s, __FILE__, __LINE__,strerror(errno)), \
   exit(0)))
#define CHECK(e) (CHECKAUX(e,#e))
#define CHECKSYS(e) (CHECKAUX((e)==0,#e))
#define CHECKFD(e) (CHECKAUX((e)>=0,#e))

#define STRING(e) #e

#define SRC_OFFSET4 12
#define DST_OFFSET4 16
#define SRC_OFFSET6 8
#define DST_OFFSET6 24
#define HLEN_OFFSET 0
#define PROTO_OFFSET 9
#define PROTO_ICMP 1
#define PROTO_IGMP 2
#define PROTO_TCP 6
#define PROTO_UDP 17

void describe4(uint8_t *p, size_t nbytes, const char *dev)
{
   char fromaddr[INET_ADDRSTRLEN];
   char toaddr[INET_ADDRSTRLEN];
   int headerlen = 4*(p[HLEN_OFFSET]&0x0f);
   int proto = p[PROTO_OFFSET];
   inet_ntop(AF_INET, p+SRC_OFFSET4, fromaddr, sizeof(fromaddr));
   inet_ntop(AF_INET, p+DST_OFFSET4, toaddr, sizeof(toaddr));
   uint8_t *phdr = p+headerlen;
   if (proto == PROTO_TCP) {
      // Should do this for IPv6 as well
      uint16_t srcport = ntohs(get16(phdr+0));
      uint16_t dstport = ntohs(get16(phdr+2));
      uint16_t flags = 0x0f & get8(phdr+13);
      char flagstring[16];
      snprintf(flagstring, sizeof(flagstring),
               "%s%s%s%s",
               (flags&1)?"F":"",
               (flags&2)?"S":"", 
               (flags&4)?"R":"",
               (flags&8)?"P":"");
      printf("dev=%s src=%s:%hu dst=%s:%hu len=%zu proto=%d flags=%s\n", 
             dev, fromaddr, srcport, toaddr, dstport, nbytes, proto, flagstring);
   } else if (proto == PROTO_UDP) {
      uint16_t srcport = ntohs(get16(phdr+0));
      uint16_t dstport = ntohs(get16(phdr+2));
      printf("proto=4 dev=%s src=%s:%hu dst=%s:%hu len=%zu proto=%d\n",
             dev, fromaddr, srcport, toaddr, dstport, nbytes, proto);
   } else {
      printf("proto=4 dev=%s src=%s dst=%s len=%zu proto=%d\n",
             dev, fromaddr, toaddr, nbytes, proto);
   }
}

void describe6(uint8_t *p, size_t nbytes, const char *dev)
{
  char fromaddr[INET6_ADDRSTRLEN];
  char toaddr[INET6_ADDRSTRLEN];
  inet_ntop(AF_INET6, p+SRC_OFFSET6, fromaddr, sizeof(fromaddr));
  inet_ntop(AF_INET6, p+DST_OFFSET6, toaddr, sizeof(toaddr));
  printf("proto=6 dev=%s src=%s dst=%s nbytes=%zu\n",
         dev, fromaddr, toaddr, nbytes);
  printbytes(p,40); // Just the header
}

bool doarp(uint8_t *p, size_t nbytes, const char *dev)
{
  (void)nbytes; (void)dev;
  uint16_t op = ntohs(get16(p+14+6));
  char fromaddr[INET_ADDRSTRLEN];
  char toaddr[INET_ADDRSTRLEN];
  // Skip 14 bytes of ethernet header
  inet_ntop(AF_INET, p+14+14, fromaddr, sizeof(fromaddr));
  inet_ntop(AF_INET, p+14+24, toaddr, sizeof(toaddr));
  // Assume ethernet and IPv4
  printf("proto=ARP op=%u src=%s dst=%s\n",
         op, fromaddr, toaddr);
  // Now construct the ARP response
  put16(p+14+6,htons(2)); // Operation
  uint8_t *mac = p+14+18;
  mac[0] = 0x02; mac[1] = 0x00;
  memcpy(mac+2,p+14+24,4); // Use expected IP as top 4 bytes of MAC
  memcpy(p,mac,6); // Copy to source (it will be swapped later).
  swap(p+14+8,p+14+18,10);
  return true;
}

bool reflect(uint8_t *p, size_t nbytes, const char *dev)
{
    using namespace globals;
    uint8_t version = p[0] >> 4;
    switch (version) {
    case 4:
        if (verbosity > 0) describe4(p,nbytes,dev);
        if (verbosity > 1) printbytes(p, nbytes);
        // Swap source and dest of an IPv4 packet
        // No checksum recalculation is necessary
        if (p[DST_OFFSET4] >= 224) { // BODGE: first byte of dest address - should use proper prefix
        printf("Skipping %u.0.0.0\n", p[DST_OFFSET4]);
        return false;
        } else {
        swap(p+SRC_OFFSET4,p+DST_OFFSET4,4);
        return true;
        }
    case 6:
        if (verbosity > 0) describe6(p,nbytes,dev);
        if (verbosity > 1) printbytes(p, nbytes);
        // Swap source and dest of an IPv6 packet
        // No checksum recalculation is necessary
        swap(p+SRC_OFFSET6,p+DST_OFFSET6,16);
        return true;
    default:
        printf("Unknown protocol %u: nbytes=%zu\n",
           version, nbytes);
    return false;
  }
}


int tun_alloc(char* dev, int devtype/* = IFF_TUN*/) {
    assert(dev != NULL);
    int fd = open("/dev/net/tun", O_RDWR);
    CHECKFD(fd);

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = devtype | IFF_NO_PI;
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    CHECKSYS(ioctl(fd, TUNSETIFF, (void*)&ifr));

    strncpy(dev, ifr.ifr_name, IFNAMSIZ);
    return fd;
}



void tun_thread() {
    std::string devname_str = globals::program.get<std::string>("--dev");
    if(devname_str.size() == 0) {
        fprintf(stderr, "No TUN device specified\n");
        return;
    }
    const char* devname = devname_str.c_str();
    printf("Acquiring tun: %s\n", devname);
    int devtype = IFF_TUN;
    char dev[IFNAMSIZ+1];
    memset(dev, 0, sizeof(dev));
    if(devname != NULL)strncpy(dev,devname,sizeof(dev)-1);

    tun_fd = tun_alloc(dev, devtype);
    if(tun_fd < 0) {
        fprintf(stderr, "Tunnel allocation failed...\n");
        exit(0);
    }

    printf("Configuring %s\n", devname);
    char cmd[128];
    sprintf(cmd, "ip link set %s up", devname);
    printf(">%s\n", cmd);
    int res = system(cmd);
    CHECK(res == 0);
    sprintf(cmd, "ip addr add 10.0.0.1/8 dev %s", devname);
    printf(">%s\n", cmd);
    res = system(cmd);
    CHECK(res == 0);

    uint8_t buf[2048];
    while(true) {
        ssize_t nread = read(tun_fd, buf, sizeof(buf));
        CHECK(nread >= 0);
        if(nread == 0)break;
        bool respond;
        if(devtype == IFF_TUN) {
            respond = reflect(buf, nread, dev);
        }
        if(respond) {
            ssize_t nwrite = write(tun_fd, buf, nread);
            CHECK(nwrite == nread);
        }
    }

}