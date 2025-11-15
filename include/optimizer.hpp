#pragma once

#include <vector>

std::vector<double> optimize_budget(const std::vector<double> &start,
                                    double total, int iters, double lr);
