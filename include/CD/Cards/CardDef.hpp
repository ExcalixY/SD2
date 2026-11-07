#pragma once

#include <optional>
#include <string>
#include <vector>

#include "CD/Cards/CardKind.hpp"
#include "CD/Effects/Action.hpp"
#include "CD/Effects/Trigger.hpp"

namespace CD {

struct CardDef {
  std::string id;
  std::string name;
  std::string description;
  std::string iconSprite;

  CardKind kind = CardKind::DAMAGE;
  bool isWild = false;
  bool isAttack = false;

  int damage = 0;
  int mult = 0;

  int duration = 1;
  std::optional<Trigger> trigger;
  std::vector<ActionSpec> actions;

  int maxCopies = 3;
  int strength = 3;
  int cost = 3;
};

}
