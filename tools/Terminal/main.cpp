#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
#include <print>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

#include "CD/Cards/Card.hpp"
#include "CD/Cards/Catalog.hpp"
#include "CD/Commands.hpp"
#include "CD/Config.hpp"
#include "CD/Cpu/Cpu.hpp"
#include "CD/Effects/Effect.hpp"
#include "CD/Fight/Fight.hpp"
#include "CD/Fight/PlayerState.hpp"
#include "CD/Fight/Side.hpp"
#include "CD/Game.hpp"
#include "CD/GameEvents.hpp"
#include "CD/Phase.hpp"
#include "CD/Result.hpp"
#include "CD/Run/Run.hpp"
#include "CD/Run/Upgrade.hpp"
#include "CD/Structures/Containers.hpp"
#include "Terminal_Printer.hpp"
#include "nlohmann/json_fwd.hpp"

namespace {

constexpr int MAX_CPU_PLAYS = 20;

struct Options {
  std::string deck;
  std::optional<uint64_t> seed;
  std::filesystem::path assets;
  bool randomDiscard = false;
  bool autoPlay = false;
  int fights = 10;
  bool quiet = false;
};

template <typename T>
std::optional<T> ParseNumber(std::string_view text) {
  T value{};
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
  if (error != std::errc{} || end != text.data() + text.size()) {
    return std::nullopt;
  }

  return value;
}

std::optional<Options> ParseOptions(int argc, char** argv) {
  Options options;

  for (int i = 1; i < argc; i++) {
    const std::string_view arg = argv[i];
    const bool has_value = i + 1 < argc;

    if (arg == "--deck" && has_value) {
      options.deck = argv[++i];
    } else if (arg == "--seed" && has_value) {
      options.seed = ParseNumber<uint64_t>(argv[++i]);
    } else if (arg == "--assets" && has_value) {
      options.assets = argv[++i];
    } else if (arg == "--fights" && has_value) {
      options.fights = ParseNumber<int>(argv[++i]).value_or(options.fights);
    } else if (arg == "--discard") {
      options.randomDiscard = true;
    } else if (arg == "--auto") {
      options.autoPlay = true;
    } else if (arg == "--quiet") {
      options.quiet = true;
    } else {
      std::println(
        "Usage: SD2Terminal [--deck ID] [--seed N] [--assets DIR] [--discard] [--auto] [--fights N] [--quiet]"
      );
      return std::nullopt;
    }
  }

  return options;
}

std::optional<std::filesystem::path> FindConfigDir(const std::filesystem::path& given, const char* executable) {
  if (!given.empty()) {
    return given / "Config";
  }

  std::vector<std::filesystem::path> roots = {std::filesystem::current_path()};
  std::error_code error;
  const std::filesystem::path exe = std::filesystem::canonical(executable, error);
  if (!error) {
    roots.push_back(exe.parent_path());
  }

  for (std::filesystem::path root : roots) {
    for (int depth = 0; depth < 6 && !root.empty(); depth++, root = root.parent_path()) {
      const std::filesystem::path dir = root / "Assets" / "Config";
      if (std::filesystem::exists(dir / "cards.json")) {
        return dir;
      }
      if (root == root.parent_path()) {
        break;
      }
    }
  }

  return std::nullopt;
}

nlohmann::json ReadJson(const std::filesystem::path& path) {
  std::ifstream file(path);
  if (!file) {
    throw std::runtime_error("Can't open " + path.string());
  }

  return nlohmann::json::parse(file);
}

std::vector<std::string> Split(const std::string& line) {
  std::istringstream stream(line);
  std::vector<std::string> words;

  for (std::string word; stream >> word;) {
    words.push_back(word);
  }

  return words;
}

std::optional<std::string> Prompt(std::string_view text) {
  std::print("{}", text);
  std::cout.flush();

  std::string line;
  if (!std::getline(std::cin, line)) {
    return std::nullopt;
  }

  return line;
}

template <typename T>
std::optional<T> At(const std::vector<T>& items, std::string_view number) {
  const std::optional<int> index = ParseNumber<int>(number);
  if (!index || *index < 1 || *index > static_cast<int>(items.size())) {
    return std::nullopt;
  }

  return items[static_cast<size_t>(*index - 1)];
}

class Terminal final {
public:
  Terminal(CD::Game& game, Options options) : m_Game(game), m_Printer(game), m_Options(std::move(options)) {
    m_Printer.SetQuiet(m_Options.quiet);
  }

