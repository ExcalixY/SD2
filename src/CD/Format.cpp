#include "CD/Format.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>

namespace CD {

int64_t RoundAmount(const double value) {
  return static_cast<int64_t>(std::floor(value + 0.5));
}

std::string FormatAmount(const int64_t value) {
  const std::string digits = std::format("{}", value < 0 ? -value : value);
  std::string out = value < 0 ? "-" : "";

  for (size_t i = 0; i < digits.size(); i++) {
    if (i > 0 && (digits.size() - i) % 3 == 0) {
      out += ',';
    }
    out += digits[i];
  }

  return out;
}

std::string FormatFactor(const double value) {
  return std::format("{}", value);
}

}
