#include "CD/Shop/Shop.hpp"

#include <cmath>
#include <utility>
#include <vector>

#include "CD/Cards/Card.hpp"
#include "CD/Cards/CardDef.hpp"
#include "CD/Cards/CardFactory.hpp"
#include "CD/Cards/CardKind.hpp"
#include "CD/Cards/Catalog.hpp"
#include "CD/Config.hpp"
#include "CD/Format.hpp"
#include "CD/Random.hpp"
#include "CD/Run/Run.hpp"
#include "CD/Run/Upgrade.hpp"

namespace CD {

Shop RollShop(const Catalog& catalog, const Config& config, CardFactory& factory, Random& random) {
  const std::vector<const CardDef*> basic = catalog.GetBasicCards();
  const std::vector<const CardDef*> wilds = catalog.GetWildCards();
  Shop shop;

  for (int i = 0; i < config.shop.cardSlots; i++) {
    const CardDef* def = random.Chance(config.shop.wildChance) ? random.Pick(wilds) : random.Pick(basic);
    Card card = factory.Make(*def, random);
    if (def->isAttack && !def->isWild && random.Chance(config.shop.tokenChance)) {
      CardFactory::AddRandomToken(card, random);
    }

    const int price = GetCardCost(card, config);
    shop.cards.push_back({.card = std::move(card), .price = price});
  }

  shop.tokenPack = {.token = random.Pick(TOKENS), .price = config.shop.tokenPack};
  return shop;
}

int GetUpgradePrice(const Run& run, const Upgrade upgrade, const Config& config) {
  const int base = config.shop.upgrades[std::to_underlying(upgrade)].basePrice;
  return static_cast<int>(RoundAmount(base * std::pow(config.shop.upgradeGrowth, run.GetUpgrade(upgrade))));
}

}
