#include "CD/Cpu/Cpu.hpp"

#include <format>
#include <optional>

#include "CD/Commands.hpp"
#include "CD/Cpu/CpuContext.hpp"
#include "CD/Cpu/CpuRules.hpp"
#include "CD/Fight/Fight.hpp"

namespace CD::Cpu {

Decision Decide(const Fight& fight) {
  if (fight.IsOver()) {
    return {.command = Command_EndTurn{}, .rule = "The fight is over"};
  }

  const Context context(fight);
  for (const Section& section : GetTree()) {
    for (const Rule& rule : section.rules) {
      if (const std::optional<Choice> choice = rule(context)) {
        return {
          .command = Command_PlayCard{.card = choice->card},
          .rule = std::format("{}: {}", section.name, choice->why),
        };
      }
    }
  }

  return {.command = Command_EndTurn{}, .rule = "Nothing to play: end the turn"};
}

}
