"""Write a synthetic 3-year weekly history with known channel effects.

The true parameters go to truth.json, so you can check how close `mmm fit`
gets. Deterministic: same seed, same files. Standard library only.

Usage: python3 make_data.py [--seed 7] [--noise 120]
"""

import argparse
import json
import math
import random
from pathlib import Path

CHANNELS = {
    # name: (beta, half-saturation alpha, slope p, decay, mean weekly spend)
    "tv": (3000, 1500, 2.0, 0.6, 800),
    "search": (2500, 400, 1.2, 0.1, 300),
    "social": (1200, 600, 1.5, 0.3, 450),
}
WEEKS = 156


def adstock(x, decay):
    out, carry = [], 0.0
    for v in x:
        carry = v + carry * decay
        out.append(carry)
    return out


def hill(x, alpha, p):
    return [v**p / (alpha**p + v**p + 1e-9) for v in x]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--seed", type=int, default=7)
    parser.add_argument("--noise", type=float, default=120.0, help="sales noise standard deviation")
    args = parser.parse_args()
    rng = random.Random(args.seed)
    here = Path(__file__).parent

    spend = {}
    for name, (*_, mean) in CHANNELS.items():
        # tv runs in flights: off 4 weeks out of every 10
        spend[name] = [
            0.0 if name == "tv" and t % 10 >= 6 else round(mean * rng.uniform(0.4, 1.6), 2)
            for t in range(WEEKS)
        ]

    contrib = {}
    for name, (beta, alpha, p, decay, _) in CHANNELS.items():
        contrib[name] = [beta * s for s in hill(adstock(spend[name], decay), alpha, p)]

    sales = []
    for t in range(WEEKS):
        base = 10000 + 8 * t
        base += 600 * math.cos(2 * math.pi * t / 52) - 300 * math.sin(2 * math.pi * t / 52)
        base += 150 * math.cos(4 * math.pi * t / 52)
        sales.append(round(base + sum(c[t] for c in contrib.values()) + rng.gauss(0, args.noise), 2))

    with open(here / "history.csv", "w") as f:
        f.write("week,sales," + ",".join(CHANNELS) + "\n")
        for t in range(WEEKS):
            f.write(f"2023-W{t:03d},{sales[t]}," + ",".join(str(spend[n][t]) for n in CHANNELS) + "\n")

    total = sum(sales)
    truth = {
        name: {
            "beta": beta, "alpha": alpha, "p": p, "decay": decay,
            "share": sum(contrib[name]) / total,
            "roi": sum(contrib[name]) / sum(spend[name]),
        }
        for name, (beta, alpha, p, decay, _) in CHANNELS.items()
    }
    (here / "truth.json").write_text(json.dumps(truth, indent=2) + "\n")
    print(f"wrote history.csv ({WEEKS} weeks) and truth.json")


if __name__ == "__main__":
    main()
