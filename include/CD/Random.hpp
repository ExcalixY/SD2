#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <utility>
#include <vector>

namespace CD {

class Random final {
public:
  explicit Random(uint64_t seed);

  [[nodiscard]]
  double Float();
  [[nodiscard]]
  int Int(int min, int max);
  [[nodiscard]]
  bool Chance(double probability);
  [[nodiscard]]
  size_t Index(size_t size);

  template <typename TContainer>
  [[nodiscard]]
  const typename TContainer::value_type& Pick(const TContainer& items);
  template <typename T>
  void Shuffle(std::vector<T>& items);

private:
  std::mt19937_64 m_Engine;
};

template <typename TContainer>
const typename TContainer::value_type& Random::Pick(const TContainer& items) {
  return items[Index(items.size())];
}

template <typename T>
void Random::Shuffle(std::vector<T>& items) {
  for (size_t i = items.size(); i > 1; i--) {
    std::swap(items[i - 1], items[Index(i)]);
  }
}

}
