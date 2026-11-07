#pragma once

#include <cstdint>

namespace CD {

struct Config;

[[nodiscard]]
int64_t GetComboDamage(int level);

[[nodiscard]]
double GetComboGrowth(int level, const Config& config);

[[nodiscard]]
double GetComboTarget(int level, const Config& config);

}
