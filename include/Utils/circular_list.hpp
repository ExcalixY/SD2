#pragma once

#include <cstddef>
#include <iterator>
#include <list>
#include <utility>

template <typename T>
class circular_list final {
public:
  class iterator {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T*;
    using reference = T&;

    iterator() = default;

    reference operator*() const {
      return *m_Iterator;
    }
    pointer operator->() const {
      return &*m_Iterator;
    }

    iterator& operator++() {
      if (++m_Iterator == m_List->end()) {
        m_Iterator = m_List->begin();
      }

      return *this;
    }

    iterator& operator--() {
      if (m_Iterator == m_List->begin()) {
        m_Iterator = m_List->end();
      }

      --m_Iterator;
      return *this;
    }

    iterator operator++(int) {
      auto t = *this;
      ++*this;
      return t;
    }

    iterator operator--(int) {
      auto t = *this;
      --*this;
      return t;
    }

    bool operator==(const iterator& other) const {
      return m_Iterator == other.m_Iterator;
    }
    bool operator!=(const iterator& other) const {
      return m_Iterator != other.m_Iterator;
    }

  private:
    friend class circular_list;

    iterator(std::list<T>* list, typename std::list<T>::iterator it) : m_List(list), m_Iterator(it) { }

    std::list<T>* m_List = nullptr;
    typename std::list<T>::iterator m_Iterator = {};
  };

  circular_list() = default;
  ~circular_list() = default;

  circular_list(const circular_list&) = default;
  circular_list(circular_list&&) = default;
  circular_list& operator=(const circular_list&) = default;
  circular_list& operator=(circular_list&&) = default;

  void push_front(const T& value) {
    m_Data.push_front(value);
  }
  void push_front(T&& value) {
    m_Data.push_front(std::move(value));
  }
  void push_back(const T& value) {
    m_Data.push_back(value);
  }
  void push_back(T&& value) {
    m_Data.push_back(std::move(value));
  }

  iterator insert(size_t pos, const T& value) {
    return iterator(&m_Data, m_Data.insert(ListAt(pos % (m_Data.size() + 1)), value));
  }
  iterator insert(size_t pos, T&& value) {
    return iterator(&m_Data, m_Data.insert(ListAt(pos % (m_Data.size() + 1)), std::move(value)));
  }

  iterator erase(iterator it) {
    auto next = m_Data.erase(it.m_Iterator);

    if (next == m_Data.end()) {
      next = m_Data.begin();
    }

    return iterator(&m_Data, next);
  }
  iterator erase(size_t index) {
    return erase(iterator(&m_Data, ListAt(index % m_Data.size())));
  }

  void pop_front() {
    m_Data.pop_front();
  }
  void pop_back() {
    m_Data.pop_back();
  }

  void clear() noexcept {
    m_Data.clear();
  }

  [[nodiscard]]
  size_t size() const noexcept {
    return m_Data.size();
  }

  [[nodiscard]]
  bool empty() const noexcept {
    return m_Data.empty();
  }

  [[nodiscard]]
  T& front() {
    return m_Data.front();
  }
  [[nodiscard]]
  const T& front() const {
    return m_Data.front();
  }

  [[nodiscard]]
  T& back() {
    return m_Data.back();
  }
  [[nodiscard]]
  const T& back() const {
    return m_Data.back();
  }

  [[nodiscard]]
  T& operator[](size_t index) {
    return *std::next(m_Data.begin(), static_cast<std::ptrdiff_t>(index % m_Data.size()));
  }
  [[nodiscard]]
  const T& operator[](size_t index) const {
    return *std::next(m_Data.begin(), static_cast<std::ptrdiff_t>(index % m_Data.size()));
  }

  [[nodiscard]]
  iterator head() {
    return iterator(&m_Data, m_Data.begin());
  }
  [[nodiscard]]
  iterator tail() {
    return iterator(&m_Data, std::prev(m_Data.end()));
  }

private:
  typename std::list<T>::iterator ListAt(size_t index) {
    return std::next(m_Data.begin(), static_cast<std::ptrdiff_t>(index));
  }

  std::list<T> m_Data = {};
};
