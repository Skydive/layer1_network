#include <iostream>
#include <string>

#include <future>
#include <thread>
#include <atomic>

#include <array>
#include <vector>

#include <complex>

#include <zmq.hpp>
#include <zmq_addon.hpp>

#include <cstdint> // uint8_t

#include "zmq_analyse.h"
#include "demod.h"

using namespace std;
static array<complex<float>, BUFFER_SIZE> buffer{};
static atomic<int> buffer_write_idx = 0;
static atomic<int> buffer_read_idx = 0;

void RecieverThread(zmq::context_t *ctx) {
    zmq::socket_t sock(*ctx, zmq::socket_type::sub);
    cout << "Connecting..." << endl;
    try {
        sock.connect("tcp://127.0.0.1:49202");
    } catch (zmq::error_t& e) {
        cout << "Error: " << e.what() << endl;
        return;
    }
    cout << "Connected!" << endl;

    sock.set(zmq::sockopt::subscribe, "");

    zmq::message_t rmsg;

    // Populate BUFFER vector in circular fashion
    while (1) {
        auto res = sock.recv(rmsg, zmq::recv_flags::none);
        cout << "R" << flush;

        const complex<float>* data = rmsg.data<complex<float>>();
        size_t sz = rmsg.size() / sizeof(complex<float>);

        size_t space_left = BUFFER_SIZE-buffer_write_idx;
        if(sz > space_left) { // Wrap around
            for(int i=0; i<space_left; i++)
                buffer[buffer_write_idx+i] = data[i];

            // WRAPPED! -- Do something here
            cout << "WRAPPED" << endl;
            buffer_write_idx = BUFFER_SIZE;
            // Sync the threads! (???)

            for(int i=space_left; i<sz; i++) {
                buffer[i-space_left] = data[i];
            }
            buffer_write_idx = sz - space_left;
            buffer_read_idx = 0;           
        } else {
            for(int i=0; i<sz; i++)
                buffer[buffer_write_idx+i] = data[i];
            buffer_write_idx += sz;
            
        }
        // copy(data, data+sz, std::begin(buffer)+buffer_write_idx);
        // cout << "Written: " << buffer_write_idx << "/" << BUFFER_SIZE << endl;
    }
}

// Process samples in our buffer vector
void process_samples() {
    int cnt = 0;
    while(1) {
        if(buffer_read_idx < buffer_write_idx) {
            // Apply IIR filter to data...
            buffer_read_idx++;
            auto data = buffer[buffer_read_idx];
            // Naive decimation is easy...
            // Perhaps we don't need to downsample?
            // 768k / 48k = 16 @ 2 SPS
            // 768k / 192k = 4 @ 8 SPS

            // Downsample
            if(++cnt == OVERSAMPLE) {
                // How?

                // 
                // Demodulate!
                demod(data);
                cnt = 0;
            }

            // Demodulate...
        }
    }
}

void process_packet(const vector<uint8_t>& packet) {
    static int count = 0;
    cout << "Packet " << setw(4) << setfill('0') << count++ << ": ";
    for(uint8_t packet_byte : packet)
        cout << hex << setw(2) << setfill('0') << (int)packet_byte << " ";
    cout << endl;
}

int main() {
    /*
     * No I/O threads are involved in passing messages using the inproc transport.
     * Therefore, if you are using a ØMQ context for in-process messaging only you
     * can initialise the context with zero I/O threads.
     *
     * Source: http://api.zeromq.org/4-3:zmq-inproc
     */
    zmq::context_t ctx(1);

    auto thread1 = async(launch::async, RecieverThread, &ctx);
    auto thread2 = async(launch::async, process_samples);

    // Give the publisher a chance to bind, since inproc requires it
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    thread1.wait();
    thread2.wait();

    /*
     * Output:
     *   An infinite loop of a mix of:
     *     Thread2: [A] Message in A envelope
     *     Thread2: [B] Message in B envelope
     *     Thread3: [A] Message in A envelope
     *     Thread3: [B] Message in B envelope
     *     Thread3: [C] Message in C envelope
     */
}