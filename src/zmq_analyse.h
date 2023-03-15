#pragma once

#include <string>
#include <argparse/argparse.hpp>

constexpr int BUFFER_SIZE = 10 * 768 * 1000;
constexpr int OVERSAMPLE = 4;
constexpr int CAPTURE_BUFFER_SIZE = 0.02 * 768 * 1000 / OVERSAMPLE;

// using namespace std;

// argparse::ArgumentParser parse_args(int argc, char** argv) {
//     argparse::ArgumentParser program("zmq_analyse");
//     program.add_argument("--connect")
//         .default_value("tcp://127.0.0.1:49202")
//         .help("Connect to a ZMQ socket to receive data from");
//     program.add_argument("--verbose")
//         .default_value(false)
//         .help("Add verbosity");
//     return program;
// }