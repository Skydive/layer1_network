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
#include <Iir.h>

#include <cstdint> // uint8_t

#include "zmq_analyse.h"
#include "demod.h"
#include "tun.h"

using namespace std;

static array<complex<float>, BUFFER_SIZE> buffer{};
static atomic<int> buffer_write_idx = 0;
static atomic<int> buffer_read_idx = 0;


argparse::ArgumentParser parse_args(int argc, char** argv) {
    using namespace globals;
    program = argparse::ArgumentParser("zmq_analyse", "0.1", argparse::default_arguments::none);

    program.add_description("");
    program.add_epilog("");

    program.add_argument("-c", "--connect")
        .help("Connect to a ZMQ socket to receive data from")
        .default_value("")
        .metavar("ZMQ_PUB_SOCKET");

    program.add_argument("-v", "--verbose")
        .help("Add verbosity to error messages")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("-r", "--rthreshold")
        .help("Threshold factor rise for signal peak detection")
        .default_value(0.3f)
        .nargs(1)
        .scan<'g', float>()
        .metavar("RISE_FACTOR");
    program.add_argument("-d", "--dev")
        .help("Tunnel network device")
        .default_value(""s)
        .nargs(1)
        .metavar("IF_NAME");


    program.add_argument("-h", "--help")
        .action([&](const std::string& s) {
            cout << program.help().str() << endl;
            exit(0);
        })
        .default_value(false)
        .help("Shows help message")
        .implicit_value(true)
        .nargs(0);
    
    try {
        program.parse_args(argc, argv);
        verbosity = program.get<bool>("--verbose") ? 2 : 0;
    } catch (const runtime_error& err) {
        cerr << err.what() << endl;
        cerr << program;
        exit(1);
    }
    return program;
}


void receive_samples(zmq::context_t *ctx) {
    zmq::socket_t sock(*ctx, zmq::socket_type::sub);
    cout << "Connecting to ZMQ socket..." << endl;
    string zmq_pub_source = globals::program.get<string>("--connect");
    try {
        sock.connect(zmq_pub_source);
    } catch (zmq::error_t& e) {
        cout << "Error: " << e.what() << endl;
        return;
    }
    cout << "Connected to ZMQ socket: " << zmq_pub_source << endl;

    sock.set(zmq::sockopt::subscribe, "");

    zmq::message_t rmsg;

    // Populate BUFFER vector in circular fashion
    while (1) {
        auto res = sock.recv(rmsg, zmq::recv_flags::none);
        // cout << "R" << flush;

        const complex<float>* data = rmsg.data<complex<float>>();
        size_t sz = rmsg.size() / sizeof(complex<float>);

        size_t space_left = BUFFER_SIZE-buffer_write_idx;
        if(sz > space_left) { // Wrap around
            for(size_t i=0; i<space_left; i++)
                buffer[buffer_write_idx+i] = data[i];

            // WRAPPED! -- Do something here
            cout << "WRAPPED" << endl;
            buffer_write_idx = BUFFER_SIZE;
            // Sync the threads! (???)

            for(size_t i=space_left; i<sz; i++) {
                buffer[i-space_left] = data[i];
            }
            buffer_write_idx = sz - space_left;
            buffer_read_idx = 0;           
        } else {
            for(size_t i=0; i<sz; i++)
                buffer[buffer_write_idx+i] = data[i];
            buffer_write_idx += sz;
            
        }
    }
}

// Process samples in our buffer vector
void process_samples() {
    int cnt = 0;
    constexpr int order = 10;
    // static Iir::Butterworth::LowPass<order> f_re, f_im;
    const float samplingrate = 768e6; // 768 MHz
    const float cutoff_frequency = 100e3; // 25 kHz
    // f_re.setup(samplingrate, cutoff_frequency);
    // f_im.setup(samplingrate, cutoff_frequency);

    while(1) {
        if(buffer_read_idx < buffer_write_idx) {
            buffer_read_idx++;
            complex<float> data = buffer[buffer_read_idx];

            // Low pass IIR filter
            // float re = f_re.filter(real(data));
            // float im = f_im.filter(imag(data));
            // data = complex<float>{re, im};

            // Naive decimation is easy...
            // Perhaps we don't need to downsample?
            // 768k / 48k = 16 @ 2 SPS
            // 768k / 192k = 4 @ 8 SPS

            // Downsample
            if(++cnt == OVERSAMPLE) {
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

int main(int argc, char** argv) {
    parse_args(argc, argv);

    zmq::context_t ctx(1);

    if(globals::program.get<string>("--connect").size() > 0) {
        auto thread1 = async(launch::async, receive_samples, &ctx);
        auto thread2 = async(launch::async, process_samples);
    }
    
    auto t = async(launch::async, tun_thread);
    t.wait();
}