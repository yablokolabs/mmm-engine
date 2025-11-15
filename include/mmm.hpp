#pragma once

#include <vector>

#include "channels.hpp"

struct MMM {
    std::vector<double> y;
    std::vector<Channel> channels;
    double t0 = 0, t1 = 0, t2 = 0;
    std::vector<double> a, b;
    std::vector<double> predict() const;
};
