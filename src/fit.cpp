#include "fit.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "adstock.hpp"
#include "linalg.hpp"
#include "optimizer.hpp"
#include "saturation.hpp"
#include "seasonality.hpp"
#include "trend.hpp"

namespace {

constexpr double kTwoPi = 6.283185307179586;

// Column layout: [intercept, t/T, (t/T)^2, cos_1, sin_1, ..., media_1..m]
struct Layout {
  size_t base;  // number of non-media columns
  size_t media; // number of channels
  size_t cols() const { return base + media; }
};

Design base_design(size_t rows, size_t T, size_t fourier, size_t media) {
  Design X;
  X.rows = rows;
  X.cols = 3 + 2 * fourier + media;
  X.data.assign(X.rows * X.cols, 0.0);
  for (size_t t = 0; t < rows; ++t) {
    const double u = static_cast<double>(t) / static_cast<double>(T);
    X.at(t, 0) = 1.0;
    X.at(t, 1) = u;
    X.at(t, 2) = u * u;
    for (size_t k = 0; k < fourier; ++k) {
      const double w = kTwoPi * static_cast<double>(k + 1) * t / 52.0;
      X.at(t, 3 + 2 * k) = std::cos(w);
      X.at(t, 4 + 2 * k) = std::sin(w);
    }
  }
  return X;
}

double median_positive(const std::vector<double> &v, size_t n) {
  std::vector<double> pos;
  for (size_t i = 0; i < n && i < v.size(); ++i)
    if (v[i] > 0)
      pos.push_back(v[i]);
  if (pos.empty())
    return 1.0;
  std::nth_element(pos.begin(), pos.begin() + pos.size() / 2, pos.end());
  return pos[pos.size() / 2];
}

// Least squares with non-negative media betas: drop the most negative media
// column and re-solve until every remaining media beta is >= 0.
bool solve_nonneg(const Design &X, const std::vector<double> &y, size_t base,
                  double ridge, std::vector<double> &beta) {
  std::vector<bool> active(X.cols, true);
  for (size_t guard = 0; guard <= X.cols; ++guard) {
    if (!solve_least_squares(X, y, active, ridge, beta))
      return false;
    size_t worst = X.cols;
    double worst_value = 0;
    for (size_t c = base; c < X.cols; ++c)
      if (active[c] && beta[c] < worst_value) {
        worst_value = beta[c];
        worst = c;
      }
    if (worst == X.cols)
      return true;
    active[worst] = false;
  }
  return true;
}

double sse(const Design &X, const std::vector<double> &y,
           const std::vector<double> &beta) {
  double s = 0;
  for (size_t r = 0; r < X.rows; ++r) {
    double yhat = 0;
    for (size_t c = 0; c < X.cols; ++c)
      yhat += X.at(r, c) * beta[c];
    s += (y[r] - yhat) * (y[r] - yhat);
  }
  return s;
}

std::vector<double> media_column(const std::vector<double> &spend,
                                 const ChannelParams &p) {
  return apply_hill(adstock_geometric(spend, p.decay), p.alpha, p.p);
}

} // namespace

Accuracy score(const std::vector<double> &actual,
               const std::vector<double> &predicted, size_t from, size_t to) {
  Accuracy a;
  if (to <= from)
    return a;
  double mean = 0;
  for (size_t t = from; t < to; ++t)
    mean += actual[t];
  mean /= static_cast<double>(to - from);
  double ss_res = 0, ss_tot = 0, ape = 0;
  size_t ape_n = 0;
  for (size_t t = from; t < to; ++t) {
    const double e = actual[t] - predicted[t];
    ss_res += e * e;
    ss_tot += (actual[t] - mean) * (actual[t] - mean);
    if (std::abs(actual[t]) > 1e-12) {
      ape += std::abs(e / actual[t]);
      ++ape_n;
    }
  }
  a.r2 = ss_tot > 0 ? 1.0 - ss_res / ss_tot : 0.0;
  a.mape = ape_n ? ape / static_cast<double>(ape_n) : 0.0;
  return a;
}