  int Run() {
    while (true) {
      switch (m_Game.GetPhase()) {
        case CD::Phase::PLAYER_TURN:
          if (m_Options.autoPlay ? !PlayCpuTurn() : !PlayPlayerTurn()) {
            return 0;
          }
          break;
        case CD::Phase::CPU_TURN:
          PlayCpuTurn();
          break;
        case CD::Phase::FIGHT_WON:
          if (!AfterWin()) {
            return 0;
          }
          break;
        case CD::Phase::SHOP:
          if (!InShop()) {
            return 0;
          }
          break;
        case CD::Phase::RUN_LOST:
          if (!AfterLoss()) {
            return 0;
          }
          break;
        case CD::Phase::NO_RUN:
          if (!ShowMenu()) {
            return 0;
          }
          break;
      }
    }
  }

private:
  bool ShowMenu() {
    std::string deck = std::exchange(m_Options.deck, {});

    while (deck.empty()) {
      std::println("");
      std::println("=== Carduellum ===");
      std::println("Pick a deck to start a run, or q to quit:");
      const auto& decks = m_Game.GetCatalog().GetDecks();
      for (size_t i = 0; i < decks.size(); i++) {
        std::println("  [{}] {} - {}", i + 1, decks[i].name, decks[i].description);
      }

      const std::optional<std::string> line = Prompt("> ");
      if (!line || *line == "q" || *line == "quit") {
        return false;
      }

      const std::optional<int> index = ParseNumber<int>(*line);
      if (index && *index >= 1 && *index <= static_cast<int>(decks.size())) {
        deck = decks[static_cast<size_t>(*index - 1)].id;
      }
    }

    return Check(m_Game.StartRun(deck, {.randomDiscard = m_Options.randomDiscard}));
  }

  bool PlayPlayerTurn() {
    m_Printer.PrintBoard();

    while (m_Game.GetPhase() == CD::Phase::PLAYER_TURN) {
      const std::optional<std::string> line = Prompt("\n> ");
      if (!line) {
        return false;
      }

      const std::vector<std::string> words = Split(*line);
      if (words.empty()) {
        continue;
      }

      const std::string& verb = words.front();
      const CD::PlayerState& you = m_Game.GetFight()->GetPlayer(CD::Side::PLAYER);

      if (verb == "p" || verb == "play") {
        const std::vector<CD::CardID> hand = CD::ToVector(you.hand);
        std::vector<CD::CardID> cards;
        for (size_t i = 1; i < words.size(); i++) {
          if (const std::optional<CD::CardID> card = At(hand, words[i])) {
            cards.push_back(*card);
          } else {
            std::println("  No card [{}] in your hand", words[i]);
          }
        }

        for (const CD::CardID card : cards) {
          Check(m_Game.PlayCard(card));
        }
        Flush();
        m_Printer.PrintBoard();
      } else if (verb == "u" || verb == "unplay") {
        const std::vector<CD::CardID> takeable = GetTakeable();
        if (words.size() < 2) {
          PrintTakeable(takeable);
        } else if (const std::optional<CD::CardID> card = At(takeable, words[1])) {
          Check(m_Game.UnplayCard(*card));
          Flush();
          m_Printer.PrintBoard();
        } else {
          PrintTakeable(takeable);
        }
      } else if (verb == "e" || verb == "end") {
        Check(m_Game.EndTurn());
        Flush();
      } else if (verb == "a" || verb == "auto") {
        PlayCpuTurn();
      } else if (verb == "i" || verb == "info") {
        if (const std::optional<CD::CardID> card =
              words.size() > 1 ? At(CD::ToVector(you.hand), words[1]) : std::nullopt) {
          m_Printer.PrintCardInfo(*card);
        } else {
          std::println("  Usage: i N (a card in your hand)");
        }
      } else if (verb == "s" || verb == "show") {
        m_Printer.PrintBoard();
      } else if (verb == "d" || verb == "deck") {
        m_Printer.PrintDeck();
      } else if (verb == "m" || verb == "menu") {
        return BackToMenu();
      } else if (verb == "q" || verb == "quit") {
        return false;
      } else {
        PrintFightHelp();
      }
    }

    return true;
  }

