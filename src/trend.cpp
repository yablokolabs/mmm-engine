#include "trend.hpp"

double trend(double t, double t0, double t1, double t2) {
  return t0 + t1 * t + t2 * t * t;
}
