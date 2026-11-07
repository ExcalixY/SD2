#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "CD/Cards/Card.hpp"
#include "CD/Fight/PlayerState.hpp"
#include "CD/Fight/Scoring.hpp"
#include "CD/Fight/Side.hpp"

namespace CD {

class Fight;
struct CardDef;

}

namespace CD::Cpu {

class Context final {
public:
  explicit Context(const Fight& fight);

  [[nodiscard]]
  const Fight& GetFight() const;
  [[nodiscard]]
  Side GetMe() const;
  [[nodiscard]]
  Side GetThem() const;
  [[nodiscard]]
  const PlayerState& GetMine() const;
  [[nodiscard]]
  const PlayerState& GetTheirs() const;
  [[nodiscard]]
  const std::vector<CardID>& GetHand() const;
  [[nodiscard]]
  const CardDef& GetDef(CardID card) const;

  [[nodiscard]]
  std::optional<CardID> FindPlayable(std::string_view card_id) const;
  [[nodiscard]]
  bool CanPlay(CardID card) const;
  [[nodiscard]]
  bool IsInMyArea(std::string_view card_id) const;
  [[nodiscard]]
  bool IsInTheirArea(std::string_view card_id) const;

  [[nodiscard]]
  double GetExpectedHit() const;
  [[nodiscard]]
  double GetFieldPower(Side side) const;
  [[nodiscard]]
  const ScoreBreakdown& GetScoreNow() const;

private:
  const Fight& m_Fight;
  Side m_Me;
  Side m_Them;
  std::vector<CardID> m_Hand;
  ScoreBreakdown m_ScoreNow;
};

}
