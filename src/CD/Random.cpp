#include "CD/Random.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace CD {

Random::Random(const uint64_t seed) : m_Engine(seed) { }

double Random::Float() {
  return static_cast<double>(m_Engine() >> 11) * 0x1.0p-53;
}

int Random::Int(const int min, const int max) {
  return min + static_cast<int>(std::floor(Float() * static_cast<double>(max - min + 1)));
}

bool Random::Chance(const double probability) {
  return Float() < probability;
}

size_t Random::Index(const size_t size) {
  return static_cast<size_t>(std::floor(Float() * static_cast<double>(size)));
}

}
