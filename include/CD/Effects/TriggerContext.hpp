#pragma once

#include <limits>
#include <string>
#include <vector>

namespace CD {

struct TriggerContext {
  double damage = 0.;
  double mult = 1.;
  std::vector<std::string> notes;
  double hitMult = 1.;
  int drawLimit = std::numeric_limits<int>::max();
  bool keepCombo = false;
  bool saved = false;
};

}
