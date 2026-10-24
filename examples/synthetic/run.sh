#!/bin/bash
# Fit on synthetic data with known truth, optimize, forecast, then grade.
set -euo pipefail
cd "$(dirname "$0")"
MMM=../../build/mmm
python3 make_data.py
$MMM fit --data history.csv --output model.json
$MMM optimize --config model.json --output plan.json
$MMM forecast --config model.json --weeks 13 --plan plan.json --output forecast.json
python3 grade.py
