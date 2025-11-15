#pragma once

#include <span>
#include <vector>

std::vector<double> apply_hill(std::span<const double> x, double alpha,
                               double p);
