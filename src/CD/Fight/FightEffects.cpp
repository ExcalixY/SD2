#include <algorithm>
#include <cstdint>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "CD/Cards/Card.hpp"
#include "CD/Cards/CardDef.hpp"
#include "CD/Config.hpp"
#include "CD/Effects/Action.hpp"
#include "CD/Effects/Effect.hpp"
#include "CD/Effects/Trigger.hpp"
#include "CD/Effects/TriggerContext.hpp"
#include "CD/Fight/Fight.hpp"
#include "CD/Fight/PlayerState.hpp"
#include "CD/Fight/Scoring.hpp"
#include "CD/Fight/Side.hpp"
#include "CD/Format.hpp"
#include "CD/GameEvents.hpp"
#include "CD/Structures/Containers.hpp"

namespace CD {

namespace {

struct Source {
  CardID card = NO_CARD;
  bool onField = false;
};

}

void Fight::Emit(const Trigger trigger, const Side subject, TriggerContext& context, const CardID only) {
  for (const Side owner : {m_Active, GetOpponent(m_Active)}) {
    PlayerState& state = Get(owner);

    std::vector<Source> sources;
    for (const Effect& effect : state.effects) {
      sources.push_back({.card = effect.card});
    }
    for (const CardID card : ToVector(state.field)) {
      if (GetDef(card).trigger) {
        sources.push_back({.card = card, .onField = true});
      }
    }

    for (const Source& source : sources) {
      if (only != NO_CARD && source.card != only) {
        continue;
      }

      const auto find_effect = [&state, &source]() -> Effect* {
        const auto it = std::ranges::find(state.effects, source.card, &Effect::card);
        return it != state.effects.end() ? &*it : nullptr;
      };

      const Effect* effect = source.onField ? nullptr : find_effect();
      if (!source.onField && (!effect || effect->spent)) {
        continue;
      }

      const CardDef& def = GetDef(source.card);
      const bool keep_only = trigger == Trigger::COMBO_BREAK && def.trigger != Trigger::COMBO_BREAK;
      if (def.trigger != trigger && !keep_only) {
        continue;
      }

      bool fired = false;
      for (const ActionSpec& spec : def.actions) {
        if (keep_only && spec.action != Action::KEEP_COMBO) {
          continue;
        }

        const Side opponent = GetOpponent(owner);
        const bool any = trigger == Trigger::PLAY;
        if (spec.player && (any || subject == owner)) {
          ApplyAction(spec, trigger, owner, *spec.player, context, source.card, owner);
          fired = true;
        }
        if (spec.opponent && (any || subject == opponent)) {
          ApplyAction(spec, trigger, opponent, *spec.opponent, context, source.card, owner);
          fired = true;
        }
      }

      if (Effect* fired_effect = fired && !source.onField ? find_effect() : nullptr) {
        fired_effect->triggered = true;
        if (fired_effect->duration == -1) {
          fired_effect->spent = true;
        }
      }

      if (m_Winner) {
        return;
      }
    }
  }
}

void Fight::Emit(const Trigger trigger, const Side subject) {
  TriggerContext context;
  Emit(trigger, subject, context);
}

void Fight::ApplyAction(
  const ActionSpec& spec,
  const Trigger trigger,
  const Side target,
  const double value,
  TriggerContext& context,
  const CardID card,
  const Side owner
) {
  PlayerState& state = Get(target);
  const CardDef& def = GetDef(card);
  const std::string who = std::format("{}'s {}", GetPlayer(owner).name, def.name);

  EffectTriggered event = {
    .owner = owner,
    .card = card,
    .trigger = trigger,
    .action = spec.action,
    .target = target,
    .value = value,
  };

  switch (spec.action) {
    case Action::HEAL: {
      const int64_t before = state.hp;
      state.hp = std::min(state.maxHp, state.hp + RoundAmount(value));
      event.amount = state.hp - before;
      event.hp = state.hp;
      event.note = std::format("{}: {} heals {}", who, state.name, FormatAmount(event.amount));
      m_Events.emplace_back(std::move(event));
      Emit(Trigger::HEAL, target);
      return;
    }
    case Action::MULTIPLY_MULT:
      context.mult *= value;
      context.notes.push_back(std::format("{} x{} mult", def.name, FormatFactor(value)));
      event.note = std::format("{}: mult x{}", who, FormatFactor(value));
      break;
    case Action::MULTIPLY_DAMAGE:
      context.damage *= value;
      context.notes.push_back(std::format("{} x{} dmg", def.name, FormatFactor(value)));
      event.note = std::format("{}: damage x{}", who, FormatFactor(value));
      break;
    case Action::ADD_DAMAGE_PER_COMBO: {
      const double added = value * state.combo;
      context.damage += added;
      context.notes.push_back(std::format("{} +{} dmg", def.name, FormatAmount(RoundAmount(added))));
      event.note = std::format("{}: +{} damage", who, FormatAmount(RoundAmount(added)));
      break;
    }
    case Action::KEEP_COMBO:
      context.keepCombo = true;
      event.note = std::format("{}: {}'s combo is kept", who, state.name);
      break;
    case Action::DRAW_CARDS:
      event.note = std::format("{}: {} draws {}", who, state.name, RoundAmount(value));
      m_Events.emplace_back(std::move(event));
      Draw(target, static_cast<int>(RoundAmount(value)));
      return;
    case Action::ADD_COMBO:
      state.combo = std::min(m_Config.comboMax, state.combo + static_cast<int>(RoundAmount(value)));
      event.combo = state.combo;
      event.note = std::format("{}: {}'s combo is now {}x", who, state.name, state.combo);
      break;
    case Action::MULTIPLY_HIT:
      context.hitMult *= value;
      event.note = std::format("{}: hit x{}", who, FormatFactor(value));
      break;
    case Action::DEAL_DAMAGE: {
      const int64_t amount = RoundAmount(value);
      event.amount = amount;
      event.hp = std::max<int64_t>(0, state.hp - amount);
      event.note = std::format("{}: {} takes {}", who, state.name, FormatAmount(amount));
      m_Events.emplace_back(std::move(event));
      Damage(target, amount);
      return;
    }
    case Action::LIMIT_DRAW:
      context.drawLimit = std::min(context.drawLimit, static_cast<int>(RoundAmount(value)));
      event.note = std::format("{}: {} draws only {}", who, state.name, RoundAmount(value));
      break;
    case Action::DESTROY_FIELD:
      event.cards = Drain(state.field);
      state.discard.insert(state.discard.end(), event.cards.begin(), event.cards.end());
      event.note = std::format("{}: destroys {} card(s) on {}'s field", who, event.cards.size(), state.name);
      break;
    case Action::DESTROY_EFFECTS: {
      for (const Effect& effect : state.effects) {
        if (effect.card != card) {
          event.cards.push_back(effect.card);
        }
      }
      std::erase_if(state.effects, [card](const Effect& effect) { return effect.card != card; });

      state.discard.insert(state.discard.end(), event.cards.begin(), event.cards.end());
      event.note = std::format("{}: destroys {} effect(s) of {}", who, event.cards.size(), state.name);
      const size_t destroyed = event.cards.size();
      m_Events.emplace_back(std::move(event));
      for (size_t i = 0; i < destroyed; i++) {
        Emit(Trigger::EFFECT_DESTROYED, target);
      }
      return;
    }
    case Action::SET_HP_PERCENT:
      state.hp = RoundAmount(static_cast<double>(state.maxHp) * value);
      context.saved = true;
      event.hp = state.hp;
      event.note = std::format("{}: {} survives with {} HP!", who, state.name, FormatAmount(state.hp));
      break;
  }

  m_Events.emplace_back(std::move(event));
}

void Fight::Damage(const Side target, const int64_t amount) {
  PlayerState& state = Get(target);
  state.hp -= amount;
  if (state.hp > 0) {
    return;
  }

  TriggerContext context;
  Emit(Trigger::LETHAL, target, context);
  if (!context.saved && state.hp <= 0) {
    state.hp = 0;
    m_Winner = GetOpponent(target);
  }
}

void Fight::PreviewCalculate(const Side side, ScoreBreakdown& score, std::span<const Card* const> field) const {
  const auto apply = [this, side, &score](const Side owner, const CardDef& def) {
    if (def.trigger != Trigger::CALCULATE) {
      return;
    }

    for (const ActionSpec& spec : def.actions) {
      const std::optional<double> value = owner == side ? spec.player : spec.opponent;
      if (!value) {
        continue;
      }

      if (spec.action == Action::MULTIPLY_MULT) {
        score.mult *= *value;
        score.notes.push_back(std::format("{} x{} mult", def.name, FormatFactor(*value)));
      } else if (spec.action == Action::MULTIPLY_DAMAGE) {
        score.damage *= *value;
        score.notes.push_back(std::format("{} x{} dmg", def.name, FormatFactor(*value)));
      } else if (spec.action == Action::ADD_DAMAGE_PER_COMBO) {
        const double added = *value * GetPlayer(side).combo;
        score.damage += added;
        score.notes.push_back(std::format("{} +{} dmg", def.name, FormatAmount(RoundAmount(added))));
      }
    }
  };

  for (const Side owner : {side, GetOpponent(side)}) {
    for (const Effect& effect : GetPlayer(owner).effects) {
      if (!effect.spent) {
        apply(owner, GetDef(effect.card));
      }
    }
  }

  for (const Card* card : field) {
    if (card->def->trigger) {
      apply(side, *card->def);
    }
  }
}

}
