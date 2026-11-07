#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string_view>
#include <vector>

#include "CD/Cards/CardDef.hpp"
#include "CD/Cards/CardKind.hpp"
#include "CD/Cards/DeckDef.hpp"

namespace CD {

struct Config;

class Catalog final {
public:
  [[nodiscard]]
  static Catalog FromJson(const nlohmann::json& cards, const nlohmann::json& decks, const Config& config);

  Catalog(const Catalog&) = delete;
  Catalog& operator=(const Catalog&) = delete;
  Catalog(Catalog&&) noexcept = default;
  Catalog& operator=(Catalog&&) noexcept = default;
  ~Catalog() = default;

  [[nodiscard]]
  const std::vector<CardDef>& GetCards() const;
  [[nodiscard]]
  const CardDef* FindCard(std::string_view id) const;
  [[nodiscard]]
  std::vector<const CardDef*> GetCardsOfKind(CardKind kind) const;
  [[nodiscard]]
  std::vector<const CardDef*> GetWildCards() const;
  [[nodiscard]]
  std::vector<const CardDef*> GetBasicCards() const;

  [[nodiscard]]
  const std::vector<DeckDef>& GetDecks() const;
  [[nodiscard]]
  const DeckDef* FindDeck(std::string_view id) const;

private:
  Catalog() = default;

  std::vector<CardDef> m_Cards;
  std::vector<DeckDef> m_Decks;
};

}
