#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

namespace CD {

enum class Upgrade {
  HAND,
  FIELD,
  AREA,
  HP,
  HANDS,
};

inline constexpr size_t UPGRADE_COUNT = 5;
inline constexpr std::array UPGRADES = {Upgrade::HAND, Upgrade::FIELD, Upgrade::AREA, Upgrade::HP, Upgrade::HANDS};

[[nodiscard]]
std::string_view ToKey(Upgrade upgrade);
[[nodiscard]]
std::optional<Upgrade> UpgradeFromKey(std::string_view key);

}
