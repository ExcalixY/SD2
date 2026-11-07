#pragma once

#include <span>
#include <string>

#include "CD/Cards/Card.hpp"
#include "CD/Fight/Side.hpp"
#include "CD/GameEvents.hpp"

namespace CD {

class Game;

}

class Terminal_Printer final {
public:
  explicit Terminal_Printer(const CD::Game& game);

  void SetQuiet(bool quiet);

  void PrintEvents(std::span<const CD::GameEvent> events) const;
  void PrintBoard() const;
  void PrintShop() const;
  void PrintDeck() const;
  void PrintCardInfo(CD::CardID card) const;

  [[nodiscard]]
  std::string DescribeCard(CD::CardID card) const;
  [[nodiscard]]
  std::string GetCardName(CD::CardID card) const;
  [[nodiscard]]
  std::string GetSideName(CD::Side side) const;

private:
  void PrintEvent(const CD::GameEvent& event) const;

  const CD::Game& m_Game;
  bool m_Quiet = false;
};
