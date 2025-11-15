#include "mmm.hpp"

#include "trend.hpp"
#include "seasonality.hpp"
#include <algorithm>
#include <execution>

std::vector<double> MMM::predict() const {
    size_t T = y.size();
    std::vector<double> out(T);
    // multithreaded channel compute
    std::vector<Channel> chs = channels;
    std::for_each(std::execution::par_unseq, chs.begin(), chs.end(),
        [&](Channel& ch){ compute_channel(ch); });
    for (size_t t = 0; t < T; ++t) {
        double yhat = trend(t, t0, t1, t2) + season(t, a, b);
        for (auto& ch : chs)
            yhat += ch.contrib[t];
        out[t] = yhat;
    }
    return out;
}
