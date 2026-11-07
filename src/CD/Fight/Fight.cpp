#include "CD/Fight/Fight.hpp"

#include <algorithm>
#include <cstdint>
#include <format>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "CD/Cards/Card.hpp"
#include "CD/Cards/CardDef.hpp"
#include "CD/Cards/Catalog.hpp"
#include "CD/Config.hpp"
#include "CD/Effects/Effect.hpp"
#include "CD/Effects/Trigger.hpp"
#include "CD/Effects/TriggerContext.hpp"
#include "CD/Fight/DiscardRisk.hpp"
#include "CD/Fight/PlayerState.hpp"
#include "CD/Fight/PokerHands.hpp"
#include "CD/Fight/Scoring.hpp"
#include "CD/Fight/Side.hpp"
#include "CD/Format.hpp"
#include "CD/GameEvents.hpp"
#include "CD/Random.hpp"
#include "CD/Result.hpp"
#include "CD/Run/FightSetup.hpp"
#include "CD/Structures/Containers.hpp"

namespace CD {

Fight::Fight(
  const Catalog& catalog, const Config& config, Random& random, std::vector<GameEvent>& events, FightSetup setup
)
    : m_Catalog(catalog),
      m_Config(config),
      m_Random(random),
      m_Events(events),
      m_Number(setup.number),
      m_Boss(setup.boss),
      m_RandomDiscard(setup.randomDiscard) {
  for (const Side side : SIDES) {
    PlayerSetup& from = side == Side::PLAYER ? setup.player : setup.cpu;
    PlayerState& state = Get(side);

    state.name = std::move(from.name);
    state.isCpu = from.isCpu;
    state.maxHp = from.maxHp;
    state.hp = from.maxHp;
    state.handSize = from.handSize;
    state.fieldSize = from.fieldSize;
    state.areaSize = from.areaSize;
    state.handLevel = from.handLevel;

    std::vector<CardID> deck;
    for (const Card& card : from.cards) {
      m_Cards.emplace(card.id, card);
      deck.push_back(card.id);
    }

    m_Random.Shuffle(deck);
    for (const CardID card : deck) {
      state.deck.push(card);
    }
  }
}

void Fight::Begin() {
  m_Events.emplace_back(
    FightStarted{
      .fight = m_Number,
      .boss = m_Boss,
      .randomDiscard = m_RandomDiscard,
      .cpuName = GetPlayer(Side::CPU).name,
      .playerMaxHp = GetPlayer(Side::PLAYER).maxHp,
      .cpuMaxHp = GetPlayer(Side::CPU).maxHp,
    }
  );
  StartTurn();
}

Result Fight::CanPlay(const CardID card) const {
  if (m_Winner) {
    return Result::Fail("The fight is over");
  }

  const PlayerState& state = GetPlayer(m_Active);
  if (!Contains(state.hand, card)) {
    return Result::Fail("That card isn't in your hand");
  }

  const CardDef& def = GetDef(card);
  if (def.isAttack) {
    if (std::ssize(state.played) >= m_Config.maxPlayedAttacks) {
      return Result::Fail(std::format("Played Cards area is full ({})", m_Config.maxPlayedAttacks));
    }
    if (def.isWild && GetWildsOnField(m_Active) >= m_Config.maxWildsOnField) {
      return Result::Fail(std::format("Max {} Wild cards on the field", m_Config.maxWildsOnField));
    }
    return Result::Ok();
  }

  if (m_SpellsPlayed >= m_Config.maxPlayedSpells) {
    return Result::Fail(std::format("Already played {} buff/debuff cards", m_Config.maxPlayedSpells));
  }
  if (std::cmp_greater_equal(state.effects.size(), state.areaSize)) {
    return Result::Fail(std::format("Buff/Debuff area is full ({})", state.areaSize));
  }

  return Result::Ok();
}

Result Fight::Play(const CardID card) {
  Result can = CanPlay(card);
  if (!can) {
    return can;
  }

  const Side side = m_Active;
  PlayerState& state = Get(side);
  const CardDef& def = GetDef(card);
  Remove(state.hand, card);
  m_PlayedThisTurn.push_back(card);

  if (def.isAttack) {
    state.played.push_back(card);
    m_AttacksPlayed++;
    m_Events.emplace_back(CardPlayed{.side = side, .card = card, .zone = PlayZone::PLAYED});
  } else {
    const Effect effect = {.card = card, .duration = def.duration};
    const bool front = def.trigger == Trigger::PLAY;
    if (front) {
      state.effects.push_front(effect);
    } else {
      state.effects.push_back(effect);
    }

    m_SpellsPlayed++;
    m_Events.emplace_back(CardPlayed{.side = side, .card = card, .zone = PlayZone::AREA, .front = front});

    TriggerContext context;
    Emit(Trigger::PLAY, side, context, card);
  }

  Emit(Trigger::CARD_PLAYED, side);
  return Result::Ok();
}

bool Fight::CanUnplay(const CardID card) const {
  if (m_Winner || std::ranges::find(m_PlayedThisTurn, card) == m_PlayedThisTurn.end()) {
    return false;
  }

  const CardDef& def = GetDef(card);
  return def.isAttack || def.trigger != Trigger::PLAY;
}

Result Fight::Unplay(const CardID card) {
  if (!CanUnplay(card)) {
    return Result::Fail("That card can't be taken back");
  }

  const Side side = m_Active;
  PlayerState& state = Get(side);

  if (GetDef(card).isAttack) {
    std::erase(state.played, card);
    m_AttacksPlayed--;
  } else {
    std::erase_if(state.effects, [card](const Effect& effect) { return effect.card == card; });
    m_SpellsPlayed--;
  }

  std::erase(m_PlayedThisTurn, card);
  state.hand.push_back(card);
  m_Events.emplace_back(CardUnplayed{.side = side, .card = card});
  return Result::Ok();
}

Result Fight::EndTurn() {
  if (m_Winner) {
    return Result::Fail("The fight is over");
  }

  const Side side = m_Active;
  const Side target = GetOpponent(side);
  PlayerState& state = Get(side);

  const std::vector<const Card*> played = GetCards(state.played);
  const std::optional<HandResult> hand = EvaluateHand(played, m_Catalog, m_Config, state.handLevel);
  if (hand) {
    m_Events.emplace_back(HandEvaluated{.side = side, .hand = *hand});
    Emit(Trigger::HAND_EVALUATED, side);
  }

  while (!state.played.empty()) {
    const CardID card = state.played.front();
    state.played.erase(state.played.begin());
    state.field.push(card);
    Emit(Trigger::FIELD_ENTER, side);

    std::vector<CardID> ejected;
    while (std::cmp_greater(state.field.size(), state.fieldSize)) {
      const CardID out = state.field.front();
      state.field.pop();
      state.discard.push_back(out);
      ejected.push_back(out);
      Emit(Trigger::FIELD_EJECT, side);
    }

    m_Events.emplace_back(CardQueued{.side = side, .card = card, .ejected = std::move(ejected)});
  }

  const ScoreBreakdown score = Score(side, hand);
  m_Events.emplace_back(TurnScored{.side = side, .score = score});

  const int combo_before = state.combo;
  ComboResult combo_result = ComboResult::UP;
  Emit(Trigger::COMBO_CHECK, side);
  if (static_cast<double>(score.total) > score.target) {
    state.combo = std::min(m_Config.comboMax, state.combo + 1);
    Emit(Trigger::COMBO_UP, side);
  } else {
    TriggerContext context;
    Emit(Trigger::COMBO_BREAK, side, context);
    combo_result = context.keepCombo ? ComboResult::KEPT : ComboResult::BROKEN;
    if (!context.keepCombo) {
      state.combo = 1;
    }
  }

  state.stats.bestCombo = std::max(state.stats.bestCombo, state.combo);
  m_Events.emplace_back(
    ComboResolved{.side = side, .result = combo_result, .before = combo_before, .after = state.combo}
  );
  Emit(Trigger::FINAL_SCORE, side);

  TriggerContext hit;
  Emit(Trigger::INCOMING_HIT, target, hit);
  const int64_t received = RoundAmount(static_cast<double>(score.total) * hit.hitMult);
  state.lastHit = received;
  state.stats.bestHit = std::max(state.stats.bestHit, received);

  const int64_t hp_before = GetPlayer(target).hp;
  m_Events.emplace_back(
    HitLanded{
      .attacker = side,
      .target = target,
      .amount = received,
      .hitMult = hit.hitMult,
      .hpBefore = hp_before,
      .hpAfter = std::max<int64_t>(0, hp_before - received),
    }
  );
  Damage(target, received);

  if (!m_Winner) {
    Emit(Trigger::AFTER_HIT, target);
  }
  if (!m_Winner) {
    Emit(Trigger::TURN_END, side);
  }

  if (!m_Winner) {
    Tick(target);
  }

  if (!m_Winner && m_RandomDiscard && !state.hand.empty()) {
    RandomDiscard(side);
  }

  if (!m_Winner) {
    m_Active = target;
    StartTurn();
  }

  return Result::Ok();
}

const Config& Fight::GetConfig() const {
  return m_Config;
}

int Fight::GetNumber() const {
  return m_Number;
}

bool Fight::IsBoss() const {
  return m_Boss;
}

bool Fight::HasRandomDiscard() const {
  return m_RandomDiscard;
}

Side Fight::GetActiveSide() const {
  return m_Active;
}

int Fight::GetRound() const {
  return (m_Turn + 1) / 2;
}

bool Fight::IsOver() const {
  return m_Winner.has_value();
}

std::optional<Side> Fight::GetWinner() const {
  return m_Winner;
}

const PlayerState& Fight::GetPlayer(const Side side) const {
  return m_Players[ToIndex(side)];
}

const Card* Fight::GetCard(const CardID card) const {
  const auto it = m_Cards.find(card);
  return it != m_Cards.end() ? &it->second : nullptr;
}

int Fight::GetWildsOnField(const Side side) const {
  const PlayerState& state = GetPlayer(side);
  const auto is_wild = [this](const CardID card) { return GetDef(card).isWild; };
  return static_cast<int>(
    std::ranges::count_if(ToVector(state.field), is_wild) + std::ranges::count_if(state.played, is_wild)
  );
}

bool Fight::HasInArea(const Side side, const std::string_view card_id) const {
  return std::ranges::any_of(GetPlayer(side).effects, [this, card_id](const Effect& effect) {
    return !effect.spent && GetDef(effect.card).id == card_id;
  });
}

ScoreBreakdown Fight::PreviewScore(const Side side) const {
  return PreviewScore(side, GetPlayer(side).played);
}

ScoreBreakdown Fight::PreviewScore(const Side side, std::span<const CardID> played) const {
  const PlayerState& state = GetPlayer(side);

  std::vector<CardID> field = ToVector(state.field);
  field.insert(field.end(), played.begin(), played.end());
  if (std::ssize(field) > state.fieldSize) {
    field.erase(field.begin(), field.end() - state.fieldSize);
  }

  const std::vector<const Card*> played_cards = GetCards(played);
  const std::vector<const Card*> field_cards = GetCards(field);
  ScoreBreakdown score =
    ScoreBase(field_cards, EvaluateHand(played_cards, m_Catalog, m_Config, state.handLevel), m_Config);
  PreviewCalculate(side, score, field_cards);
  ScoreFinish(score, state.combo, m_Config);
  return score;
}

DiscardRisk Fight::GetDiscardRisk(const Side side) const {
  return CD::GetDiscardRisk(static_cast<int>(GetPlayer(side).hand.size()), m_RandomDiscard);
}

void Fight::StartTurn() {
  const Side side = m_Active;
  PlayerState& state = Get(side);

  m_Turn++;
  state.played.clear();
  m_AttacksPlayed = 0;
  m_SpellsPlayed = 0;
  m_PlayedThisTurn.clear();
  m_Events.emplace_back(TurnStarted{.side = side, .round = GetRound()});

  Emit(Trigger::TURN_START, side);
  TriggerContext context;
  Emit(Trigger::DRAW, side, context);
  const int room = state.handSize - static_cast<int>(state.hand.size());
  Draw(side, std::min(room, context.drawLimit));
  Emit(Trigger::AFTER_DRAW, side);
}

int Fight::Draw(const Side side, const int count) {
  PlayerState& state = Get(side);
  int drawn = 0;

  for (int i = 0; i < count && std::cmp_less(state.hand.size(), state.handSize); i++) {
    if (state.deck.empty()) {
      if (state.discard.empty()) {
        break;
      }

      std::vector<CardID> pile = std::exchange(state.discard, {});
      m_Random.Shuffle(pile);
      for (const CardID card : pile) {
        state.deck.push(card);
      }

      m_Events.emplace_back(DeckRefilled{.side = side, .count = static_cast<int>(pile.size())});
      Emit(Trigger::DECK_REFILL, side);
    }

    const CardID card = state.deck.front();
    state.deck.pop();
    state.hand.push_back(card);
    m_Events.emplace_back(CardDrawn{.side = side, .card = card});
    drawn++;
  }

  return drawn;
}

ScoreBreakdown Fight::Score(const Side side, const std::optional<HandResult>& hand) {
  const PlayerState& state = GetPlayer(side);
  const std::vector<CardID> field = ToVector(state.field);
  ScoreBreakdown score = ScoreBase(GetCards(field), hand, m_Config);

  TriggerContext context = {.damage = score.damage, .mult = score.mult};
  Emit(Trigger::CALCULATE, side, context);
  score.damage = context.damage;
  score.mult = context.mult;
  score.notes = std::move(context.notes);

  ScoreFinish(score, state.combo, m_Config);
  return score;
}

void Fight::Tick(const Side side) {
  PlayerState& state = Get(side);
  std::vector<CardID> expired;

  for (size_t i = state.effects.size(); i > 0; i--) {
    Effect effect = state.effects.front();
    state.effects.pop_front();
    Emit(Trigger::EFFECT_TICK, side);

    if (effect.spent) {
      expired.push_back(effect.card);
      continue;
    }
    if (effect.duration == -1) {
      state.effects.push_back(effect);
      continue;
    }

    effect.duration--;
    if (effect.duration > 0) {
      state.effects.push_back(effect);
    } else {
      expired.push_back(effect.card);
    }
  }

  for (const CardID card : expired) {
    state.discard.push_back(card);
    m_Events.emplace_back(EffectExpired{.side = side, .card = card});
    Emit(Trigger::EFFECT_EXPIRED, side);
  }
}

void Fight::RandomDiscard(const Side side) {
  PlayerState& state = Get(side);
  const int steps = m_Random.Int(m_Config.discardWalkMin, m_Config.discardWalkMax);
  std::vector<CardID> hand_before = ToVector(state.hand);

  auto it = state.hand.head();
  for (int i = 0; i < steps; i++) {
    ++it;
  }

  const CardID card = *it;
  state.hand.erase(it);
  state.discard.push_back(card);

  const auto index = std::ranges::find(hand_before, card) - hand_before.begin();
  m_Events.emplace_back(
    CardDiscarded{
      .side = side,
      .steps = steps,
      .handBefore = std::move(hand_before),
      .index = static_cast<int>(index),
      .card = card,
    }
  );
  Emit(Trigger::RANDOM_DISCARD, side);
}

PlayerState& Fight::Get(const Side side) {
  return m_Players[ToIndex(side)];
}

const CardDef& Fight::GetDef(const CardID card) const {
  return *m_Cards.at(card).def;
}

std::vector<const Card*> Fight::GetCards(std::span<const CardID> ids) const {
  std::vector<const Card*> cards;
  cards.reserve(ids.size());

  for (const CardID id : ids) {
    cards.push_back(&m_Cards.at(id));
  }

  return cards;
}

}
