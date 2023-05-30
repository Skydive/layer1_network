#pragma once

#include <argparse/argparse.hpp>

constexpr int BUFFER_SIZE = 10 * 768 * 1000;
constexpr int OVERSAMPLE = 4;
constexpr int CAPTURE_BUFFER_SIZE = 0.1 * 768 * 1000 / OVERSAMPLE;

namespace globals {
    inline argparse::ArgumentParser program;
    inline int verbosity = 0;
}