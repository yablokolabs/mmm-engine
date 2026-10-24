#pragma once

#include <string>
#include <vector>

#include "data_io.hpp"
#include "mmm.hpp"

struct FitOptions {
  size_t holdout_weeks = 13; // last N weeks scored, never used for fitting
  size_t fourier_terms = 2;  // yearly seasonality harmonics (period 52)
  size_t max_passes = 6;     // coordinate-descent passes over the channels
  double ridge = 1e-6;       // tiny ridge for numerical stability
  std::vector<double> decay_grid{0.0, 0.1, 0.2, 0.3, 0.4,
                                 0.5, 0.6, 0.7, 0.8};
  std::vector<double> slope_grid{1.0, 1.5, 2.0, 2.5, 3.0};
  // Half-saturation as a multiple of the channel's median adstocked spend.
  std::vector<double> half_sat_grid{0.3, 0.5, 0.75, 1.0, 1.5, 2.0, 3.0};
};

struct Accuracy {
  double r2 = 0;
  double mape = 0; // mean absolute percentage error, as a fraction
};

struct ChannelReport {
  std::string name;
  double spend_total = 0;
  double contribution_total = 0;
  double share = 0;       // contribution / total target
  double roi = 0;         // contribution per unit spend over the history
  double current_spend = 0; // mean weekly spend over the last 13 weeks
  double mroi = 0;        // marginal return per extra unit at current spend
};

struct FitResult {
  MMM model;          // parameters refit on all weeks, ready for `mmm run`
  Accuracy train;     // model fit on the training weeks
  Accuracy holdout;   // same model scored on the held-out weeks
  size_t train_weeks = 0;
  size_t holdout_weeks = 0;
  size_t passes = 0;
  double base_share = 0; // trend + seasonality share of the target
  std::vector<ChannelReport> channels;
};

// Deterministic fit: coordinate-descent grid search over each channel's
// decay, half-saturation and slope; trend, seasonality and channel betas
// solved by least squares with betas constrained to be non-negative.
FitResult fit_mmm(const Dataset &data, const FitOptions &opts);

Accuracy score(const std::vector<double> &actual,
               const std::vector<double> &predicted, size_t from, size_t to);
