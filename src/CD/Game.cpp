#include "CD/Game.hpp"

#include <algorithm>
#include <cstdint>
#include <format>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "CD/Cards/Card.hpp"
#include "CD/Cards/CardFactory.hpp"
#include "CD/Cards/Catalog.hpp"
#include "CD/Cards/DeckDef.hpp"
#include "CD/Commands.hpp"
#include "CD/Config.hpp"
#include "CD/Deck/DeckGenerator.hpp"
#include "CD/Deck/DeckStats.hpp"
#include "CD/Fight/DiscardRisk.hpp"
#include "CD/Fight/Fight.hpp"
#include "CD/Fight/PlayerState.hpp"
#include "CD/Fight/Scoring.hpp"
#include "CD/Fight/Side.hpp"
#include "CD/GameEvents.hpp"
#include "CD/Phase.hpp"
#include "CD/Result.hpp"
#include "CD/Run/FightSetup.hpp"
#include "CD/Run/Rewards.hpp"
#include "CD/Run/Run.hpp"
#include "CD/Run/Upgrade.hpp"
#include "CD/Shop/Shop.hpp"

namespace CD {

namespace {

template <typename... TFuncs>
struct Overloaded : TFuncs... {
  using TFuncs::operator()...;
};

FightSummary Summarise(const Fight& fight) {
  const PlayerState& player = fight.GetPlayer(Side::PLAYER);
  const PlayerState& cpu = fight.GetPlayer(Side::CPU);

  return {
    .fight = fight.GetNumber(),
    .boss = fight.IsBoss(),
    .rounds = fight.GetRound(),
    .player = player.stats,
    .playerHp = player.hp,
    .playerMaxHp = player.maxHp,
    .cpuName = cpu.name,
    .cpuHp = cpu.hp,
    .cpuMaxHp = cpu.maxHp,
  };
}

}

Game::Game(const Catalog& catalog, Config config, const uint64_t seed)
    : m_Catalog(catalog), m_Config(std::move(config)), m_Random(seed) { }

Result Game::StartRun(const std::string_view deck_id, const RunOptions& options) {
  if (Result phase = RequirePhase({Phase::NO_RUN}); !phase) {
    return phase;
  }

  const DeckDef* deck = m_Catalog.FindDeck(deck_id);
  if (!deck) {
    return Result::Fail(std::format("Unknown deck \"{}\"", deck_id));
  }

  const DeckCounts counts = GetDeckCounts(deck->balance, m_Config.deckSize, m_Catalog, m_Config, m_Random);
  std::optional<GeneratedDeck> cards = GenerateDeck(counts, m_Catalog, m_Config, m_Factory, m_Random);
  if (!cards) {
    return Result::Fail(std::format("Couldn't deal a valid {}", deck->name));
  }

  m_Run = Run{
    .deckId = deck->id,
    .deckName = deck->name,
    .cards = std::move(cards->cards),
    .deckStats = std::move(cards->stats),
    .options = options,
  };
  m_Events.emplace_back(
    RunStarted{
      .deckId = m_Run->deckId,
      .deckName = m_Run->deckName,
      .cardCount = static_cast<int>(m_Run->cards.size()),
    }
  );

  Result fight = StartFight();
  if (!fight) {
    m_Run.reset();
  }

  return fight;
}

Result Game::AbandonRun() {
  if (m_Phase == Phase::NO_RUN) {
    return Result::Fail("There is no run to abandon");
  }

  m_Fight.reset();
  m_Shop.reset();
  m_Phase = Phase::NO_RUN;
  return Result::Ok();
}

Result Game::PlayCard(const CardID card) {
  if (Result phase = RequirePhase({Phase::PLAYER_TURN, Phase::CPU_TURN}); !phase) {
    return phase;
  }

  Result played = m_Fight->Play(card);
  AfterFightCommand();
  return played;
}

Result Game::UnplayCard(const CardID card) {
  if (Result phase = RequirePhase({Phase::PLAYER_TURN, Phase::CPU_TURN}); !phase) {
    return phase;
  }

  return m_Fight->Unplay(card);
}

Result Game::EndTurn() {
  if (Result phase = RequirePhase({Phase::PLAYER_TURN, Phase::CPU_TURN}); !phase) {
    return phase;
  }

  Result ended = m_Fight->EndTurn();
  AfterFightCommand();
  return ended;
}

Result Game::EnterShop() {
  if (Result phase = RequirePhase({Phase::FIGHT_WON}); !phase) {
    return phase;
  }

  m_Fight.reset();
  m_Shop = RollShop(m_Catalog, m_Config, m_Factory, m_Random);
  m_Phase = Phase::SHOP;
  return Result::Ok();
}

Result Game::BuyCard(const int slot) {
  if (Result phase = RequirePhase({Phase::SHOP}); !phase) {
    return phase;
  }

  if (slot < 0 || slot >= std::ssize(m_Shop->cards)) {
    return Result::Fail("There is no card in that slot");
  }

  ShopCard& item = m_Shop->cards[static_cast<size_t>(slot)];
  if (item.sold) {
    return Result::Fail("Already sold");
  }
  if (m_Run->gold < item.price) {
    return Result::Fail("Not enough gold");
  }

  m_Run->gold -= item.price;
  item.sold = true;
  m_Run->cards.push_back(item.card);
  return Result::Ok();
}

Result Game::BuyTokenPack(const CardID target) {
  if (Result phase = RequirePhase({Phase::SHOP}); !phase) {
    return phase;
  }

  TokenPack& pack = m_Shop->tokenPack;
  if (pack.sold) {
    return Result::Fail("Already sold");
  }

  const auto card = std::ranges::find(m_Run->cards, target, &Card::id);
  if (card == m_Run->cards.end()) {
    return Result::Fail("Pick a card from your deck");
  }
  if (!card->def->isAttack) {
    return Result::Fail("Tokens only go on attack cards");
  }
  if (m_Run->gold < pack.price) {
    return Result::Fail("Not enough gold");
  }

  m_Run->gold -= pack.price;
  pack.sold = true;
  CardFactory::AddToken(*card, pack.token);
  return Result::Ok();
}

Result Game::BuyUpgrade(const Upgrade upgrade) {
  if (Result phase = RequirePhase({Phase::SHOP}); !phase) {
    return phase;
  }

  const int price = GetUpgradePrice(upgrade);
  if (m_Run->gold < price) {
    return Result::Fail("Not enough gold");
  }

  m_Run->gold -= price;
  m_Run->upgrades[std::to_underlying(upgrade)]++;
  return Result::Ok();
}

Result Game::NextFight() {
  if (Result phase = RequirePhase({Phase::SHOP}); !phase) {
    return phase;
  }

  m_Shop.reset();
  return StartFight();
}

Result Game::Submit(const Command& command) {
  return std::visit(
    Overloaded{
      [this](const Command_StartRun& start) { return StartRun(start.deckId, start.options); },
      [this](const Command_AbandonRun&) { return AbandonRun(); },
      [this](const Command_PlayCard& play) { return PlayCard(play.card); },
      [this](const Command_UnplayCard& unplay) { return UnplayCard(unplay.card); },
      [this](const Command_EndTurn&) { return EndTurn(); },
      [this](const Command_EnterShop&) { return EnterShop(); },
      [this](const Command_BuyCard& buy) { return BuyCard(buy.slot); },
      [this](const Command_BuyTokenPack& buy) { return BuyTokenPack(buy.target); },
      [this](const Command_BuyUpgrade& buy) { return BuyUpgrade(buy.upgrade); },
      [this](const Command_NextFight&) { return NextFight(); },
    },
    command
  );
}

Phase Game::GetPhase() const {
  return m_Phase;
}

const Config& Game::GetConfig() const {
  return m_Config;
}

const Catalog& Game::GetCatalog() const {
  return m_Catalog;
}

const Run* Game::GetRun() const {
  return m_Run ? &*m_Run : nullptr;
}

const Fight* Game::GetFight() const {
  return m_Fight.get();
}

const Shop* Game::GetShop() const {
  return m_Shop ? &*m_Shop : nullptr;
}

const Card* Game::GetCard(const CardID card) const {
  if (m_Fight) {
    if (const Card* found = m_Fight->GetCard(card)) {
      return found;
    }
  }

  if (m_Run) {
    const auto it = std::ranges::find(m_Run->cards, card, &Card::id);
    if (it != m_Run->cards.end()) {
      return &*it;
    }
  }

  if (m_Shop) {
    const auto it = std::ranges::find(m_Shop->cards, card, [](const ShopCard& item) { return item.card.id; });
    if (it != m_Shop->cards.end()) {
      return &it->card;
    }
  }

  return nullptr;
}

Result Game::CanPlay(const CardID card) const {
  if (Result phase = RequirePhase({Phase::PLAYER_TURN, Phase::CPU_TURN}); !phase) {
    return phase;
  }

  return m_Fight->CanPlay(card);
}

bool Game::CanUnplay(const CardID card) const {
  return m_Fight && m_Fight->CanUnplay(card);
}

ScoreBreakdown Game::PreviewScore(const Side side) const {
  return m_Fight ? m_Fight->PreviewScore(side) : ScoreBreakdown{};
}

DiscardRisk Game::GetDiscardRisk(const Side side) const {
  return m_Fight ? m_Fight->GetDiscardRisk(side) : DiscardRisk{};
}

int Game::GetUpgradePrice(const Upgrade upgrade) const {
  return m_Run ? CD::GetUpgradePrice(*m_Run, upgrade, m_Config) : 0;
}

std::vector<GameEvent> Game::TakeEvents() {
  return std::exchange(m_Events, {});
}

Result Game::RequirePhase(const std::initializer_list<Phase> phases) const {
  if (std::ranges::find(phases, m_Phase) != phases.end()) {
    return Result::Ok();
  }

  return Result::Fail(std::format("Not now: {}", ToString(m_Phase)));
}

Result Game::StartFight() {
  std::optional<FightSetup> setup = MakeFightSetup(*m_Run, m_Catalog, m_Config, m_Factory, m_Random);
  if (!setup) {
    return Result::Fail("Couldn't deal the CPU a valid deck");
  }

  m_Fight = std::make_unique<Fight>(m_Catalog, m_Config, m_Random, m_Events, std::move(*setup));
  m_Fight->Begin();
  AfterFightCommand();
  return Result::Ok();
}

void Game::AfterFightCommand() {
  if (!m_Fight->IsOver()) {
    m_Phase = m_Fight->GetActiveSide() == Side::PLAYER ? Phase::PLAYER_TURN : Phase::CPU_TURN;
    return;
  }

  FightSummary summary = Summarise(*m_Fight);

  if (m_Fight->GetWinner() == Side::CPU) {
    m_Events.emplace_back(
      FightLost{
        .summary = std::move(summary),
        .bossesDefeated = (m_Run->fight - 1) / m_Config.bossEvery,
      }
    );
    m_Phase = Phase::RUN_LOST;
    return;
  }

  std::vector<GoldPart> gold = GetGoldReward(summary.rounds, summary.boss, m_Config, m_Random);
  int total = 0;
  for (const GoldPart& part : gold) {
    total += part.gold;
  }

  const bool first_boss_down = summary.boss && !m_Run->wonRun;
  m_Run->gold += total;
  m_Run->wins++;
  m_Run->wonRun = m_Run->wonRun || summary.boss;
  m_Run->fight++;

  m_Events.emplace_back(
    FightWon{
      .summary = std::move(summary),
      .gold = std::move(gold),
      .goldTotal = total,
      .firstBossDown = first_boss_down,
    }
  );
  m_Phase = Phase::FIGHT_WON;
}

}
