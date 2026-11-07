#pragma once

#include <string>

#include "CD/Commands.hpp"

namespace CD {

class Fight;

}

namespace CD::Cpu {

struct Decision {
  Command command;
  std::string rule;
};

[[nodiscard]]
Decision Decide(const Fight& fight);

}
