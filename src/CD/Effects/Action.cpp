#include "CD/Effects/Action.hpp"

#include <array>
#include <optional>
#include <string_view>
#include <utility>

namespace CD {

namespace {

constexpr std::array<std::pair<Action, std::string_view>, 13> KEYS = {{
  {Action::HEAL, "heal"},
  {Action::MULTIPLY_MULT, "multiplyMult"},
  {Action::MULTIPLY_DAMAGE, "multiplyDamage"},
  {Action::ADD_DAMAGE_PER_COMBO, "addDamagePerCombo"},
  {Action::KEEP_COMBO, "keepCombo"},
  {Action::DRAW_CARDS, "drawCards"},
  {Action::ADD_COMBO, "addCombo"},
  {Action::MULTIPLY_HIT, "multiplyHit"},
  {Action::DEAL_DAMAGE, "dealDamage"},
  {Action::LIMIT_DRAW, "limitDraw"},
  {Action::DESTROY_FIELD, "destroyField"},
  {Action::DESTROY_EFFECTS, "destroyEffects"},
  {Action::SET_HP_PERCENT, "setHpPercent"},
}};

}

std::string_view ToKey(const Action action) {
  for (const auto& [value, key] : KEYS) {
    if (value == action) {
      return key;
    }
  }

  return "?";
}

std::optional<Action> ActionFromKey(const std::string_view key) {
  for (const auto& [value, name] : KEYS) {
    if (name == key) {
      return value;
    }
  }

  return std::nullopt;
}

}
