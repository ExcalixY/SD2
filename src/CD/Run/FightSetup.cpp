#include "CD/Run/FightSetup.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

#include "CD/Cards/CardFactory.hpp"
#include "CD/Cards/Catalog.hpp"
#include "CD/Config.hpp"
#include "CD/Deck/DeckGenerator.hpp"
#include "CD/Format.hpp"
#include "CD/Random.hpp"
#include "CD/Run/Run.hpp"
#include "CD/Run/Upgrade.hpp"

namespace CD {

namespace {

int Grow(const int level, const CpuGrowth& growth) {
  return std::min(growth.max, level / growth.every);
}

}

bool IsBossFight(const int fight, const Config& config) {
  return fight % config.bossEvery == 0;
}

std::optional<FightSetup> MakeFightSetup(
  const Run& run, const Catalog& catalog, const Config& config, CardFactory& factory, Random& random
) {
  const bool boss = IsBossFight(run.fight, config);

  std::optional<GeneratedDeck> cpu_deck = GenerateCpuDeck(run.cards, boss, catalog, config, factory, random);
  if (!cpu_deck) {
    return std::nullopt;
  }

  const int level = (run.fight - 1) / config.cpuLevelEvery;
  const double cpu_hp =
    static_cast<double>(config.cpuHp) * std::pow(config.cpuHpGrowth, run.fight - 1) * (boss ? config.bossHpMult : 1.);

  return FightSetup{
    .number = run.fight,
    .boss = boss,
    .randomDiscard = run.options.randomDiscard || boss,
    .player =
      {
        .name = "Player",
        .cards = run.cards,
        .maxHp =
          RoundAmount(static_cast<double>(config.playerHp) * (1. + config.hpUpgrade * run.GetUpgrade(Upgrade::HP))),
        .handSize = config.handSize + run.GetUpgrade(Upgrade::HAND),
        .fieldSize = config.fieldSize + run.GetUpgrade(Upgrade::FIELD),
        .areaSize = config.areaSize + run.GetUpgrade(Upgrade::AREA),
        .handLevel = 1 + run.GetUpgrade(Upgrade::HANDS),
      },
    .cpu = {
      .name = boss ? "Boss" : "CPU",
      .isCpu = true,
      .cards = std::move(cpu_deck->cards),
      .maxHp = RoundAmount(cpu_hp),
      .handSize = config.handSize + Grow(level, config.cpuHandGrowth),
      .fieldSize = config.fieldSize + Grow(level, config.cpuFieldGrowth),
      .areaSize = config.areaSize + Grow(level, config.cpuAreaGrowth),
      .handLevel = 1 + level,
    },
  };
}

}
