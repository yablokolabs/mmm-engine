#include "adstock.hpp"

std::vector<double> adstock_geometric(std::span<const double> x, double decay) {
    std::vector<double> out(x.size());
    double carry = 0.0;
    for (size_t i = 0; i < x.size(); ++i) {
        carry = x[i] + carry * decay;
        out[i] = carry;
    }
    return out;
}
