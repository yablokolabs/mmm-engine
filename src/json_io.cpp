#include "json_io.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

MMM loadMMMConfig(const std::string& path) {
    MMM model;
    std::ifstream f(path);
    json j;
    f >> j;
    model.t0 = j["trend"]["t0"];
    model.t1 = j["trend"]["t1"];
    model.t2 = j["trend"]["t2"];
    model.a = j["seasonality"]["a"].get<std::vector<double>>();
    model.b = j["seasonality"]["b"].get<std::vector<double>>();
    for (auto& jc : j["channels"]) {
        Channel ch;
        ch.params.beta  = jc["params"]["beta"];
        ch.params.alpha = jc["params"]["alpha"];
        ch.params.p     = jc["params"]["p"];
        ch.params.decay = jc["params"]["decay"];
        ch.spend = jc["spend"].get<std::vector<double>>();
        model.channels.push_back(ch);
    }
    model.y = std::vector<double>(model.channels[0].spend.size(), 0);
    return model;
}

void savePrediction(const std::string& path, const std::vector<double>& pred) {
    json j;
    j["prediction"] = pred;
    std::ofstream f(path);
    f << j.dump(4);
}
