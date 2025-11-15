#include "optimizer.hpp"

#include <numeric>

std::vector<double> optimize_budget(
    const std::vector<double>& start,
    double total,
    int iters,
    double lr
) {
    std::vector<double> b = start;
    for (int it = 0; it < iters; ++it) {
        for (double& x : b)
            x += lr;  // dummy gradient
        double sum = std::accumulate(b.begin(), b.end(), 0.0);
        double scale = total / sum;
        for (double& x : b)
            x *= scale;
    }
    return b;
}
