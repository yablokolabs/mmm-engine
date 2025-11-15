#pragma once

#include <string>
#include <vector>

#include "mmm.hpp"

MMM loadMMMConfig(const std::string& path);
void savePrediction(const std::string& path, const std::vector<double>& pred);
