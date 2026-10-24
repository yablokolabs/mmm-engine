"""Grade a fitted model and its budget plan against the known truth.

Run after: make_data.py, mmm fit, mmm optimize (see run.sh).
Total ROI of always-on channels is hard to separate from baseline demand in
any MMM; marginal ROI and the quality of the budget decision are what the
data can pin down, so those are graded here.
"""

import json
from pathlib import Path

here = Path(__file__).parent
truth = json.loads((here / "truth.json").read_text())
model = json.loads((here / "model.json").read_text())
plan = json.loads((here / "plan.json").read_text())


def response(p, x):
    level = x / (1 - p["decay"])
    return p["beta"] * level ** p["p"] / (p["alpha"] ** p["p"] + level ** p["p"])


def marginal(p, x, h=1e-3):
    return (response(p, x + h) - response(p, x - h)) / (2 * h)


names = [c["name"] for c in model["channels"]]
fitted = {c["name"]: c["params"] for c in model["channels"]}
current = {c["name"]: c["current_weekly_spend"] for c in plan["channels"]}
optimal = {c["name"]: c["optimal_weekly_spend"] for c in plan["channels"]}
lower = {c["name"]: c["lower_bound"] for c in plan["channels"]}
upper = {c["name"]: c["upper_bound"] for c in plan["channels"]}

print("channel  mROI true  mROI fitted")
for n in names:
    print(f"{n:8s} {marginal(truth[n], current[n]):9.2f}  {marginal(fitted[n], current[n]):11.2f}")

# Best achievable allocation under the TRUE curves, same budget and bounds.
budget = sum(current.values())
a, b, c = names
best, step = 0.0, budget / 600
x0 = lower[a]
while x0 <= upper[a]:
    x1 = lower[b]
    while x1 <= upper[b]:
        x2 = budget - x0 - x1
        if lower[c] <= x2 <= upper[c]:
            best = max(best, response(truth[a], x0) + response(truth[b], x1) + response(truth[c], x2))
        x1 += step
    x0 += step

now = sum(response(truth[n], current[n]) for n in names)
with_plan = sum(response(truth[n], optimal[n]) for n in names)
print(f"\nTrue weekly media sales: current {now:.0f}, fitted plan {with_plan:.0f}, best possible {best:.0f}")
print(f"Fitted plan captures {(with_plan - now) / (best - now) * 100:.0f}% of the achievable lift")
