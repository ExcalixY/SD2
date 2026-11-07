#pragma once

#include <vector>

#include "CD/Cards/Card.hpp"
#include "CD/Cards/CardKind.hpp"
#include "CD/Run/Upgrade.hpp"

namespace CD {

class CardFactory;
class Catalog;
class Random;
struct Config;
struct Run;

struct ShopCard {
  Card card;
  int price = 0;
  bool sold = false;
};

struct TokenPack {
  Token token = Token::YELLOW;
  int price = 0;
  bool sold = false;
};

struct Shop {
  std::vector<ShopCard> cards;
  TokenPack tokenPack;
};

[[nodiscard]]
Shop RollShop(const Catalog& catalog, const Config& config, CardFactory& factory, Random& random);

[[nodiscard]]
int GetUpgradePrice(const Run& run, Upgrade upgrade, const Config& config);

}
