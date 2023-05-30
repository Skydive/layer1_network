#include <deque>
#include <complex>
#include <vector>
#include <cstdint> // uint8_t

#include <algorithm> // search

#include <iostream>

#include "decode.h"
#include "demod.h"
#include "zmq_analyse.h"

template<typename T>
vector<complex<T>> mm_sync(const vector<complex<T>>& samples, int sps) {
    float mu = 0;
    auto out = vector<complex<T>>(samples.size() + 10);
    auto out_rail = vector<complex<float>>(samples.size() + 10);
    size_t i_in = 0, i_out = 2;
    while(i_out < samples.size() && i_in+16 < samples.size()) {
        out[i_out] = samples[i_in + int(mu)];
        out_rail[i_out] = complex<float>{(float)int(real(out[i_out]) > 0), (float)int(imag(out[i_out]) > 0)};
        auto x = (out_rail[i_out] - out_rail[i_out-2]) * conj(out[i_out-1]);
        auto y = (out[i_out] - out[i_out-2]) * conj(out_rail[i_out-1]);
        auto mm_val = real(y - x);
        mu += sps + 0.3 * mm_val;
        i_in += int(floor(mu));
        mu = mu - floor(mu);
        i_out += 1;
    }
    auto result = vector<complex<T>>();
    copy(out.begin() + 2, out.begin() + i_out, back_inserter(result));
    return result;
}

void demod(const complex<float>& data) {
    static int demod_cnt = 0;
    demod_cnt++;

    static deque<float> prev_power_floor(8);
    static vector<complex<float>> power_floor_hist(8, 0);
    static int power_floor_idx = 0;
    // Calculate current power floor
    float power_floor_avg = 0;
    for(size_t i=0; i<power_floor_hist.size(); i++)
        power_floor_avg += abs(power_floor_hist[i]);
    power_floor_avg /= power_floor_hist.size();

    prev_power_floor.pop_front();
    prev_power_floor.push_back(power_floor_avg);
    float last_power_floor_avg = prev_power_floor.front();

    // Add to power floor
    power_floor_hist[power_floor_idx++] = data;
    power_floor_idx = power_floor_idx % power_floor_hist.size();

    static bool trigger_state = false;
    static vector<complex<float>> captured_samples(CAPTURE_BUFFER_SIZE);
    static size_t captured_samples_read_idx = 0;

    float trigger_rise_factor = globals::program.get<float>("--rthreshold");
    // cout << last_power_floor_avg << "\t" << power_floor_avg << endl;
    if(!trigger_state && power_floor_avg - last_power_floor_avg > trigger_rise_factor*last_power_floor_avg) {
        trigger_state = true;
        cout << "TRIGGERED" << endl;
        captured_samples_read_idx = 0;
    }


    if(trigger_state) {
        captured_samples[captured_samples_read_idx++] = data;
    }

    if(captured_samples_read_idx >= captured_samples.size()) {
        trigger_state = false;
        captured_samples_read_idx = 0;
        // vector<complex<float>> samples_sync = captured_samples;
        vector<complex<float>> samples_sync = mm_sync(captured_samples, 8);
        vector<float> demod_samples(samples_sync.size());

        // ofstream fout("./zmq_analyse.cf32", ios::binary);
        // fout.write((char*)&samples_sync[0], samples_sync.size() * sizeof(decltype(samples_sync)::value_type));
        // fout.close();

        vector<uint8_t> bits(samples_sync.size()-1);
        for(size_t i=0; i<samples_sync.size(); i++) {
            demod_samples[i] = arg(samples_sync[i]);
        }
        for(size_t i=0; i<bits.size(); i++) {
            float dphi = arg(samples_sync[i+1]) - arg(samples_sync[i]);
            if(dphi < 0) {
                dphi += 2.0f * M_PI;
            } else if(dphi > 2.0f * M_PI) {
                dphi -= 2.0f * M_PI;
            }
            dphi /= M_PI;
            int bit = (int)roundf(dphi) % 2;
            bits[i] = bit;
        }

        const static vector<uint8_t> SYNC = {1,1,1,0,0,0,0,1,0,1,0,1,1,0,1,0,1,1,1,0,1,0,0,0,1,0,0,1,0,0,1,1}; 
        auto it = std::search(bits.begin(), bits.end(), SYNC.begin(), SYNC.end());
        if(it == bits.end()) {
            // Verbose
            if(globals::verbosity > 0) {
                cerr << "Frame dropped: No preamble detected!" << endl;
            }
            return;
        }

        // 

        int frame_idx = std::distance(bits.begin(), it); 
        int end_frame_idx = frame_idx + SYNC.size();

        // cout << demod_cnt << ": frame_idx: " << frame_idx << " SYNC_SIZE: " << SYNC.size() << endl;

        vector<uint8_t> decoded_bytes;

        // cout << "BYTES: ";
        for(size_t i=end_frame_idx; i<bits.size(); i += 8) {
            uint8_t value = 0;
            for(auto j=i; j<i+8; j++) {
                value |= (bits[j] << (7 - (j-i)));
            }
            decoded_bytes.push_back(value);
        }
        // cout << endl;

        // cout << demod_cnt << ": PRE-DECODE: " << decoded_bytes.size() << "BITS-N: " << bits.size() << endl;
        decode_bytes(decoded_bytes);

        // TODO: Benchmark 
        // TODO: Extend to d8psk!
        // TODO: Obtain IP packet and send into TUN device.
    }

    // TRIGGER if power floor rises substantially
    // UNTRIGGER when power floor drops

    // Look at how dumpvdl2 works(???)
    // Code Costas Loop (Frequency Correction!)
}