FitResult fit_mmm(const Dataset &data, const FitOptions &opts) {
  const size_t T = data.target.size();
  const size_t m = data.channel_names.size();
  if (m == 0)
    throw std::invalid_argument("no channel spend columns");
  if (opts.holdout_weeks >= T)
    throw std::invalid_argument("holdout must be shorter than the history");
  const size_t n_train = T - opts.holdout_weeks;
  const size_t base = 3 + 2 * opts.fourier_terms;
  if (n_train < 2 * (base + m))
    throw std::invalid_argument(
        "too few training weeks (" + std::to_string(n_train) + ") for " +
        std::to_string(base + m) + " coefficients; need at least " +
        std::to_string(2 * (base + m)));

  std::vector<double> y_train(data.target.begin(), data.target.begin() + n_train);
  Design X = base_design(n_train, T, opts.fourier_terms, m);

  // Starting point: moderate carryover, curve centred on typical spend.
  std::vector<ChannelParams> params(m);
  for (size_t i = 0; i < m; ++i) {
    params[i] = {0.0, 1.0, 1.5, 0.3};
    params[i].alpha = median_positive(
        adstock_geometric(data.spend[i], params[i].decay), n_train);
    const auto col = media_column(data.spend[i], params[i]);
    for (size_t t = 0; t < n_train; ++t)
      X.at(t, base + i) = col[t];
  }

  std::vector<double> beta;
  if (!solve_nonneg(X, y_train, base, opts.ridge, beta))
    throw std::runtime_error("design matrix is singular; check for constant or duplicate columns");
  double best_sse = sse(X, y_train, beta);

  FitResult result;
  for (size_t pass = 0; pass < opts.max_passes; ++pass) {
    bool changed = false;
    for (size_t i = 0; i < m; ++i) {
      ChannelParams best_p = params[i];
      std::vector<double> best_col(n_train);
      for (size_t t = 0; t < n_train; ++t)
        best_col[t] = X.at(t, base + i);

      for (double decay : opts.decay_grid) {
        const auto ad = adstock_geometric(data.spend[i], decay);
        const double med = median_positive(ad, n_train);
        for (double mult : opts.half_sat_grid) {
          for (double slope : opts.slope_grid) {
            const ChannelParams cand{0.0, mult * med, slope, decay};
            const auto col = apply_hill(ad, cand.alpha, cand.p);
            for (size_t t = 0; t < n_train; ++t)
              X.at(t, base + i) = col[t];
            std::vector<double> b;
            if (!solve_nonneg(X, y_train, base, opts.ridge, b))
              continue;
            const double s = sse(X, y_train, b);
            if (s < best_sse * (1.0 - 1e-9)) {
              best_sse = s;
              best_p = cand;
              for (size_t t = 0; t < n_train; ++t)
                best_col[t] = col[t];
              changed = true;
            }
          }
        }
      }
      params[i] = best_p;
      for (size_t t = 0; t < n_train; ++t)
        X.at(t, base + i) = best_col[t];
    }
    result.passes = pass + 1;
    if (!changed)
      break;
  }

  // Score the training-only model on train and holdout weeks.
  solve_nonneg(X, y_train, base, opts.ridge, beta);
  Design X_all = base_design(T, T, opts.fourier_terms, m);
  for (size_t i = 0; i < m; ++i) {
    const auto col = media_column(data.spend[i], params[i]);
    for (size_t t = 0; t < T; ++t)
      X_all.at(t, base + i) = col[t];
  }
  std::vector<double> pred(T, 0.0);
  for (size_t t = 0; t < T; ++t)
    for (size_t c = 0; c < X_all.cols; ++c)
      pred[t] += X_all.at(t, c) * beta[c];
  result.train = score(data.target, pred, 0, n_train);
  result.holdout = score(data.target, pred, n_train, T);
  result.train_weeks = n_train;
  result.holdout_weeks = opts.holdout_weeks;

  // Final model: same curves, linear terms refit on every week.
  std::vector<double> final_beta;
  if (!solve_nonneg(X_all, data.target, base, opts.ridge, final_beta))
    throw std::runtime_error("final refit failed");

  MMM &model = result.model;
  const double Td = static_cast<double>(T);
  model.t0 = final_beta[0];
  model.t1 = final_beta[1] / Td;
  model.t2 = final_beta[2] / (Td * Td);
  for (size_t k = 0; k < opts.fourier_terms; ++k) {
    model.a.push_back(final_beta[3 + 2 * k]);
    model.b.push_back(final_beta[4 + 2 * k]);
  }
  model.y = data.target;
  double target_total = 0;
  for (double v : data.target)
    target_total += v;

  double base_total = 0;
  for (size_t t = 0; t < T; ++t)
    base_total += trend(t, model.t0, model.t1, model.t2) + season(t, model.a, model.b);
  result.base_share = target_total != 0 ? base_total / target_total : 0;

  for (size_t i = 0; i < m; ++i) {
    Channel ch;
    ch.name = data.channel_names[i];
    ch.params = params[i];
    ch.params.beta = final_beta[base + i];
    ch.spend = data.spend[i];
    compute_channel(ch);

    ChannelReport r;
    r.name = ch.name;
    for (size_t t = 0; t < T; ++t) {
      r.spend_total += ch.spend[t];
      r.contribution_total += ch.contrib[t];
    }
    r.share = target_total != 0 ? r.contribution_total / target_total : 0;
    r.roi = r.spend_total > 0 ? r.contribution_total / r.spend_total : 0;
    const size_t recent = std::min<size_t>(13, T);
    for (size_t t = T - recent; t < T; ++t)
      r.current_spend += ch.spend[t];
    r.current_spend /= static_cast<double>(recent);
    r.mroi = marginal_return(ch.params, r.current_spend);

    result.channels.push_back(r);
    model.channels.push_back(std::move(ch));
  }
  return result;
}
