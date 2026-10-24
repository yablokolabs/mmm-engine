#pragma once

#include <string>
#include <vector>

// Weekly history: one row per week, a target (e.g. sales) and one spend
// column per channel.
struct Dataset {
  std::vector<std::string> dates;
  std::string target_name;
  std::vector<double> target;
  std::vector<std::string> channel_names;
  std::vector<std::vector<double>> spend; // [channel][week]
};

// Reads a CSV whose first column is the week label, `target` names the
// response column, and every other column is a channel's spend. Throws
// std::runtime_error with the offending line on bad input.
Dataset load_csv(const std::string &path, const std::string &target);
