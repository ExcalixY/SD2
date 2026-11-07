#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "CD/Cards/Card.hpp"

namespace CD::Cpu {

class Context;

struct Choice {
  CardID card = NO_CARD;
  std::string why;
};

using Rule = std::function<std::optional<Choice>(const Context&)>;

struct Section {
  std::string_view name;
  std::vector<Rule> rules;
};

[[nodiscard]]
const std::vector<Section>& GetTree();

}
