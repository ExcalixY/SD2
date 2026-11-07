#include "CD/Fight/Combo.hpp"

#include <cstdint>
#include <utility>

#include "CD/Config.hpp"

namespace CD {

int64_t GetComboDamage(const int level) {
  if (level <= 1) {
    return 100;
  }

  int64_t previous = 100;
  int64_t current = 200;
  for (int i = 2; i < level; i++) {
    previous = std::exchange(current, previous + current);
  }

  return current;
}

double GetComboGrowth(const int level, const Config& config) {
  for (const ComboThreshold& threshold : config.comboThresholds) {
    if (level <= threshold.upToLevel) {
      return threshold.growth;
    }
  }

  return config.comboThresholds.empty() ? 0. : config.comboThresholds.back().growth;
}

double GetComboTarget(const int level, const Config& config) {
  return static_cast<double>(GetComboDamage(level)) * (1. + GetComboGrowth(level, config));
}

}
