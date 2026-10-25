#include <algorithm>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

#include "data_io.hpp"
#include "fit.hpp"
#include "json_io.hpp"
#include "mmm.hpp"
#include "optimizer.hpp"

static const char *kUsage = R"(Usage:
  mmm fit      --data history.csv [--target sales] [--holdout 13] [--fourier 2]
               [--max-correlation 0.98] --output model.json
  mmm optimize --config model.json [--budget WEEKLY_TOTAL] [--bound 0.5] --output plan.json
  mmm forecast --config model.json --weeks 26 [--plan plan.json] --output forecast.json
  mmm run      --config model.json --output out.json

fit       Learn adstock, saturation, trend, seasonality and channel effects
          from weekly history. The last --holdout weeks are scored, not fitted.
          Refuses constant channels and channel pairs whose spend correlates
          at or above --max-correlation; warns from 0.90.
optimize  Reallocate a weekly budget across channels to maximise sales.
          Default budget: current weekly spend (mean of the last 13 weeks).
          --bound 0.5 keeps each channel within +/-50% of its current spend.
forecast  Predict the next N weeks with weekly spend from --plan (or current spend).
run       Predict the history weeks of a config.
)";

static std::map<std::string, std::string> parse_flags(int argc, char **argv) {
  std::map<std::string, std::string> flags;
  for (int i = 2; i < argc; ++i) {
    std::string a = argv[i];
    if (a.rfind("--", 0) != 0)
      throw std::invalid_argument("unexpected argument '" + a + "'");
    if (i + 1 >= argc)
      throw std::invalid_argument(a + " needs a value");
    flags[a] = argv[++i];
  }
  return flags;
}

static std::string require(const std::map<std::string, std::string> &flags,
                           const std::string &name) {
  auto it = flags.find(name);
  if (it == flags.end())
    throw std::invalid_argument("missing " + name);
  return it->second;
}

static std::string get(const std::map<std::string, std::string> &flags,
                       const std::string &name, const std::string &fallback) {
  auto it = flags.find(name);
  return it == flags.end() ? fallback : it->second;
}

static std::vector<double> current_weekly_spend(const MMM &model) {
  std::vector<double> cur;
  for (const auto &ch : model.channels) {
    const size_t T = ch.spend.size();
    const size_t recent = std::min<size_t>(13, T);
    double s = 0;
    for (size_t t = T - recent; t < T; ++t)
      s += ch.spend[t];
    cur.push_back(recent ? s / static_cast<double>(recent) : 0.0);
  }
  return cur;
}

static int cmd_fit(const std::map<std::string, std::string> &flags) {
  const Dataset data = load_csv(require(flags, "--data"), get(flags, "--target", "sales"));
  FitOptions opts;
  opts.holdout_weeks = std::stoul(get(flags, "--holdout", "13"));
  opts.fourier_terms = std::stoul(get(flags, "--fourier", "2"));
  opts.max_correlation = std::stod(get(flags, "--max-correlation", "0.98"));
  const FitResult fit = fit_mmm(data, opts);
  const std::string out = require(flags, "--output");
  saveFit(out, data, fit, opts);

  std::cout << "fitted " << data.channel_names.size() << " channels on "
            << fit.train_weeks << " weeks (" << fit.passes << " passes)\n"
            << "train   R2 " << fit.train.r2 << "  MAPE " << fit.train.mape * 100 << "%\n";
  if (fit.holdout_weeks)
    std::cout << "holdout R2 " << fit.holdout.r2 << "  MAPE " << fit.holdout.mape * 100
              << "% (" << fit.holdout_weeks << " weeks never seen by the fit)\n";
  for (const auto &w : fit.warnings)
    std::cout << "warning: " << w << "\n";
  for (const auto &r : fit.channels)
    std::cout << "  " << r.name << ": share " << r.share * 100 << "%, ROI " << r.roi
              << ", marginal ROI " << r.mroi << "\n";
  std::cout << "Saved to " << out << "\n";
  return 0;
}

static int cmd_optimize(const std::map<std::string, std::string> &flags) {
  const MMM model = loadMMMConfig(require(flags, "--config"));
  const auto current = current_weekly_spend(model);
  double budget = 0;
  for (double v : current)
    budget += v;
  if (flags.count("--budget"))
    budget = std::stod(flags.at("--budget"));
  const double bound = std::stod(get(flags, "--bound", "0.5"));

  std::vector<ChannelParams> params;
  for (const auto &ch : model.channels)
    params.push_back(ch.params);
  const Allocation a = optimize_budget(params, current, budget, bound);
  const std::string out = require(flags, "--output");
  savePlan(out, model, a, bound);

  const double lift = a.optimal_response - a.current_response;
  std::cout << "weekly budget " << budget << ", bound +/-" << bound * 100 << "%\n";
  for (size_t i = 0; i < params.size(); ++i)
    std::cout << "  " << model.channels[i].name << ": " << a.current[i] << " -> "
              << a.optimal[i] << "\n";
  std::cout << "media-driven sales per week: " << a.current_response << " -> "
            << a.optimal_response << " (+" << lift << ")\n"
            << "Saved to " << out << "\n";
  return 0;
}

static int cmd_forecast(const std::map<std::string, std::string> &flags) {
  MMM model = loadMMMConfig(require(flags, "--config"));
  const size_t weeks = std::stoul(require(flags, "--weeks"));
  const std::vector<double> plan = flags.count("--plan")
                                       ? loadPlanSpend(flags.at("--plan"), model)
                                       : current_weekly_spend(model);
  const size_t history = model.y.size();
  for (size_t i = 0; i < model.channels.size(); ++i)
    model.channels[i].spend.insert(model.channels[i].spend.end(), weeks, plan[i]);
  model.y.assign(history + weeks, 0.0);
  const auto pred = model.predict();
  const std::string out = require(flags, "--output");
  saveForecast(out, pred, history, plan, model);
  std::cout << "Saved to " << out << "\n";
  return 0;
}

static int cmd_run(const std::map<std::string, std::string> &flags) {
  const MMM m = loadMMMConfig(require(flags, "--config"));
  const auto pred = m.predict();
  const std::string out = require(flags, "--output");
  savePrediction(out, pred);
  std::cout << "Saved to " << out << "\n";
  return 0;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << kUsage;
    return 1;
  }
  const std::string cmd = argv[1];
  try {
    const auto flags = parse_flags(argc, argv);
    if (cmd == "fit")
      return cmd_fit(flags);
    if (cmd == "optimize")
      return cmd_optimize(flags);
    if (cmd == "forecast")
      return cmd_forecast(flags);
    if (cmd == "run")
      return cmd_run(flags);
  } catch (const std::exception &e) {
    std::cerr << "mmm " << cmd << ": " << e.what() << "\n";
    return 1;
  }
  std::cout << kUsage;
  return cmd == "--help" || cmd == "help" ? 0 : 1;
}
