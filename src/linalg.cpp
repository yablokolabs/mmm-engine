#include "linalg.hpp"

#include <cmath>

bool solve_least_squares(const Design &X, const std::vector<double> &y,
                         const std::vector<bool> &active, double ridge,
                         std::vector<double> &beta) {
  std::vector<size_t> cols;
  for (size_t c = 0; c < X.cols; ++c)
    if (active[c])
      cols.push_back(c);
  const size_t k = cols.size();
  beta.assign(X.cols, 0.0);
  if (k == 0)
    return true;

  // Normal equations A = X'X + ridge I, rhs = X'y over the active columns.
  std::vector<double> A(k * k, 0.0), rhs(k, 0.0);
  for (size_t r = 0; r < X.rows; ++r) {
    for (size_t i = 0; i < k; ++i) {
      const double xi = X.at(r, cols[i]);
      rhs[i] += xi * y[r];
      for (size_t j = 0; j <= i; ++j)
        A[i * k + j] += xi * X.at(r, cols[j]);
    }
  }
  for (size_t i = 0; i < k; ++i) {
    A[i * k + i] += ridge;
    for (size_t j = 0; j < i; ++j)
      A[j * k + i] = A[i * k + j];
  }

  // Cholesky A = L L' in place (lower triangle).
  for (size_t j = 0; j < k; ++j) {
    double d = A[j * k + j];
    for (size_t m = 0; m < j; ++m)
      d -= A[j * k + m] * A[j * k + m];
    if (d <= 1e-12)
      return false;
    const double ljj = std::sqrt(d);
    A[j * k + j] = ljj;
    for (size_t i = j + 1; i < k; ++i) {
      double s = A[i * k + j];
      for (size_t m = 0; m < j; ++m)
        s -= A[i * k + m] * A[j * k + m];
      A[i * k + j] = s / ljj;
    }
  }

  // Forward then back substitution.
  std::vector<double> z(k);
  for (size_t i = 0; i < k; ++i) {
    double s = rhs[i];
    for (size_t m = 0; m < i; ++m)
      s -= A[i * k + m] * z[m];
    z[i] = s / A[i * k + i];
  }
  for (size_t ii = k; ii-- > 0;) {
    double s = z[ii];
    for (size_t m = ii + 1; m < k; ++m)
      s -= A[m * k + ii] * beta[cols[m]];
    beta[cols[ii]] = s / A[ii * k + ii];
  }
  return true;
}