  bool PlayCpuTurn() {
    for (int plays = 0; plays < MAX_CPU_PLAYS; plays++) {
      const CD::Fight* fight = m_Game.GetFight();
      if (!fight || fight->IsOver()) {
        return true;
      }

      const CD::Cpu::Decision decision = CD::Cpu::Decide(*fight);
      const bool ends = std::holds_alternative<CD::Command_EndTurn>(decision.command);
      if (!m_Options.quiet) {
        std::println("  [{} rule] {}", m_Printer.GetSideName(fight->GetActiveSide()), decision.rule);
      }

      const CD::Result result = m_Game.Submit(decision.command);
      Flush();
      if (!result || ends) {
        if (!ends) {
          Check(m_Game.EndTurn());
          Flush();
        }
        return true;
      }
    }

    Check(m_Game.EndTurn());
    Flush();
    return true;
  }

  bool AfterWin() {
    if (m_Options.autoPlay) {
      if (m_Game.GetRun()->fight > m_Options.fights) {
        std::println("Stopping after {} fight(s).", m_Options.fights);
        return false;
      }
      return Check(m_Game.EnterShop());
    }

    const std::optional<std::string> line = Prompt("\n[Enter] go to the shop, m menu, q quit > ");
    if (!line || *line == "q") {
      return false;
    }
    if (*line == "m") {
      return BackToMenu();
    }

    return Check(m_Game.EnterShop());
  }

  bool AfterLoss() {
    if (m_Options.autoPlay) {
      return false;
    }

    const std::optional<std::string> line = Prompt("\n[Enter] back to the menu, q quit > ");
    if (!line || *line == "q") {
      return false;
    }

    return BackToMenu();
  }

  bool BackToMenu() {
    std::println("Run abandoned.");
    return Check(m_Game.AbandonRun());
  }

  bool InShop() {
    if (m_Options.autoPlay) {
      BuyCheapestUpgrades();
      return Check(m_Game.NextFight());
    }

    m_Printer.PrintShop();

    while (m_Game.GetPhase() == CD::Phase::SHOP) {
      const std::optional<std::string> line = Prompt("\n> ");
      if (!line) {
        return false;
      }

      const std::vector<std::string> words = Split(*line);
      if (words.empty()) {
        continue;
      }

      const std::string& verb = words.front();
      const std::string argument = words.size() > 1 ? words[1] : "";

      if (verb == "b" || verb == "buy") {
        const std::optional<int> slot = ParseNumber<int>(argument);
        Check(m_Game.BuyCard(slot.value_or(0) - 1));
        m_Printer.PrintShop();
      } else if (verb == "t" || verb == "token") {
        std::vector<CD::CardID> deck;
        for (const CD::Card& card : m_Game.GetRun()->cards) {
          deck.push_back(card.id);
        }
        Check(m_Game.BuyTokenPack(At(deck, argument).value_or(CD::NO_CARD)));
        m_Printer.PrintShop();
      } else if (verb == "u" || verb == "upgrade") {
        if (const std::optional<CD::Upgrade> upgrade = CD::UpgradeFromKey(argument)) {
          Check(m_Game.BuyUpgrade(*upgrade));
        } else {
          std::println("  Upgrades: hand, field, area, hp, hands");
        }
        m_Printer.PrintShop();
      } else if (verb == "d" || verb == "deck") {
        m_Printer.PrintDeck();
      } else if (verb == "s" || verb == "show") {
        m_Printer.PrintShop();
      } else if (verb == "n" || verb == "next") {
        Check(m_Game.NextFight());
        Flush();
      } else if (verb == "m" || verb == "menu") {
        return BackToMenu();
      } else if (verb == "q" || verb == "quit") {
        return false;
      } else {
        PrintShopHelp();
      }
    }

    return true;
  }

