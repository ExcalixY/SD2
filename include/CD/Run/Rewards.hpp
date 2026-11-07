#pragma once

#include <string>
#include <vector>

namespace CD {

class Random;
struct Config;

struct GoldPart {
  std::string label;
  int gold = 0;
};

[[nodiscard]]
std::vector<GoldPart> GetGoldReward(int rounds, bool boss, const Config& config, Random& random);

}
