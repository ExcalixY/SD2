#include "Terminal_Printer.hpp"

#include <cstddef>
#include <format>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "CD/Cards/Card.hpp"
#include "CD/Cards/CardDef.hpp"
#include "CD/Cards/CardKind.hpp"
#include "CD/Config.hpp"
#include "CD/Effects/Effect.hpp"
#include "CD/Fight/DiscardRisk.hpp"
#include "CD/Fight/Fight.hpp"
#include "CD/Fight/PlayerState.hpp"
#include "CD/Fight/Scoring.hpp"
#include "CD/Fight/Side.hpp"
#include "CD/Format.hpp"
#include "CD/Game.hpp"
#include "CD/GameEvents.hpp"
#include "CD/Run/Rewards.hpp"
#include "CD/Run/Run.hpp"
#include "CD/Run/Upgrade.hpp"
#include "CD/Shop/Shop.hpp"
#include "CD/Structures/Containers.hpp"

namespace {

template <typename... TFuncs>
struct Overloaded : TFuncs... {
  using TFuncs::operator()...;
};

std::string JoinNames(const Terminal_Printer& printer, std::span<const CD::CardID> cards) {
  std::string out;

  for (const CD::CardID card : cards) {
    out += out.empty() ? "" : ", ";
    out += printer.GetCardName(card);
  }

  return out.empty() ? "-" : out;
}

std::string DescribeTokens(const CD::Card& card) {
  std::string out;

  for (const CD::Token token : CD::TOKENS) {
    const int count = card.GetTokens(token);
    if (count > 0) {
      out += std::format(" +{} {}", count, CD::ToString(token));
    }
  }

  return out;
}

void PrintSide(const Terminal_Printer& printer, const CD::Fight& fight, const CD::Side side) {
  const CD::PlayerState& state = fight.GetPlayer(side);
  const bool active = fight.GetActiveSide() == side && !fight.IsOver();

  std::println(
    "{} {:<7} HP {} / {}   combo {}x   deck {}   discard {}",
    active ? "*" : " ",
    state.name,
    CD::FormatAmount(state.hp),
    CD::FormatAmount(state.maxHp),
    state.combo,
    state.deck.size(),
    state.discard.size()
  );
  std::println(
    "    Field ({}/{}): {}", state.field.size(), state.fieldSize, JoinNames(printer, CD::ToVector(state.field))
  );

  std::string effects;
  for (const CD::Effect& effect : state.effects) {
    effects += effects.empty() ? "" : ", ";
    effects += effect.duration == -1
                 ? std::format("{} (until used)", printer.GetCardName(effect.card))
                 : std::format("{} ({} turn(s))", printer.GetCardName(effect.card), effect.duration);
  }
  std::println("    Effects ({}/{}): {}", state.effects.size(), state.areaSize, effects.empty() ? "-" : effects);
}

}

Terminal_Printer::Terminal_Printer(const CD::Game& game) : m_Game(game) { }

void Terminal_Printer::SetQuiet(const bool quiet) {
  m_Quiet = quiet;
}

void Terminal_Printer::PrintEvents(std::span<const CD::GameEvent> events) const {
  for (size_t i = 0; i < events.size(); i++) {
    const auto* drawn = std::get_if<CD::CardDrawn>(&events[i]);
    if (!drawn) {
      PrintEvent(events[i]);
      continue;
    }

    std::vector<CD::CardID> cards = {drawn->card};
    while (i + 1 < events.size()) {
      const auto* next = std::get_if<CD::CardDrawn>(&events[i + 1]);
      if (!next || next->side != drawn->side) {
        break;
      }
      cards.push_back(next->card);
      i++;
    }

    if (m_Quiet) {
      continue;
    }
    if (drawn->side == CD::Side::PLAYER) {
      std::println("  You draw {}", JoinNames(*this, cards));
    } else {
      std::println("  {} draws {} card(s)", GetSideName(drawn->side), cards.size());
    }
  }
}

void Terminal_Printer::PrintBoard() const {
  const CD::Fight* fight = m_Game.GetFight();
  if (!fight) {
    return;
  }

  std::println("");
  std::println(
    "=== Fight {}{} | Round {} | {}'s turn ===",
    fight->GetNumber(),
    fight->IsBoss() ? " (BOSS)" : "",
    fight->GetRound(),
    GetSideName(fight->GetActiveSide())
  );
  PrintSide(*this, *fight, CD::Side::CPU);
  PrintSide(*this, *fight, CD::Side::PLAYER);

  const CD::PlayerState& you = fight->GetPlayer(CD::Side::PLAYER);
  std::println(
    "    Played ({}/{}): {}", you.played.size(), m_Game.GetConfig().maxPlayedAttacks, JoinNames(*this, you.played)
  );

  std::println("    Hand:");
  int index = 1;
  for (const CD::CardID card : CD::ToVector(you.hand)) {
    const bool playable = m_Game.CanPlay(card).ok;
    std::println("      [{}] {}{}", index++, DescribeCard(card), playable ? "" : "  (can't play now)");
  }

  const CD::ScoreBreakdown score = m_Game.PreviewScore(CD::Side::PLAYER);
  std::println(
    "    Preview: {} | {} dmg x {} mult + {} combo = {}  (beat {} to raise the combo)",
    score.hand ? std::string(score.hand->name) : "no hand",
    CD::FormatAmount(CD::RoundAmount(score.damage)),
    CD::FormatFactor(score.mult),
    CD::FormatAmount(score.combo),
    CD::FormatAmount(score.total),
    CD::FormatAmount(CD::RoundAmount(score.target))
  );

  if (fight->HasRandomDiscard()) {
    std::println("    Discard risk: {}", m_Game.GetDiscardRisk(CD::Side::PLAYER).label);
  }
}

