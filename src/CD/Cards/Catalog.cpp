#include "CD/Cards/Catalog.hpp"

#include <algorithm>
#include <format>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "CD/Cards/CardDef.hpp"
#include "CD/Cards/CardKind.hpp"
#include "CD/Cards/DeckDef.hpp"
#include "CD/Config.hpp"
#include "CD/Effects/Action.hpp"
#include "CD/Effects/Trigger.hpp"
#include "nlohmann/json_fwd.hpp"

namespace CD {

namespace {

std::optional<double> ReadSide(const nlohmann::json& action, const char* key) {
  if (!action.contains(key)) {
    return std::nullopt;
  }

  const nlohmann::json& value = action.at(key);
  if (value.is_boolean()) {
    return value.get<bool>() ? std::optional(1.) : std::nullopt;
  }

  return value.get<double>();
}

CardKind ReadKind(const std::string& type, const std::string& subtype, const std::string& id) {
  if (type == "WILD") {
    return subtype == "ATTACK" ? CardKind::WILD_ATTACK : CardKind::WILD_SPELL;
  }
  if (subtype == "DAMAGE") {
    return CardKind::DAMAGE;
  }
  if (subtype == "MULT") {
    return CardKind::MULT;
  }
  if (subtype == "BUFF") {
    return CardKind::BUFF;
  }
  if (subtype == "DEBUFF") {
    return CardKind::DEBUFF;
  }

  throw std::runtime_error(std::format("Card \"{}\" has an unknown type: {} / {}", id, type, subtype));
}

CardDef ReadCard(const nlohmann::json& json, const Config& config) {
  CardDef def;
  def.id = json.at("id").get<std::string>();
  def.name = json.at("name").get<std::string>();
  def.description = json.value("description", "");
  def.iconSprite = json.value("iconSprite", "");

  const nlohmann::json& specs = json.at("specs");
  const auto type = json.at("type").get<std::string>();
  def.kind = ReadKind(type, specs.value("subtype", ""), def.id);
  def.isWild = type == "WILD";
  def.isAttack = def.kind == CardKind::DAMAGE || def.kind == CardKind::MULT || def.kind == CardKind::WILD_ATTACK;

  def.damage = specs.value("damage", 0);
  def.mult = specs.value("mult", 0);
  def.duration = specs.value("duration", 1);

  if (specs.contains("event")) {
    const auto key = specs.at("event").get<std::string>();
    def.trigger = TriggerFromKey(key);
    if (!def.trigger) {
      throw std::runtime_error(std::format("Card \"{}\" has an unknown event: {}", def.id, key));
    }
  }

  for (const nlohmann::json& action : specs.value("actions", nlohmann::json::array())) {
    const auto key = action.at("name").get<std::string>();
    const std::optional<Action> name = ActionFromKey(key);
    if (!name) {
      throw std::runtime_error(std::format("Card \"{}\" has an unknown action: {}", def.id, key));
    }

    def.actions.push_back({
      .action = *name,
      .player = ReadSide(action, "player"),
      .opponent = ReadSide(action, "opponent"),
    });
  }

  const auto meta = std::ranges::find(config.cardMeta, std::string_view(def.id), &CardMeta::id);
  const CardMeta& fallback = meta != config.cardMeta.end() ? *meta : config.defaultMeta;
  def.maxCopies = json.value("maxCopies", fallback.maxCopies);
  def.strength = json.value("strength", fallback.strength);
  def.cost = json.value("cost", def.isWild ? config.wildCost : config.cardCost);
  return def;
}

DeckDef ReadDeck(const nlohmann::json& json) {
  const nlohmann::json& balance = json.at("card-balance");

  return {
    .id = json.at("id").get<std::string>(),
    .name = json.at("name").get<std::string>(),
    .description = json.value("description", ""),
    .sprite = json.value("sprite", ""),
    .balance = {
      .random = balance.value("random", false),
      .allowWildRandom = balance.value("allowWildRandom", false),
      .damage = balance.value("type_damage", 0),
      .mult = balance.value("type_mult", 0),
      .buff = balance.value("type_buff", 0),
      .debuff = balance.value("type_debuff", 0),
      .wildDamage = balance.value("type_wild_damage", 0),
      .wildMagic = balance.value("type_wild_magic", 0),
    },
  };
}

}

Catalog Catalog::FromJson(const nlohmann::json& cards, const nlohmann::json& decks, const Config& config) {
  Catalog catalog;

  try {
    for (const char* group : {"damageCards", "spellCards", "wildCards"}) {
      for (const nlohmann::json& card : cards.at(group)) {
        catalog.m_Cards.push_back(ReadCard(card, config));
      }
    }

    for (const nlohmann::json& deck : decks.at("decks")) {
      catalog.m_Decks.push_back(ReadDeck(deck));
    }
  } catch (const nlohmann::json::exception& error) {
    throw std::runtime_error(std::format("Card data is malformed: {}", error.what()));
  }

  if (catalog.m_Cards.empty() || catalog.m_Decks.empty()) {
    throw std::runtime_error("Card data has no cards or no decks.");
  }

  return catalog;
}

const std::vector<CardDef>& Catalog::GetCards() const {
  return m_Cards;
}

const CardDef* Catalog::FindCard(const std::string_view id) const {
  const auto it = std::ranges::find(m_Cards, id, &CardDef::id);
  return it != m_Cards.end() ? &*it : nullptr;
}

std::vector<const CardDef*> Catalog::GetCardsOfKind(const CardKind kind) const {
  std::vector<const CardDef*> defs;

  for (const CardDef& def : m_Cards) {
    if (def.kind == kind) {
      defs.push_back(&def);
    }
  }

  return defs;
}

std::vector<const CardDef*> Catalog::GetWildCards() const {
  std::vector<const CardDef*> defs;

  for (const CardDef& def : m_Cards) {
    if (def.isWild) {
      defs.push_back(&def);
    }
  }

  return defs;
}

std::vector<const CardDef*> Catalog::GetBasicCards() const {
  std::vector<const CardDef*> defs;

  for (const CardDef& def : m_Cards) {
    if (!def.isWild) {
      defs.push_back(&def);
    }
  }

  return defs;
}

const std::vector<DeckDef>& Catalog::GetDecks() const {
  return m_Decks;
}

const DeckDef* Catalog::FindDeck(const std::string_view id) const {
  const auto it = std::ranges::find(m_Decks, id, &DeckDef::id);
  return it != m_Decks.end() ? &*it : nullptr;
}

}
