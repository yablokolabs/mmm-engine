#include "optimizer.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <string>

double steady_response(const ChannelParams &p, double weekly_spend) {
  if (weekly_spend <= 0)
    return 0.0;
  const double level = weekly_spend / (1.0 - p.decay);
  const double xp = std::pow(level, p.p);
  return p.beta * xp / (std::pow(p.alpha, p.p) + xp + 1e-9);
}

double marginal_return(const ChannelParams &p, double weekly_spend) {
  const double h = std::max(1e-6, std::abs(weekly_spend) * 1e-4);
  const double lo = std::max(0.0, weekly_spend - h);
  return (steady_response(p, weekly_spend + h) - steady_response(p, lo)) /
         (weekly_spend + h - lo);
}

static double total_response(const std::vector<ChannelParams> &params,
                             const std::vector<double> &x) {
  double s = 0;
  for (size_t i = 0; i < params.size(); ++i)
    s += steady_response(params[i], x[i]);
  return s;
}

Allocation optimize_budget(const std::vector<ChannelParams> &params,
                           const std::vector<double> &current, double budget,
                           double bound) {
  const size_t n = params.size();
  if (current.size() != n)
    throw std::invalid_argument("one current spend per channel is required");
  if (bound < 0)
    throw std::invalid_argument("bound must be >= 0");

  Allocation a;
  a.current = current;
  a.lower.resize(n);
  a.upper.resize(n);
  for (size_t i = 0; i < n; ++i) {
    a.lower[i] = std::max(0.0, (1.0 - bound) * current[i]);
    a.upper[i] = (1.0 + bound) * current[i];
  }
  const double min_total = std::accumulate(a.lower.begin(), a.lower.end(), 0.0);
  const double max_total = std::accumulate(a.upper.begin(), a.upper.end(), 0.0);
  if (budget < min_total - 1e-9 || budget > max_total + 1e-9)
    throw std::invalid_argument(
        "budget " + std::to_string(budget) + " is outside the reachable range [" +
        std::to_string(min_total) + ", " + std::to_string(max_total) +
        "] for bound " + std::to_string(bound) + "; widen --bound");

  // 1. Greedy: hand out the budget above the lower bounds in small steps,
  //    each to the channel with the largest gain for that step.
  std::vector<double> x = a.lower;
  const int steps = 4000;
  const double remaining = budget - min_total;
  const double step = remaining / steps;
  for (int s = 0; s < steps && step > 0; ++s) {
    size_t best = n;
    double best_gain = -1;
    for (size_t i = 0; i < n; ++i) {
      if (x[i] + step > a.upper[i] + 1e-9)
        continue;
      const double gain =
          steady_response(params[i], x[i] + step) - steady_response(params[i], x[i]);
      if (gain > best_gain) {
        best_gain = gain;
        best = i;
      }
    }
    if (best == n)
      break;
    x[best] += step;
  }
  // Put any rounding remainder where it still fits.
  double gap = budget - std::accumulate(x.begin(), x.end(), 0.0);
  for (size_t i = 0; i < n && gap > 1e-12; ++i) {
    const double add = std::min(gap, a.upper[i] - x[i]);
    x[i] += add;
    gap -= add;
  }

  // 2. Pairwise exchange: move budget between two channels while it helps,
  //    with shrinking move sizes. Escapes greedy's myopia on S-curves.
  double move = std::max(budget, 1e-9) / 20.0;
  double best_total = total_response(params, x);
  while (move > budget * 1e-6) {
    bool improved = false;
    for (size_t from = 0; from < n; ++from) {
      for (size_t to = 0; to < n; ++to) {
        if (from == to)
          continue;
        const double d =
            std::min({move, x[from] - a.lower[from], a.upper[to] - x[to]});
        if (d <= 0)
          continue;
        x[from] -= d;
        x[to] += d;
        const double t = total_response(params, x);
        if (t > best_total + 1e-12) {
          best_total = t;
          improved = true;
        } else {
          x[from] += d;
          x[to] -= d;
        }
      }
    }
    if (!improved)
      move /= 2;
  }

  a.optimal = x;
  a.current_response = total_response(params, current);
  a.optimal_response = best_total;
  for (size_t i = 0; i < n; ++i) {
    a.mroi_current.push_back(marginal_return(params[i], current[i]));
    a.mroi_optimal.push_back(marginal_return(params[i], x[i]));
  }
  return a;
}
