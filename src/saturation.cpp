#include "saturation.hpp"

#include <cmath>

static inline double hill(double x, double alpha, double p) {
    double xp = std::pow(x, p);
    return xp / (std::pow(alpha, p) + xp + 1e-9);
}

std::vector<double> apply_hill(std::span<const double> x, double alpha, double p) {
    std::vector<double> out(x.size());
    for (size_t i = 0; i < x.size(); ++i)
        out[i] = hill(x[i], alpha, p);
    return out;
}
