#pragma once

#include <cstdint>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "CD/Cards/Card.hpp"
#include "CD/Cards/CardFactory.hpp"
#include "CD/Commands.hpp"
#include "CD/Config.hpp"
#include "CD/Fight/DiscardRisk.hpp"
#include "CD/Fight/Fight.hpp"
#include "CD/Fight/Scoring.hpp"
#include "CD/Fight/Side.hpp"
#include "CD/GameEvents.hpp"
#include "CD/Phase.hpp"
#include "CD/Random.hpp"
#include "CD/Result.hpp"
#include "CD/Run/Run.hpp"
#include "CD/Run/Upgrade.hpp"
#include "CD/Shop/Shop.hpp"

namespace CD {

class Catalog;

class Game final {
public:
  Game(const Catalog& catalog, Config config, uint64_t seed);

  Game(const Game&) = delete;
  Game& operator=(const Game&) = delete;
  Game(Game&&) = delete;
  Game& operator=(Game&&) = delete;
  ~Game() = default;

  Result StartRun(std::string_view deck_id, const RunOptions& options = {});
  Result AbandonRun();

  Result PlayCard(CardID card);
  Result UnplayCard(CardID card);
  Result EndTurn();

  Result EnterShop();
  Result BuyCard(int slot);
  Result BuyTokenPack(CardID target);
  Result BuyUpgrade(Upgrade upgrade);
  Result NextFight();

  Result Submit(const Command& command);

  [[nodiscard]]
  Phase GetPhase() const;
  [[nodiscard]]
  const Config& GetConfig() const;
  [[nodiscard]]
  const Catalog& GetCatalog() const;
  [[nodiscard]]
  const Run* GetRun() const;
  [[nodiscard]]
  const Fight* GetFight() const;
  [[nodiscard]]
  const Shop* GetShop() const;
  [[nodiscard]]
  const Card* GetCard(CardID card) const;

  [[nodiscard]]
  Result CanPlay(CardID card) const;
  [[nodiscard]]
  bool CanUnplay(CardID card) const;
  [[nodiscard]]
  ScoreBreakdown PreviewScore(Side side) const;
  [[nodiscard]]
  DiscardRisk GetDiscardRisk(Side side) const;
  [[nodiscard]]
  int GetUpgradePrice(Upgrade upgrade) const;

  [[nodiscard]]
  std::vector<GameEvent> TakeEvents();

private:
  [[nodiscard]]
  Result RequirePhase(std::initializer_list<Phase> phases) const;
  Result StartFight();
  void AfterFightCommand();

  const Catalog& m_Catalog;
  Config m_Config;
  Random m_Random;
  CardFactory m_Factory;

  Phase m_Phase = Phase::NO_RUN;
  std::optional<Run> m_Run;
  std::unique_ptr<Fight> m_Fight;
  std::optional<Shop> m_Shop;
  std::vector<GameEvent> m_Events;
};

}
