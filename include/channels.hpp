#pragma once

#include <string>
#include <vector>

struct ChannelParams {
  double beta;
  double alpha;
  double p;
  double decay;
};

struct Channel {
  std::string name;
  ChannelParams params;
  std::vector<double> spend;
  std::vector<double> adstocked;
  std::vector<double> saturated;
  std::vector<double> contrib;
};

void compute_channel(Channel &ch);
