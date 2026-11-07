#pragma once

#include <cstddef>
#include <queue>
#include <utility>
#include <vector>

#include "Utils/circular_list.hpp"

namespace CD {

template <typename T>
[[nodiscard]]
std::vector<T> ToVector(std::queue<T> queue);

template <typename T>
std::vector<T> Drain(std::queue<T>& queue);

template <typename T>
[[nodiscard]]
std::vector<T> ToVector(const circular_list<T>& list);

template <typename T>
[[nodiscard]]
bool Contains(const circular_list<T>& list, const T& value);

template <typename T>
bool Remove(circular_list<T>& list, const T& value);

template <typename T>
std::vector<T> ToVector(std::queue<T> queue) {
  return Drain(queue);
}

template <typename T>
std::vector<T> Drain(std::queue<T>& queue) {
  std::vector<T> items;
  items.reserve(queue.size());

  while (!queue.empty()) {
    items.push_back(std::move(queue.front()));
    queue.pop();
  }

  return items;
}

template <typename T>
std::vector<T> ToVector(const circular_list<T>& list) {
  std::vector<T> items;
  items.reserve(list.size());

  for (size_t i = 0; i < list.size(); i++) {
    items.push_back(list[i]);
  }

  return items;
}

template <typename T>
bool Contains(const circular_list<T>& list, const T& value) {
  for (size_t i = 0; i < list.size(); i++) {
    if (list[i] == value) {
      return true;
    }
  }

  return false;
}

template <typename T>
bool Remove(circular_list<T>& list, const T& value) {
  auto it = list.head();

  for (size_t i = 0; i < list.size(); i++, ++it) {
    if (*it == value) {
      list.erase(it);
      return true;
    }
  }

  return false;
}

}
