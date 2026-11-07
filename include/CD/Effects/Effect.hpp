#pragma once

#include "CD/Cards/Card.hpp"

namespace CD {

struct Effect {
  CardID card = NO_CARD;
  int duration = 1;
  bool triggered = false;
  bool spent = false;
};

}
