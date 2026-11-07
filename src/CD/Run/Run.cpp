#include "CD/Run/Run.hpp"

#include <utility>

#include "CD/Run/Upgrade.hpp"

namespace CD {

int Run::GetUpgrade(const Upgrade upgrade) const {
  return upgrades[std::to_underlying(upgrade)];
}

}
