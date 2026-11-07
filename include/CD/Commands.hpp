#pragma once

#include <string>
#include <variant>

#include "CD/Cards/Card.hpp"
#include "CD/Run/Run.hpp"
#include "CD/Run/Upgrade.hpp"

namespace CD {

struct Command_StartRun {
  std::string deckId;
  RunOptions options;
};

struct Command_AbandonRun { };

struct Command_PlayCard {
  CardID card = NO_CARD;
};

struct Command_UnplayCard {
  CardID card = NO_CARD;
};

struct Command_EndTurn { };

struct Command_EnterShop { };

struct Command_BuyCard {
  int slot = 0;
};

struct Command_BuyTokenPack {
  CardID target = NO_CARD;
};

struct Command_BuyUpgrade {
  Upgrade upgrade = Upgrade::HAND;
};

struct Command_NextFight { };

using Command = std::variant<
  Command_StartRun,
  Command_AbandonRun,
  Command_PlayCard,
  Command_UnplayCard,
  Command_EndTurn,
  Command_EnterShop,
  Command_BuyCard,
  Command_BuyTokenPack,
  Command_BuyUpgrade,
  Command_NextFight>;

}