void Terminal_Printer::PrintShop() const {
  const CD::Shop* shop = m_Game.GetShop();
  const CD::Run* run = m_Game.GetRun();
  if (!shop || !run) {
    return;
  }

  std::println("");
  std::println("=== Shop | {} gold | next: fight {} ===", run->gold, run->fight);
  std::println("  Cards (b N):");
  for (size_t i = 0; i < shop->cards.size(); i++) {
    const CD::ShopCard& item = shop->cards[i];
    if (item.sold) {
      std::println("    [{}] sold", i + 1);
    } else {
      std::println("    [{}] {} - {} gold", i + 1, DescribeCard(item.card.id), item.price);
    }
  }

  const CD::TokenPack& pack = shop->tokenPack;
  std::println(
    "  Token pack (t N, N from the deck list): {}",
    pack.sold ? "sold" : std::format("{} token - {} gold", CD::ToString(pack.token), pack.price)
  );

  std::println("  Upgrades (u KEY):");
  for (const CD::Upgrade upgrade : CD::UPGRADES) {
    std::println(
      "    {:<6} {:<26} level {} - {} gold",
      CD::ToKey(upgrade),
      m_Game.GetConfig().shop.upgrades[std::to_underlying(upgrade)].name,
      run->GetUpgrade(upgrade) + 1,
      m_Game.GetUpgradePrice(upgrade)
    );
  }
}

void Terminal_Printer::PrintDeck() const {
  const CD::Run* run = m_Game.GetRun();
  if (!run) {
    return;
  }

  std::println("{} ({} cards):", run->deckName, run->cards.size());
  for (size_t i = 0; i < run->cards.size(); i++) {
    std::println("  [{}] {}", i + 1, DescribeCard(run->cards[i].id));
  }
}

void Terminal_Printer::PrintCardInfo(const CD::CardID card) const {
  const CD::Card* found = m_Game.GetCard(card);
  if (!found) {
    return;
  }

  const CD::CardDef& def = *found->def;
  std::println("{} - {} ({})", def.name, CD::ToString(def.kind), CD::ToString(found->suit));
  std::println("  {}", def.description);
  if (found->GetTokenCount() > 0) {
    std::println("  Tokens:{}", DescribeTokens(*found));
  }
}

std::string Terminal_Printer::DescribeCard(const CD::CardID card) const {
  const CD::Card* found = m_Game.GetCard(card);
  if (!found) {
    return "?";
  }

  const CD::CardDef& def = *found->def;
  std::string stats;
  if (def.damage > 0) {
    stats += std::format(" +{} dmg", def.damage);
  }
  if (def.mult > 0) {
    stats += std::format(" +{} mult", def.mult);
  }
  if (!def.isAttack || def.trigger) {
    stats += std::format(" - {}", def.description);
  }

  return std::format("{} ({}){}{}", def.name, CD::ToString(found->suit), stats, DescribeTokens(*found));
}

std::string Terminal_Printer::GetCardName(const CD::CardID card) const {
  const CD::Card* found = m_Game.GetCard(card);
  return found ? found->def->name : "?";
}

std::string Terminal_Printer::GetSideName(const CD::Side side) const {
  const CD::Fight* fight = m_Game.GetFight();
  return fight ? fight->GetPlayer(side).name : (side == CD::Side::PLAYER ? "Player" : "CPU");
}

