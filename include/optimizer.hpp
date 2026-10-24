#pragma once

#include <vector>

#include "channels.hpp"

// Weekly sales a channel drives at a constant weekly spend, once adstock has
// reached its steady state (adstock level = spend / (1 - decay)).
double steady_response(const ChannelParams &p, double weekly_spend);

// Extra weekly sales per extra unit of weekly spend at `weekly_spend`.
double marginal_return(const ChannelParams &p, double weekly_spend);

struct Allocation {
  std::vector<double> current;      // weekly spend per channel today
  std::vector<double> optimal;      // weekly spend per channel after optimizing
  std::vector<double> lower, upper; // bounds the optimizer respected
  std::vector<double> mroi_current, mroi_optimal;
  double current_response = 0; // weekly media-driven sales at `current`
  double optimal_response = 0; // weekly media-driven sales at `optimal`
};

// Maximizes total steady-state response for a fixed weekly budget. Each
// channel stays within [(1 - bound) * current, (1 + bound) * current] so the
// plan never leaves the spend range the model was fitted on. Greedy
// marginal-return allocation followed by pairwise exchange refinement, which
// also handles S-shaped (slope > 1) response curves. Deterministic.
Allocation optimize_budget(const std::vector<ChannelParams> &params,
                           const std::vector<double> &current, double budget,
                           double bound);
