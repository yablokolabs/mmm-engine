#pragma once

#include <vector>
#include <span>

std::vector<double> apply_hill(std::span<const double> x, double alpha, double p);
