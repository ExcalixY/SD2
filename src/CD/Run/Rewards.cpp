#include "CD/Run/Rewards.hpp"

#include <format>
#include <vector>

#include "CD/Config.hpp"
#include "CD/Random.hpp"

namespace CD {

std::vector<GoldPart> GetGoldReward(const int rounds, const bool boss, const Config& config, Random& random) {
  std::vector<GoldPart> parts = {{.label = "Win", .gold = random.Int(config.gold.min, config.gold.max)}};

  if (rounds <= config.gold.fastWinRounds) {
    parts.push_back({
      .label = std::format("Fast win ({} turn{})", rounds, rounds == 1 ? "" : "s"),
      .gold = config.gold.fastWinBonus,
    });
  }
  if (boss) {
    parts.push_back({.label = "Boss defeated", .gold = config.gold.bossBonus});
  }

  return parts;
}

}
