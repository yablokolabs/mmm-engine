#pragma once

#include <string>
#include <vector>

#include "data_io.hpp"
#include "fit.hpp"
#include "mmm.hpp"
#include "optimizer.hpp"

MMM loadMMMConfig(const std::string &path);
void savePrediction(const std::string &path, const std::vector<double> &pred);

// Writes the fitted model in the `run` config schema plus a "fit" block with
// accuracy and per-channel contribution, ROI and marginal ROI.
void saveFit(const std::string &path, const Dataset &data, const FitResult &fit,
             const FitOptions &opts);

// Writes a budget plan: per-channel current vs optimal weekly spend.
void savePlan(const std::string &path, const MMM &model, const Allocation &a,
              double bound);

// Reads the "optimal_weekly_spend" per channel from a plan written by savePlan.
std::vector<double> loadPlanSpend(const std::string &path, const MMM &model);

void saveForecast(const std::string &path, const std::vector<double> &pred,
                  size_t history_weeks, const std::vector<double> &weekly_spend,
                  const MMM &model);