void Terminal_Printer::PrintEvent(const CD::GameEvent& event) const {
  const bool quiet = m_Quiet;

  std::visit(
    Overloaded{
      [](const CD::RunStarted& started) {
        std::println("Run started with the {} ({} cards)", started.deckName, started.cardCount);
      },
      [](const CD::FightStarted& started) {
        std::println("");
        std::println(
          "##### Fight {}{}: Player {} HP vs {} {} HP{}",
          started.fight,
          started.boss ? " (BOSS)" : "",
          CD::FormatAmount(started.playerMaxHp),
          started.cpuName,
          CD::FormatAmount(started.cpuMaxHp),
          started.randomDiscard ? " | random discard is on" : ""
        );
      },
      [this, quiet](const CD::TurnStarted& started) {
        if (!quiet) {
          std::println("");
          std::println("--- {}'s turn {} ---", GetSideName(started.side), started.round);
        }
      },
      [this, quiet](const CD::DeckRefilled& refilled) {
        if (!quiet) {
          std::println("  {} shuffles {} card(s) back into the deck", GetSideName(refilled.side), refilled.count);
        }
      },
      [](const CD::CardDrawn&) { },
      [this, quiet](const CD::CardPlayed& played) {
        if (!quiet) {
          std::println(
            "  {} plays {}{}",
            GetSideName(played.side),
            GetCardName(played.card),
            played.zone == CD::PlayZone::AREA ? " to the Buff/Debuff area" : ""
          );
        }
      },
      [this, quiet](const CD::CardUnplayed& unplayed) {
        if (!quiet) {
          std::println("  {} takes {} back", GetSideName(unplayed.side), GetCardName(unplayed.card));
        }
      },
      [quiet](const CD::EffectTriggered& triggered) {
        if (!quiet) {
          std::println("  * {}", triggered.note);
        }
      },
      [this, quiet](const CD::HandEvaluated& evaluated) {
        if (!quiet) {
          std::println(
            "  {} has a {} (level {}): +{} dmg, +{} mult",
            GetSideName(evaluated.side),
            evaluated.hand.name,
            evaluated.hand.level,
            CD::FormatAmount(evaluated.hand.damage),
            evaluated.hand.mult
          );
        }
      },
      [this, quiet](const CD::CardQueued& queued) {
        if (!quiet && !queued.ejected.empty()) {
          std::println(
            "  {} enters the field, pushing off {}", GetCardName(queued.card), JoinNames(*this, queued.ejected)
          );
        }
      },
      [this, quiet](const CD::TurnScored& scored) {
        if (quiet) {
          return;
        }

        const CD::ScoreBreakdown& score = scored.score;
        std::println(
          "  {} scores {} dmg x {} mult + {} combo = {}",
          GetSideName(scored.side),
          CD::FormatAmount(CD::RoundAmount(score.damage)),
          CD::FormatFactor(score.mult),
          CD::FormatAmount(score.combo),
          CD::FormatAmount(score.total)
        );
        for (const std::string& note : score.notes) {
          std::println("    ({})", note);
        }
      },
      [this, quiet](const CD::ComboResolved& combo) {
        if (quiet) {
          return;
        }

        const std::string_view result = combo.result == CD::ComboResult::UP     ? "goes up"
                                        : combo.result == CD::ComboResult::KEPT ? "is kept"
                                                                                : "breaks";
        std::println("  {}'s combo {}: {}x -> {}x", GetSideName(combo.side), result, combo.before, combo.after);
      },
      [this, quiet](const CD::HitLanded& hit) {
        if (!quiet) {
          std::println(
            "  {} hits {} for {}{} ({} HP left)",
            GetSideName(hit.attacker),
            GetSideName(hit.target),
            CD::FormatAmount(hit.amount),
            hit.hitMult < 1. ? " (blocked)" : "",
            CD::FormatAmount(hit.hpAfter)
          );
        }
      },
      [this, quiet](const CD::EffectExpired& expired) {
        if (!quiet) {
          std::println("  {}'s {} leaves the area", GetSideName(expired.side), GetCardName(expired.card));
        }
      },
      [this, quiet](const CD::CardDiscarded& discarded) {
        if (!quiet) {
          std::println(
            "  Random discard: walked {} steps around {}'s hand, {} is discarded",
            discarded.steps,
            GetSideName(discarded.side),
            GetCardName(discarded.card)
          );
        }
      },
      [](const CD::FightWon& won) {
        std::println("");
        std::println(
          "VICTORY in {} round(s)! Best hit {}, best combo {}x, {} HP left",
          won.summary.rounds,
          CD::FormatAmount(won.summary.player.bestHit),
          won.summary.player.bestCombo,
          CD::FormatAmount(won.summary.playerHp)
        );
        for (const CD::GoldPart& part : won.gold) {
          std::println("  {}: +{} gold", part.label, part.gold);
        }
        if (won.firstBossDown) {
          std::println("The first boss is down! The run goes on in endless mode.");
        }
      },
      [](const CD::FightLost& lost) {
        std::println("");
        std::println(
          "DEFEAT at fight {}{} after {} round(s). {} had {} / {} HP left. Bosses defeated: {}",
          lost.summary.fight,
          lost.summary.boss ? " (boss)" : "",
          lost.summary.rounds,
          lost.summary.cpuName,
          CD::FormatAmount(lost.summary.cpuHp),
          CD::FormatAmount(lost.summary.cpuMaxHp),
          lost.bossesDefeated
        );
      },
    },
    event
  );
}
