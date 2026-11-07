#pragma once

#include <string>

namespace CD {

struct DeckBalance {
  bool random = false;
  bool allowWildRandom = false;
  int damage = 0;
  int mult = 0;
  int buff = 0;
  int debuff = 0;
  int wildDamage = 0;
  int wildMagic = 0;
};

struct DeckDef {
  std::string id;
  std::string name;
  std::string description;
  std::string sprite;
  DeckBalance balance;
};

}
