
#include <iostream>

#include "util.h"
#include "decode.h"

#include "zmq_analyse.h"

void decode_bytes(const vector<uint8_t>& decoded_bytes) {
    int mlen1 = decoded_bytes[1] + (decoded_bytes[0] << 8);
    int mlen2 = decoded_bytes[3] + (decoded_bytes[2] << 8);
    if(mlen1 != mlen2) {
        if(globals::verbosity > 0)cerr << "Frame dropped: invalid length in header" << endl;
        return;
    }
    int end_idx = 4+mlen1;
    uint16_t payload_crc = decoded_bytes[end_idx+1] + (decoded_bytes[end_idx] << 8);
    // TODO: Make Crc16 consistent
    uint16_t calc_crc = crc16(&decoded_bytes[4], mlen1);
    if(payload_crc != calc_crc) {
        if(globals::verbosity > 0)cerr << "Frame dropped: invalid CRC check" << endl;
        return;
    }
    // cout << hex << crc << endl;
    // cout << hex << payload_crc << endl;
    vector<uint8_t> packet(&decoded_bytes[4], &decoded_bytes[4]+mlen1);
    process_packet(packet);
}