# Changelog

## v0.2.0

### Added
- `mmm fit`: learns adstock decay, Hill half-saturation and slope per channel (coordinate grid search), with trend, seasonality and non-negative channel betas by least squares. Scores the last `--holdout` weeks on a model that never saw them.
- `mmm optimize`: budget reallocation by greedy marginal return plus pairwise exchange, each channel within `--bound` of current spend. Reports lift and marginal ROI.
- CSV input with line-numbered errors.
- `examples/synthetic`: 3 years of data with known answers, `run.sh`, `grade.py`.
- Test suite (`mmm_tests`): known-truth recovery, determinism, optimizer vs brute force, decision quality on the true curves.
- CI on every push and pull request; releases (tag push or the "Release" workflow's Run workflow button) ship Linux x86-64 and arm64 binaries and a Docker image on GHCR.
- `MMM_NATIVE` CMake option (default ON); release builds turn it off for portable binaries.

### Changed
- `mmm forecast` future weeks include media driven by a spend plan (`--plan`), or current spend by default. Previously only trend and seasonality.

### Removed
- The placeholder budget optimizer that drifted toward an equal split.

## v0.1.0

- Deterministic `run` and `forecast` from a hand-written JSON config.
