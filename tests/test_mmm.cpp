// Plain assert-style tests; run with `ctest` or ./build/mmm_tests.
#include <cmath>
#include <cstdio>
#include <fstream>
#include <functional>
#include <random>
#include <string>
#include <vector>

#include "adstock.hpp"
#include "data_io.hpp"
#include "fit.hpp"
#include "json_io.hpp"
#include "optimizer.hpp"
#include "saturation.hpp"
#include "seasonality.hpp"
#include "trend.hpp"

static int failures = 0;

#define CHECK(cond, msg)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::printf("  FAIL %s:%d %s\n", __FILE__, __LINE__, msg);              \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

// Synthetic history from known parameters, so the fit can be graded.
struct Truth {
  std::vector<ChannelParams> params;
  std::vector<double> shares;
  Dataset data;
};

static Truth make_truth(unsigned seed, double noise_sd) {
  const size_t T = 156;
  Truth tr;
  tr.data.target_name = "sales";
  tr.data.channel_names = {"tv", "search", "social"};
  tr.params = {{3000, 1500, 2.0, 0.6}, {2500, 400, 1.2, 0.1}, {1200, 600, 1.5, 0.3}};
  const double mean_spend[] = {800, 300, 450};
  std::mt19937 rng(seed);
  std::uniform_real_distribution<double> u(0.4, 1.6);
  std::normal_distribution<double> eps(0.0, noise_sd);
  tr.data.spend.assign(3, std::vector<double>(T));
  for (size_t i = 0; i < 3; ++i)
    for (size_t t = 0; t < T; ++t) {
      // tv runs in flights: off for 4 weeks out of every 10
      const bool on = i != 0 || (t % 10) < 6;
      tr.data.spend[i][t] = on ? mean_spend[i] * u(rng) : 0.0;
    }
  std::vector<double> contrib_total(3, 0.0);
  double total = 0;
  std::vector<std::vector<double>> contrib(3);
  for (size_t i = 0; i < 3; ++i) {
    const auto ad = adstock_geometric(tr.data.spend[i], tr.params[i].decay);
    const auto sat = apply_hill(ad, tr.params[i].alpha, tr.params[i].p);
    for (size_t t = 0; t < T; ++t)
      contrib[i].push_back(tr.params[i].beta * sat[t]);
  }
  for (size_t t = 0; t < T; ++t) {
    double y = trend(t, 10000, 8, 0) + season(t, {600, 150}, {-300, 0});
    for (size_t i = 0; i < 3; ++i) {
      y += contrib[i][t];
      contrib_total[i] += contrib[i][t];
    }
    y += eps(rng);
    tr.data.target.push_back(y);
    tr.data.dates.push_back("w" + std::to_string(t));
    total += y;
  }
  for (double c : contrib_total)
    tr.shares.push_back(c / total);
  return tr;
}

static void test_fit_recovers_known_truth() {
  std::puts("fit recovers known channel shares");
  const Truth tr = make_truth(7, 120.0);
  FitOptions opts;
  const FitResult fit = fit_mmm(tr.data, opts);
  CHECK(fit.holdout.r2 > 0.85, "holdout R2 above 0.85");
  CHECK(fit.holdout.mape < 0.03, "holdout MAPE below 3%");
  for (size_t i = 0; i < 3; ++i) {
    const double err = std::abs(fit.channels[i].share - tr.shares[i]);
    std::printf("    %-7s true share %5.1f%%  fitted %5.1f%%  decay true %.1f fitted %.1f\n",
                tr.data.channel_names[i].c_str(), tr.shares[i] * 100,
                fit.channels[i].share * 100, tr.params[i].decay,
                fit.model.channels[i].params.decay);
    CHECK(err < 0.03, "channel share within 3 points of truth");
  }
}

static void test_fitted_plan_captures_true_lift() {
  std::puts("plan from the fitted model captures most of the true achievable lift");
  const Truth tr = make_truth(7, 120.0);
  const FitResult fit = fit_mmm(tr.data, FitOptions{});
  std::vector<ChannelParams> fitted;
  std::vector<double> cur;
  double budget = 0;
  for (size_t i = 0; i < 3; ++i) {
    fitted.push_back(fit.model.channels[i].params);
    cur.push_back(fit.channels[i].current_spend);
    budget += cur.back();
  }
  const Allocation plan = optimize_budget(fitted, cur, budget, 0.5);
  const Allocation ideal = optimize_budget(tr.params, cur, budget, 0.5);
  double true_current = 0, true_plan = 0;
  for (size_t i = 0; i < 3; ++i) {
    true_current += steady_response(tr.params[i], cur[i]);
    true_plan += steady_response(tr.params[i], plan.optimal[i]);
  }
  const double captured =
      (true_plan - true_current) / (ideal.optimal_response - true_current);
  std::printf("    captured %.0f%% of the lift available under the true model\n",
              captured * 100);
  CHECK(captured > 0.8, "fitted plan captures over 80% of achievable lift");
}

static void test_fit_is_deterministic() {
  std::puts("fit is deterministic");
  const Truth tr = make_truth(11, 120.0);
  const FitResult a = fit_mmm(tr.data, FitOptions{});
  const FitResult b = fit_mmm(tr.data, FitOptions{});
  for (size_t i = 0; i < 3; ++i)
    CHECK(a.model.channels[i].params.beta == b.model.channels[i].params.beta,
          "same input gives identical betas");
}

