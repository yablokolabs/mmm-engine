#pragma once

#include <vector>

struct ChannelParams {
    double beta;
    double alpha;
    double p;
    double decay;
};

struct Channel {
    ChannelParams params;
    std::vector<double> spend;
    std::vector<double> adstocked;
    std::vector<double> saturated;
    std::vector<double> contrib;
};

void compute_channel(Channel& ch);
