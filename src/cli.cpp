#include <iostream>

#include "mmm.hpp"
#include "json_io.hpp"
#include "seasonality.hpp"
#include "trend.hpp"

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage:\n"
                  << "  mmm run --config model.json --output out.json\n"
                  << "  mmm forecast --config model.json --weeks 26 --output out.json\n";
        return 1;
    }
    std::string cmd = argv[1];
    if (cmd == "run") {
        std::string cfg, out;
        for (int i = 2; i < argc; ++i) {
            std::string a = argv[i];
            if (a == "--config") cfg = argv[++i];
            else if (a == "--output") out = argv[++i];
        }
        MMM m = loadMMMConfig(cfg);
        auto pred = m.predict();
        savePrediction(out, pred);
        std::cout << "Saved to " << out << "\n";
        return 0;
    }
    if (cmd == "forecast") {
        std::string cfg, out;
        int weeks = 0;
        for (int i = 2; i < argc; ++i) {
            std::string a = argv[i];
            if (a == "--config") cfg = argv[++i];
            else if (a == "--weeks") weeks = std::stoi(argv[++i]);
            else if (a == "--output") out = argv[++i];
        }
        MMM m = loadMMMConfig(cfg);
        auto base = m.predict();
        std::vector<double> fcast = base;
        for (int w = 0; w < weeks; w++) {
            size_t t = base.size() + w;
            double val = trend(t, m.t0, m.t1, m.t2) + season(t, m.a, m.b);
            fcast.push_back(val);
        }
        savePrediction(out, fcast);
        std::cout << "Saved to " << out << "\n";
        return 0;
    }
    std::cout << "Unknown command\n";
    return 1;
}
