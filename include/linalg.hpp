#pragma once

#include <vector>

// Dense row-major design matrix: rows = observations, cols = features.
struct Design {
  size_t rows = 0;
  size_t cols = 0;
  std::vector<double> data;
  double &at(size_t r, size_t c) { return data[r * cols + c]; }
  double at(size_t r, size_t c) const { return data[r * cols + c]; }
};

// Ridge least squares: argmin ||X b - y||^2 + ridge * ||b||^2 over the
// columns where `active[c]` is true; inactive columns get b = 0. Solved with
// Cholesky on the normal equations. Returns false if the system is singular.
bool solve_least_squares(const Design &X, const std::vector<double> &y,
                         const std::vector<bool> &active, double ridge,
                         std::vector<double> &beta);
