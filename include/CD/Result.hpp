#pragma once

#include <string>
#include <utility>

namespace CD {

struct Result {
  bool ok = true;
  std::string reason;

  [[nodiscard]]
  static Result Ok() {
    return {};
  }

  [[nodiscard]]
  static Result Fail(std::string reason) {
    return {.ok = false, .reason = std::move(reason)};
  }

  explicit operator bool() const {
    return ok;
  }
};

}
