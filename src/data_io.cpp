#include "data_io.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

static std::vector<std::string> split_csv_line(const std::string &line) {
  std::vector<std::string> cells;
  std::string cell;
  std::stringstream ss(line);
  while (std::getline(ss, cell, ',')) {
    while (!cell.empty() && (cell.back() == '\r' || cell.back() == ' '))
      cell.pop_back();
    while (!cell.empty() && cell.front() == ' ')
      cell.erase(cell.begin());
    cells.push_back(cell);
  }
  return cells;
}

static double parse_number(const std::string &cell, size_t line,
                           const std::string &column) {
  try {
    size_t used = 0;
    double v = std::stod(cell, &used);
    if (used != cell.size())
      throw std::invalid_argument(cell);
    return v;
  } catch (const std::exception &) {
    throw std::runtime_error("line " + std::to_string(line) + ", column '" +
                             column + "': '" + cell + "' is not a number");
  }
}

Dataset load_csv(const std::string &path, const std::string &target) {
  std::ifstream f(path);
  if (!f)
    throw std::runtime_error("cannot open " + path);

  std::string line;
  if (!std::getline(f, line))
    throw std::runtime_error(path + " is empty");
  const auto header = split_csv_line(line);
  if (header.size() < 3)
    throw std::runtime_error(
        "need at least 3 columns: week, " + target + ", one channel's spend");

  Dataset d;
  d.target_name = target;
  size_t target_col = header.size();
  std::vector<size_t> channel_cols;
  for (size_t c = 1; c < header.size(); ++c) {
    if (header[c] == target)
      target_col = c;
    else {
      channel_cols.push_back(c);
      d.channel_names.push_back(header[c]);
    }
  }
  if (target_col == header.size())
    throw std::runtime_error("no '" + target + "' column in " + path);
  d.spend.resize(channel_cols.size());

  size_t line_no = 1;
  while (std::getline(f, line)) {
    ++line_no;
    if (line.find_first_not_of(" \r\t") == std::string::npos)
      continue;
    const auto cells = split_csv_line(line);
    if (cells.size() != header.size())
      throw std::runtime_error("line " + std::to_string(line_no) + ": " +
                               std::to_string(cells.size()) + " cells, header has " +
                               std::to_string(header.size()));
    d.dates.push_back(cells[0]);
    d.target.push_back(parse_number(cells[target_col], line_no, target));
    for (size_t i = 0; i < channel_cols.size(); ++i) {
      double v = parse_number(cells[channel_cols[i]], line_no, d.channel_names[i]);
      if (v < 0)
        throw std::runtime_error("line " + std::to_string(line_no) + ", column '" +
                                 d.channel_names[i] + "': spend cannot be negative");
      d.spend[i].push_back(v);
    }
  }
  return d;
}