static void test_optimizer() {
  std::puts("optimizer conserves budget, respects bounds, beats current mix");
  const Truth tr = make_truth(3, 0.0);
  const std::vector<double> cur{480, 300, 450};
  const double budget = 1230;
  const Allocation a = optimize_budget(tr.params, cur, budget, 0.5);
  double sum = 0;
  for (size_t i = 0; i < 3; ++i) {
    sum += a.optimal[i];
    CHECK(a.optimal[i] >= a.lower[i] - 1e-6 && a.optimal[i] <= a.upper[i] + 1e-6,
          "spend within bounds");
  }
  CHECK(std::abs(sum - budget) < 1e-6 * budget, "budget conserved");
  CHECK(a.optimal_response >= a.current_response, "optimal >= current response");

  // Brute force over a fine grid of the same constrained space.
  double brute = 0;
  for (double x0 = a.lower[0]; x0 <= a.upper[0]; x0 += 2)
    for (double x1 = a.lower[1]; x1 <= a.upper[1]; x1 += 2) {
      const double x2 = budget - x0 - x1;
      if (x2 < a.lower[2] || x2 > a.upper[2])
        continue;
      brute = std::max(brute, steady_response(tr.params[0], x0) +
                                  steady_response(tr.params[1], x1) +
                                  steady_response(tr.params[2], x2));
    }
  std::printf("    optimizer %.3f vs brute-force grid %.3f\n", a.optimal_response, brute);
  CHECK(a.optimal_response >= brute - 1e-3 * brute, "within 0.1% of brute force");
}

static void test_steady_state_matches_simulation() {
  std::puts("steady-state response matches a long simulated run");
  const ChannelParams p{1000, 500, 1.5, 0.6};
  std::vector<double> spend(400, 200.0);
  const auto sat = apply_hill(adstock_geometric(spend, p.decay), p.alpha, p.p);
  CHECK(std::abs(p.beta * sat.back() - steady_response(p, 200.0)) < 1e-6,
        "steady_response equals converged adstock + hill");
}

static void test_run_config_still_loads() {
  std::puts("existing run configs still load");
  const MMM m = loadMMMConfig(std::string(MMM_SOURCE_DIR) + "/examples/basic/model.json");
  const auto pred = m.predict();
  CHECK(pred.size() == m.channels[0].spend.size(), "one prediction per week");
}

static void test_unidentifiable_channels_are_refused() {
  std::puts("fit refuses constant and perfectly correlated channels");
  Truth tr = make_truth(5, 120.0);
  Dataset twin = tr.data;
  twin.channel_names.push_back("search_copy");
  std::vector<double> copy = twin.spend[1];
  for (double &v : copy)
    v *= 2.0; // same pattern, different scale: r = 1
  twin.spend.push_back(copy);
  try {
    fit_mmm(twin, FitOptions{});
    CHECK(false, "expected a correlated-channels error");
  } catch (const std::invalid_argument &e) {
    CHECK(std::string(e.what()).find("search_copy") != std::string::npos,
          "error names the correlated channel");
  }

  Dataset flat = tr.data;
  flat.channel_names.push_back("radio");
  flat.spend.push_back(std::vector<double>(flat.target.size(), 50.0));
  try {
    fit_mmm(flat, FitOptions{});
    CHECK(false, "expected a constant-channel error");
  } catch (const std::invalid_argument &e) {
    CHECK(std::string(e.what()).find("constant") != std::string::npos,
          "error says constant");
  }

  // Strong but separable correlation: fits, with a warning.
  Dataset close = tr.data;
  close.channel_names.push_back("search_echo");
  std::vector<double> echo = close.spend[1];
  std::mt19937 rng(1);
  std::uniform_real_distribution<double> jitter(0.85, 1.15);
  for (double &v : echo)
    v *= jitter(rng);
  close.spend.push_back(echo);
  const double r = correlation(close.spend[1], close.spend[3], close.target.size());
  std::printf("    echo correlation r = %.3f\n", r);
  const FitResult fit = fit_mmm(close, FitOptions{});
  CHECK(!fit.warnings.empty(), "correlated-but-separable pair produces a warning");
}

static void test_csv_errors() {
  std::puts("csv loader reports bad cells with line numbers");
  const std::string path = "mmm_test_bad.csv";
  std::ofstream(path) << "week,sales,tv\nw1,100,10\nw2,abc,10\n";
  try {
    load_csv(path, "sales");
    CHECK(false, "expected an error");
  } catch (const std::exception &e) {
    CHECK(std::string(e.what()).find("line 3") != std::string::npos, "names line 3");
  }
  std::remove(path.c_str());
}

int main() {
  test_steady_state_matches_simulation();
  test_optimizer();
  test_fit_recovers_known_truth();
  test_fitted_plan_captures_true_lift();
  test_fit_is_deterministic();
  test_run_config_still_loads();
  test_unidentifiable_channels_are_refused();
  test_csv_errors();
  std::printf("%s (%d failures)\n", failures ? "FAILED" : "ALL PASSED", failures);
  return failures ? 1 : 0;
}
