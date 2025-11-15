#include "seasonality.hpp"

#include <cmath>

double season(size_t t, const std::vector<double>& a, const std::vector<double>& b) {
    double s = 0.0;
    for (size_t k = 0; k < a.size(); ++k) {
        s += a[k] * std::cos(2 * M_PI * (k + 1) * t / 52.0);
        s += b[k] * std::sin(2 * M_PI * (k + 1) * t / 52.0);
    }
    return s;
}
