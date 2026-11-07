#include "CD/Cpu/CpuContext.hpp"

#include <algorithm>
#include <optional>
#include <string_view>
#include <vector>

#include "CD/Cards/Card.hpp"
#include "CD/Cards/CardDef.hpp"
#include "CD/Fight/Fight.hpp"
#include "CD/Fight/PlayerState.hpp"
#include "CD/Fight/Scoring.hpp"
#include "CD/Fight/Side.hpp"
#include "CD/Structures/Containers.hpp"

namespace CD::Cpu {

namespace {

constexpr double EXPECTED_HIT_MARGIN = 1.2;

}

Context::Context(const Fight& fight)
    : m_Fight(fight),
      m_Me(fight.GetActiveSide()),
      m_Them(GetOpponent(m_Me)),
      m_Hand(ToVector(fight.GetPlayer(m_Me).hand)),
      m_ScoreNow(fight.PreviewScore(m_Me)) { }

const Fight& Context::GetFight() const {
  return m_Fight;
}

Side Context::GetMe() const {
  return m_Me;
}

Side Context::GetThem() const {
  return m_Them;
}

const PlayerState& Context::GetMine() const {
  return m_Fight.GetPlayer(m_Me);
}

const PlayerState& Context::GetTheirs() const {
  return m_Fight.GetPlayer(m_Them);
}

const std::vector<CardID>& Context::GetHand() const {
  return m_Hand;
}

const CardDef& Context::GetDef(const CardID card) const {
  return *m_Fight.GetCard(card)->def;
}

std::optional<CardID> Context::FindPlayable(const std::string_view card_id) const {
  for (const CardID card : m_Hand) {
    if (GetDef(card).id == card_id && CanPlay(card)) {
      return card;
    }
  }

  return std::nullopt;
}

bool Context::CanPlay(const CardID card) const {
  return m_Fight.CanPlay(card).ok;
}

bool Context::IsInMyArea(const std::string_view card_id) const {
  return m_Fight.HasInArea(m_Me, card_id);
}

bool Context::IsInTheirArea(const std::string_view card_id) const {
  return m_Fight.HasInArea(m_Them, card_id);
}

double Context::GetExpectedHit() const {
  return static_cast<double>(GetTheirs().lastHit) * EXPECTED_HIT_MARGIN;
}

double Context::GetFieldPower(const Side side) const {
  double damage = 0.;
  double mult = 0.;

  for (const CardID card : ToVector(m_Fight.GetPlayer(side).field)) {
    damage += GetDef(card).damage;
    mult += GetDef(card).mult;
  }

  return damage * std::max(1., mult);
}

const ScoreBreakdown& Context::GetScoreNow() const {
  return m_ScoreNow;
}

}
