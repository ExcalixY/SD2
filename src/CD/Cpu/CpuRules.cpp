#include "CD/Cpu/CpuRules.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "CD/Cards/Card.hpp"
#include "CD/Cards/CardDef.hpp"
#include "CD/Config.hpp"
#include "CD/Cpu/CpuContext.hpp"
#include "CD/Fight/DiscardRisk.hpp"
#include "CD/Fight/Fight.hpp"
#include "CD/Fight/PlayerState.hpp"
#include "CD/Fight/Scoring.hpp"
#include "CD/Structures/Containers.hpp"

namespace CD::Cpu {

namespace {

constexpr double LOW_HP = 0.5;
constexpr double BIG_HIT = 0.2;
constexpr int HIGH_COMBO = 5;
constexpr int VERY_HIGH_COMBO = 7;
constexpr double STRONGER_FIELD = 1.25;
constexpr int STRONG_FIELD_SIZE = 3;
constexpr int MANY_EFFECTS = 2;
constexpr double RISKY_DISCARD = 0.2;
constexpr double DOUBLE_DOWN = 1.2;
constexpr int COMBO_WORTH_SAVING = 3;

using Condition = std::function<bool(const Context&)>;

Rule PlayIf(const std::string_view card_id, std::string why, Condition when) {
  return [card_id, why = std::move(why), when = std::move(when)](const Context& context) -> std::optional<Choice> {
    if (!when(context)) {
      return std::nullopt;
    }

    const std::optional<CardID> card = context.FindPlayable(card_id);
    if (!card) {
      return std::nullopt;
    }

    return Choice{.card = *card, .why = why};
  };
}

Rule Play(const std::string_view card_id, std::string why) {
  return PlayIf(card_id, std::move(why), [](const Context&) { return true; });
}

std::optional<Choice> PlayWilds(const Context& context) {
  const DiscardRisk risk = context.GetFight().GetDiscardRisk(context.GetMe());

  std::vector<CardID> wilds;
  for (const CardID card : context.GetHand()) {
    if (context.GetDef(card).isWild && context.CanPlay(card)) {
      wilds.push_back(card);
    }
  }

  const bool risky = risk.chance >= RISKY_DISCARD;
  if (wilds.empty() || (!risky && wilds.size() < 2)) {
    return std::nullopt;
  }

  const std::string why = risky ? std::format("discard risk {}", risk.label) : "holding 2+";
  for (const CardID card : wilds) {
    if (context.GetDef(card).isAttack) {
      return Choice{.card = card, .why = why};
    }
  }

  if (!context.IsInMyArea("saving_grace")) {
    for (const CardID card : wilds) {
      if (context.GetDef(card).id == "saving_grace") {
        return Choice{.card = card, .why = why};
      }
    }
  }

  return std::nullopt;
}

std::optional<Choice> PlayBestAttackSet(const Context& context) {
  const Fight& fight = context.GetFight();
  const Config& config = fight.GetConfig();
  const PlayerState& mine = context.GetMine();

  std::vector<CardID> attacks;
  for (const CardID card : context.GetHand()) {
    if (context.GetDef(card).isAttack) {
      attacks.push_back(card);
    }
  }

  const size_t room = std::max(0, config.maxPlayedAttacks - static_cast<int>(mine.played.size()));
  const int wilds_on_field = fight.GetWildsOnField(context.GetMe());

  std::vector<CardID> best_set;
  int64_t best_total = context.GetScoreNow().total;
  std::vector<CardID> set;

  const std::function<void(size_t)> search = [&](const size_t start) {
    if (!set.empty()) {
      int wilds = wilds_on_field;
      for (const CardID card : set) {
        wilds += context.GetDef(card).isWild ? 1 : 0;
      }

      if (wilds <= config.maxWildsOnField) {
        std::vector<CardID> played = mine.played;
        played.insert(played.end(), set.begin(), set.end());

        const int64_t total = fight.PreviewScore(context.GetMe(), played).total;
        if (total > best_total || (total == best_total && set.size() > best_set.size())) {
          best_total = total;
          best_set = set;
        }
      }
    }

    if (set.size() == room) {
      return;
    }

    for (size_t i = start; i < attacks.size(); i++) {
      set.push_back(attacks[i]);
      search(i + 1);
      set.pop_back();
    }
  };
  search(0);

  if (best_set.empty()) {
    return std::nullopt;
  }

  std::string names;
  for (const CardID card : best_set) {
    names += names.empty() ? "" : " + ";
    names += context.GetDef(card).name;
  }

  return Choice{.card = best_set.front(), .why = std::format("best set -> {}", names)};
}

std::vector<Section> BuildTree() {
  const auto expected_hit_kills = [](const Context& context) {
    return context.GetExpectedHit() >= static_cast<double>(context.GetMine().hp);
  };
  const auto their_combo_over = [](const int level) {
    return [level](const Context& context) { return context.GetTheirs().combo > level; };
  };
  const auto their_field_stronger = [](const Context& context) {
    return context.GetFieldPower(context.GetThem()) > context.GetFieldPower(context.GetMe()) * STRONGER_FIELD &&
           std::cmp_greater_equal(context.GetTheirs().field.size(), STRONG_FIELD_SIZE);
  };
  const auto combo_breaks = [](const Context& context) {
    const ScoreBreakdown& now = context.GetScoreNow();
    return static_cast<double>(now.total) <= now.target;
  };

  return {
    {
      .name = "Defensive",
      .rules =
        {
          PlayIf(
            "refund",
            "under 50% HP, heal",
            [](const Context& context) {
              return static_cast<double>(context.GetMine().hp) < static_cast<double>(context.GetMine().maxHp) * LOW_HP;
            }
          ),
          PlayIf(
            "cold_streak",
            "next hit is 20%+ of my HP",
            [](const Context& context) {
              return context.GetExpectedHit() >= static_cast<double>(context.GetMine().hp) * BIG_HIT &&
                     !context.IsInMyArea("cold_streak");
            }
          ),
          PlayIf(
            "no_action",
            "next hit could kill",
            [=](const Context& context) { return expected_hit_kills(context) && !context.IsInMyArea("no_action"); }
          ),
          PlayIf(
            "black_hole",
            "next hit could kill, no No Action",
            [=](const Context& context) { return expected_hit_kills(context) && !context.IsInMyArea("no_action"); }
          ),
          PlayIf(
            "saving_grace",
            "next hit could kill",
            [=](const Context& context) { return expected_hit_kills(context) && !context.IsInMyArea("saving_grace"); }
          ),
        },
    },
    {
      .name = "Break combo",
      .rules =
        {
          PlayIf(
            "cold_streak",
            "their combo is over 5x",
            [=](const Context& context) {
              return their_combo_over(HIGH_COMBO)(context) && !context.IsInMyArea("cold_streak");
            }
          ),
          PlayIf(
            "misdeal",
            "their combo is over 5x",
            [=](const Context& context) {
              return their_combo_over(HIGH_COMBO)(context) && !context.IsInMyArea("misdeal");
            }
          ),
          PlayIf(
            "no_dice",
            "they hold Lucky Number 7",
            [=](const Context& context) {
              return their_combo_over(HIGH_COMBO)(context) && context.IsInTheirArea("lucky_number_7");
            }
          ),
          PlayIf("black_hole", "their combo is over 7x", their_combo_over(VERY_HIGH_COMBO)),
        },
    },
    {
      .name = "Opponent field",
      .rules =
        {
          PlayIf(
            "black_hole",
            "stronger than mine",
            [=](const Context& context) {
              const std::vector<CardID> field = ToVector(context.GetTheirs().field);
              const bool has_wild =
                std::ranges::any_of(field, [&context](const CardID card) { return context.GetDef(card).isWild; });
              return their_field_stronger(context) || has_wild;
            }
          ),
          PlayIf(
            "burn_it_up",
            "stronger than mine",
            [=](const Context& context) { return their_field_stronger(context) && !context.FindPlayable("black_hole"); }
          ),
          PlayIf(
            "no_dice",
            "2+ effects in play",
            [](const Context& context) {
              return std::cmp_greater_equal(context.GetTheirs().effects.size(), MANY_EFFECTS);
            }
          ),
        },
    },
    {
      .name = "My Wilds",
      .rules = {PlayWilds},
    },
    {
      .name = "Attack",
      .rules =
        {
          PlayBestAttackSet,
          PlayIf(
            "double_down",
            "kills or reaches the combo",
            [](const Context& context) {
              const ScoreBreakdown& now = context.GetScoreNow();
              const double boosted = now.damage * now.mult * DOUBLE_DOWN + static_cast<double>(now.combo);
              const bool kills = boosted >= static_cast<double>(context.GetTheirs().hp);
              const bool reaches = static_cast<double>(now.total) <= now.target && boosted > now.target;
              return !context.IsInMyArea("double_down") && now.damage > 0. && (kills || reaches);
            }
          ),
          PlayIf(
            "lucky_number_7",
            "combo will break anyway",
            [=](const Context& context) {
              return combo_breaks(context) && context.GetMine().combo >= COMBO_WORTH_SAVING &&
                     !context.IsInMyArea("lucky_number_7");
            }
          ),
          PlayIf(
            "hit_me",
            "room in hand, draw 2",
            [](const Context& context) {
              return std::cmp_less_equal(context.GetMine().hand.size(), context.GetMine().handSize - 1);
            }
          ),
        },
    },
    {
      .name = "Always",
      .rules = {
        Play("let_it_ride", "Let It Ride"),
        PlayIf(
          "saving_grace", "Saving Grace", [](const Context& context) { return !context.IsInMyArea("saving_grace"); }
        ),
        PlayIf(
          "lucky_number_7",
          "Lucky Number 7",
          [](const Context& context) { return !context.IsInMyArea("lucky_number_7"); }
        ),
        PlayIf("rake", "Rake", [](const Context& context) { return !context.IsInMyArea("rake"); }),
        PlayIf("cold_streak", "Cold Streak", [](const Context& context) { return !context.IsInMyArea("cold_streak"); }),
        PlayIf("misdeal", "Misdeal", [](const Context& context) { return !context.IsInMyArea("misdeal"); }),
      },
    },
  };
}

}

const std::vector<Section>& GetTree() {
  static const std::vector<Section> s_Tree = BuildTree();
  return s_Tree;
}

}
