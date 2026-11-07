#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "CD/Cards/Card.hpp"
#include "CD/Effects/Action.hpp"
#include "CD/Effects/Trigger.hpp"
#include "CD/Effects/TriggerContext.hpp"
#include "CD/Fight/DiscardRisk.hpp"
#include "CD/Fight/PlayerState.hpp"
#include "CD/Fight/PokerHands.hpp"
#include "CD/Fight/Scoring.hpp"
#include "CD/Fight/Side.hpp"
#include "CD/GameEvents.hpp"
#include "CD/Result.hpp"
#include "CD/Run/FightSetup.hpp"

namespace CD {

class Catalog;
class Random;
struct Config;

class Fight final {
public:
  Fight(const Catalog& catalog, const Config& config, Random& random, std::vector<GameEvent>& events, FightSetup setup);

  Fight(const Fight&) = delete;
  Fight& operator=(const Fight&) = delete;
  Fight(Fight&&) = delete;
  Fight& operator=(Fight&&) = delete;
  ~Fight() = default;

  void Begin();

  [[nodiscard]]
  Result CanPlay(CardID card) const;
  Result Play(CardID card);
  [[nodiscard]]
  bool CanUnplay(CardID card) const;
  Result Unplay(CardID card);
  Result EndTurn();

  [[nodiscard]]
  const Config& GetConfig() const;
  [[nodiscard]]
  int GetNumber() const;
  [[nodiscard]]
  bool IsBoss() const;
  [[nodiscard]]
  bool HasRandomDiscard() const;
  [[nodiscard]]
  Side GetActiveSide() const;
  [[nodiscard]]
  int GetRound() const;
  [[nodiscard]]
  bool IsOver() const;
  [[nodiscard]]
  std::optional<Side> GetWinner() const;

  [[nodiscard]]
  const PlayerState& GetPlayer(Side side) const;
  [[nodiscard]]
  const Card* GetCard(CardID card) const;
  [[nodiscard]]
  int GetWildsOnField(Side side) const;
  [[nodiscard]]
  bool HasInArea(Side side, std::string_view card_id) const;

  [[nodiscard]]
  ScoreBreakdown PreviewScore(Side side) const;
  [[nodiscard]]
  ScoreBreakdown PreviewScore(Side side, std::span<const CardID> played) const;
  [[nodiscard]]
  DiscardRisk GetDiscardRisk(Side side) const;

private:
  void Emit(Trigger trigger, Side subject, TriggerContext& context, CardID only = NO_CARD);
  void Emit(Trigger trigger, Side subject);
  void ApplyAction(
    const ActionSpec& spec, Trigger trigger, Side target, double value, TriggerContext& context, CardID card, Side owner
  );
  void Damage(Side target, int64_t amount);
  void PreviewCalculate(Side side, ScoreBreakdown& score, std::span<const Card* const> field) const;

  void StartTurn();
  int Draw(Side side, int count);
  ScoreBreakdown Score(Side side, const std::optional<HandResult>& hand);
  void Tick(Side side);
  void RandomDiscard(Side side);

  [[nodiscard]]
  PlayerState& Get(Side side);
  [[nodiscard]]
  const CardDef& GetDef(CardID card) const;
  [[nodiscard]]
  std::vector<const Card*> GetCards(std::span<const CardID> ids) const;

  const Catalog& m_Catalog;
  const Config& m_Config;
  Random& m_Random;
  std::vector<GameEvent>& m_Events;

  std::unordered_map<CardID, Card> m_Cards;
  std::array<PlayerState, 2> m_Players;
  int m_Number = 1;
  bool m_Boss = false;
  bool m_RandomDiscard = false;

  Side m_Active = Side::PLAYER;
  int m_Turn = 0;
  std::optional<Side> m_Winner;

  int m_AttacksPlayed = 0;
  int m_SpellsPlayed = 0;
  std::vector<CardID> m_PlayedThisTurn;
};

}
