#include "json_io.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

using json = nlohmann::json;

static json read_json(const std::string &path) {
  std::ifstream f(path);
  if (!f)
    throw std::runtime_error("cannot open " + path);
  json j;
  f >> j;
  return j;
}

static void write_json(const std::string &path, const json &j) {
  std::ofstream f(path);
  if (!f)
    throw std::runtime_error("cannot write " + path);
  f << j.dump(4);
}

MMM loadMMMConfig(const std::string &path) {
  MMM model;
  json j = read_json(path);
  model.t0 = j["trend"]["t0"];
  model.t1 = j["trend"]["t1"];
  model.t2 = j["trend"]["t2"];
  model.a = j["seasonality"]["a"].get<std::vector<double>>();
  model.b = j["seasonality"]["b"].get<std::vector<double>>();
  for (auto &jc : j["channels"]) {
    Channel ch;
    ch.name = jc.value("name", "channel_" + std::to_string(model.channels.size() + 1));
    ch.params.beta = jc["params"]["beta"];
    ch.params.alpha = jc["params"]["alpha"];
    ch.params.p = jc["params"]["p"];
    ch.params.decay = jc["params"]["decay"];
    ch.spend = jc["spend"].get<std::vector<double>>();
    model.channels.push_back(ch);
  }
  if (model.channels.empty())
    throw std::runtime_error(path + " has no channels");
  model.y = std::vector<double>(model.channels[0].spend.size(), 0);
  return model;
}

void savePrediction(const std::string &path, const std::vector<double> &pred) {
  json j;
  j["prediction"] = pred;
  write_json(path, j);
}

void saveFit(const std::string &path, const Dataset &data, const FitResult &fit,
             const FitOptions &opts) {
  const MMM &m = fit.model;
  json j;
  j["trend"] = {{"t0", m.t0}, {"t1", m.t1}, {"t2", m.t2}};
  j["seasonality"] = {{"a", m.a}, {"b", m.b}};
  j["channels"] = json::array();
  for (const auto &ch : m.channels)
    j["channels"].push_back({{"name", ch.name},
                             {"params",
                              {{"beta", ch.params.beta},
                               {"alpha", ch.params.alpha},
                               {"p", ch.params.p},
                               {"decay", ch.params.decay}}},
                             {"spend", ch.spend}});
  j["dates"] = data.dates;
  j["target"] = {{"name", data.target_name}, {"values", data.target}};

  json channels = json::array();
  for (const auto &r : fit.channels)
    channels.push_back({{"name", r.name},
                        {"spend_total", r.spend_total},
                        {"contribution_total", r.contribution_total},
                        {"share", r.share},
                        {"roi", r.roi},
                        {"current_weekly_spend", r.current_spend},
                        {"mroi", r.mroi}});
  j["fit"] = {{"method", "coordinate grid search + non-negative least squares"},
              {"train_weeks", fit.train_weeks},
              {"holdout_weeks", fit.holdout_weeks},
              {"passes", fit.passes},
              {"train", {{"r2", fit.train.r2}, {"mape", fit.train.mape}}},
              {"holdout", {{"r2", fit.holdout.r2}, {"mape", fit.holdout.mape}}},
              {"base_share", fit.base_share},
              {"training_model_prediction", fit.holdout_prediction},
              {"warnings", fit.warnings},
              {"channels", channels},
              {"options",
               {{"fourier_terms", opts.fourier_terms},
                {"decay_grid", opts.decay_grid},
                {"slope_grid", opts.slope_grid},
                {"half_saturation_grid", opts.half_sat_grid},
                {"max_correlation", opts.max_correlation}}}};
  write_json(path, j);
}

void savePlan(const std::string &path, const MMM &model, const Allocation &a,
              double bound) {
  json channels = json::array();
  for (size_t i = 0; i < model.channels.size(); ++i)
    channels.push_back({{"name", model.channels[i].name},
                        {"current_weekly_spend", a.current[i]},
                        {"optimal_weekly_spend", a.optimal[i]},
                        {"lower_bound", a.lower[i]},
                        {"upper_bound", a.upper[i]},
                        {"current_weekly_contribution",
                         steady_response(model.channels[i].params, a.current[i])},
                        {"optimal_weekly_contribution",
                         steady_response(model.channels[i].params, a.optimal[i])},
                        {"mroi_current", a.mroi_current[i]},
                        {"mroi_optimal", a.mroi_optimal[i]}});
  double budget = 0;
  for (double v : a.optimal)
    budget += v;
  const double lift = a.optimal_response - a.current_response;
  json j = {{"weekly_budget", budget},
            {"bound", bound},
            {"current_weekly_media_contribution", a.current_response},
            {"optimal_weekly_media_contribution", a.optimal_response},
            {"weekly_lift", lift},
            {"lift_pct", a.current_response > 0 ? lift / a.current_response : 0},
            {"channels", channels},
            {"assumption", "steady-state weekly response; spend held within bounds of the fitted range"}};
  write_json(path, j);
}

std::vector<double> loadPlanSpend(const std::string &path, const MMM &model) {
  json j = read_json(path);
  std::vector<double> spend;
  for (const auto &ch : model.channels) {
    bool found = false;
    for (const auto &jc : j["channels"])
      if (jc.value("name", "") == ch.name) {
        spend.push_back(jc["optimal_weekly_spend"]);
        found = true;
      }
    if (!found)
      throw std::runtime_error(path + " has no plan for channel '" + ch.name + "'");
  }
  return spend;
}

void saveForecast(const std::string &path, const std::vector<double> &pred,
                  size_t history_weeks, const std::vector<double> &weekly_spend,
                  const MMM &model) {
  json plan = json::object();
  for (size_t i = 0; i < model.channels.size(); ++i)
    plan[model.channels[i].name] = weekly_spend[i];
  json j;
  j["prediction"] = pred;
  j["history_weeks"] = history_weeks;
  j["future"] = std::vector<double>(pred.begin() + history_weeks, pred.end());
  j["weekly_spend_plan"] = plan;
  write_json(path, j);
}