  void BuyCheapestUpgrades() {
    while (true) {
      const auto cheapest = std::ranges::min(CD::UPGRADES, {}, [this](const CD::Upgrade upgrade) {
        return m_Game.GetUpgradePrice(upgrade);
      });
      if (!m_Game.BuyUpgrade(cheapest)) {
        return;
      }
      if (!m_Options.quiet) {
        std::println("  Bought upgrade: {}", CD::ToKey(cheapest));
      }
    }
  }

  std::vector<CD::CardID> GetTakeable() const {
    const CD::PlayerState& you = m_Game.GetFight()->GetPlayer(CD::Side::PLAYER);
    std::vector<CD::CardID> cards;

    for (const CD::CardID card : you.played) {
      cards.push_back(card);
    }
    for (const CD::Effect& effect : you.effects) {
      if (m_Game.CanUnplay(effect.card)) {
        cards.push_back(effect.card);
      }
    }

    return cards;
  }

  void PrintTakeable(const std::vector<CD::CardID>& cards) const {
    if (cards.empty()) {
      std::println("  Nothing to take back");
      return;
    }

    std::println("  Take back (u N):");
    for (size_t i = 0; i < cards.size(); i++) {
      std::println("    [{}] {}", i + 1, m_Printer.GetCardName(cards[i]));
    }
  }

  static void PrintFightHelp() {
    std::println("  p N [N ...]  play cards from your hand (numbers as listed)");
    std::println("  u [N]        take back a card played this turn");
    std::println("  e            end the turn");
    std::println("  a            let the CPU play this turn for you");
    std::println("  i N          card info");
    std::println("  s            show the board");
    std::println("  d            show your deck");
    std::println("  m            abandon the run, back to the menu");
    std::println("  q            quit");
  }

  static void PrintShopHelp() {
    std::println("  b N          buy card N");
    std::println("  t N          buy the token pack for card N of your deck (see d)");
    std::println("  u KEY        buy an upgrade: hand, field, area, hp, hands");
    std::println("  d            show your deck");
    std::println("  s            show the shop");
    std::println("  n            next fight");
    std::println("  m            abandon the run, back to the menu");
    std::println("  q            quit");
  }

  bool Check(const CD::Result& result) {
    if (!result) {
      std::println("  ! {}", result.reason);
    }
    Flush();
    return result.ok;
  }

  void Flush() {
    const std::vector<CD::GameEvent> events = m_Game.TakeEvents();
    m_Printer.PrintEvents(events);
  }

  CD::Game& m_Game;
  Terminal_Printer m_Printer;
  Options m_Options;
};

}

int main(int argc, char** argv) {
  const std::optional<Options> options = ParseOptions(argc, argv);
  if (!options) {
    return 1;
  }

  const std::optional<std::filesystem::path> config_dir = FindConfigDir(options->assets, argv[0]);
  if (!config_dir) {
    std::println(stderr, "Can't find Assets/Config/cards.json. Run from the project folder or pass --assets DIR.");
    return 1;
  }

  try {
    const CD::Config config;
    const CD::Catalog catalog =
      CD::Catalog::FromJson(ReadJson(*config_dir / "cards.json"), ReadJson(*config_dir / "decks.json"), config);

    const uint64_t seed = options->seed.value_or(std::random_device{}());
    std::println("Carduellum | seed {}", seed);

    CD::Game game(catalog, config, seed);
    Terminal terminal(game, *options);
    return terminal.Run();
  } catch (const std::exception& error) {
    std::println(stderr, "Error: {}", error.what());
    return 1;
  }
}
