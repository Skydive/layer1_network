#pragma once
#include <cstdint> // uint8_t
#include <vector>

using namespace std;

void decode_bytes(const vector<uint8_t>& decoded_bytes);
void process_packet(const vector<uint8_t>& packet);
