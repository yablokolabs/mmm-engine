#include "channels.hpp"

#include "adstock.hpp"
#include "saturation.hpp"

void compute_channel(Channel &ch) {
  ch.adstocked = adstock_geometric(ch.spend, ch.params.decay);
  ch.saturated = apply_hill(ch.adstocked, ch.params.alpha, ch.params.p);
  ch.contrib.resize(ch.saturated.size());
  for (size_t i = 0; i < ch.saturated.size(); ++i)
    ch.contrib[i] = ch.params.beta * ch.saturated[i];
}
