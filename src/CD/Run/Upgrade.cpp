#include "CD/Run/Upgrade.hpp"

#include <optional>
#include <string_view>

namespace CD {

std::string_view ToKey(const Upgrade upgrade) {
  switch (upgrade) {
    case Upgrade::HAND:
      return "hand";
    case Upgrade::FIELD:
      return "field";
    case Upgrade::AREA:
      return "area";
    case Upgrade::HP:
      return "hp";
    case Upgrade::HANDS:
      return "hands";
  }

  return "?";
}

std::optional<Upgrade> UpgradeFromKey(const std::string_view key) {
  for (const Upgrade upgrade : UPGRADES) {
    if (ToKey(upgrade) == key) {
      return upgrade;
    }
  }

  return std::nullopt;
}

}
