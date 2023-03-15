# Some constants used to ioctl the device file. I got them by a simple C
# program.
import os
import fcntl
import struct

TUNSETIFF = 0x400454ca
TUNSETOWNER = TUNSETIFF + 2
IFF_TUN = 0x0001
IFF_TAP = 0x0002
IFF_NO_PI = 0x1000

# Open TUN device file.
tun = os.open('/dev/net/tun', os.O_RDWR)

# Tall it we want a TUN device named tun0.
ifr = struct.pack('16sH', b'tun0', IFF_TUN | IFF_NO_PI)

fcntl.ioctl(tun, TUNSETIFF, ifr)
# Optionally, we want it be accessed by the normal user.
fcntl.ioctl(tun, TUNSETOWNER, 1000)

while True:
    # Read an IP packet been sent to this TUN device.
    packet = list(os.read(tun, 2048))
    # print(packet, type(packet), type(packet[0]))
    # Modify it to an ICMP Echo Reply packet.
    
    # Note that I have not checked content of the packet, but treat all packets
    # been sent to our TUN device as an ICMP Echo Request.

    # Swap source and destination address.
    packet[12:16], packet[16:20] = packet[16:20], packet[12:16]

    # # Under Linux, the code below is not necessary to make the TUN device to
    # # work. I don't know why yet, but if you run tcpdump, you can see the
    # # difference.
    # if True:
    #     # Change ICMP type code to Echo Reply (0).
    #     packet[20] = 0
    #     # Clear original ICMP Checksum field.
    #     packet[22:24] = 0, 0
    #     # Calculate new checksum.
    #     checksum = 0
    #     # for every 16-bit of the ICMP payload:
    #     for i in range(20, len(packet), 2):
    #         print(type(packet), type(packet[i]))
    #         half_word = ((packet[i]) << 8) + (packet[i+1])
    #         checksum += half_word
    #     # Get one's complement of the checksum.
    #     checksum = ~(checksum + 4) & 0xffff
    #     # Put the new checksum back into the packet.
    #     packet[22] = (checksum >> 8)
    #     packet[23] = (checksum & ((1 << 8) -1))

    # Write the reply packet into TUN device.
    os.write(tun, bytes(packet))