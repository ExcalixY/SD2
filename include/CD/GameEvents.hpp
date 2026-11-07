#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "CD/Cards/Card.hpp"
#include "CD/Effects/Action.hpp"
#include "CD/Effects/Trigger.hpp"
#include "CD/Fight/PlayerState.hpp"
#include "CD/Fight/PokerHands.hpp"
#include "CD/Fight/Scoring.hpp"
#include "CD/Fight/Side.hpp"
#include "CD/Run/Rewards.hpp"

namespace CD {

struct RunStarted {
  std::string deckId;
  std::string deckName;
  int cardCount = 0;
};

struct FightStarted {
  int fight = 1;
  bool boss = false;
  bool randomDiscard = false;
  std::string cpuName;
  int64_t playerMaxHp = 0;
  int64_t cpuMaxHp = 0;
};

struct TurnStarted {
  Side side = Side::PLAYER;
  int round = 1;
};

struct DeckRefilled {
  Side side = Side::PLAYER;
  int count = 0;
};

struct CardDrawn {
  Side side = Side::PLAYER;
  CardID card = NO_CARD;
};

enum class PlayZone {
  PLAYED,
  AREA,
};

struct CardPlayed {
  Side side = Side::PLAYER;
  CardID card = NO_CARD;
  PlayZone zone = PlayZone::PLAYED;
  bool front = false;
};

struct CardUnplayed {
  Side side = Side::PLAYER;
  CardID card = NO_CARD;
};

struct EffectTriggered {
  Side owner = Side::PLAYER;
  CardID card = NO_CARD;
  Trigger trigger = Trigger::PLAY;
  Action action = Action::HEAL;
  Side target = Side::PLAYER;
  double value = 0.;

  int64_t amount = 0;
  int64_t hp = 0;
  int combo = 0;
  std::vector<CardID> cards;
  std::string note;
};

struct HandEvaluated {
  Side side = Side::PLAYER;
  HandResult hand;
};

struct CardQueued {
  Side side = Side::PLAYER;
  CardID card = NO_CARD;
  std::vector<CardID> ejected;
};

struct TurnScored {
  Side side = Side::PLAYER;
  ScoreBreakdown score;
};

enum class ComboResult {
  UP,
  KEPT,
  BROKEN,
};

struct ComboResolved {
  Side side = Side::PLAYER;
  ComboResult result = ComboResult::UP;
  int before = 1;
  int after = 1;
};

struct HitLanded {
  Side attacker = Side::PLAYER;
  Side target = Side::CPU;
  int64_t amount = 0;
  double hitMult = 1.;
  int64_t hpBefore = 0;
  int64_t hpAfter = 0;
};

struct EffectExpired {
  Side side = Side::PLAYER;
  CardID card = NO_CARD;
};

struct CardDiscarded {
  Side side = Side::PLAYER;
  int steps = 0;
  std::vector<CardID> handBefore;
  int index = 0;
  CardID card = NO_CARD;
};

struct FightSummary {
  int fight = 1;
  bool boss = false;
  int rounds = 0;
  PlayerStats player;
  int64_t playerHp = 0;
  int64_t playerMaxHp = 0;
  std::string cpuName;
  int64_t cpuHp = 0;
  int64_t cpuMaxHp = 0;
};

struct FightWon {
  FightSummary summary;
  std::vector<GoldPart> gold;
  int goldTotal = 0;
  bool firstBossDown = false;
};

struct FightLost {
  FightSummary summary;
  int bossesDefeated = 0;
};

using GameEvent = std::variant<
  RunStarted,
  FightStarted,
  TurnStarted,
  DeckRefilled,
  CardDrawn,
  CardPlayed,
  CardUnplayed,
  EffectTriggered,
  HandEvaluated,
  CardQueued,
  TurnScored,
  ComboResolved,
  HitLanded,
  EffectExpired,
  CardDiscarded,
  FightWon,
  FightLost>;

}
