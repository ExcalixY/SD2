#pragma once

#include <cstdint>
#include <string>

namespace CD {

[[nodiscard]]
int64_t RoundAmount(double value);

[[nodiscard]]
std::string FormatAmount(int64_t value);

[[nodiscard]]
std::string FormatFactor(double value);

}
