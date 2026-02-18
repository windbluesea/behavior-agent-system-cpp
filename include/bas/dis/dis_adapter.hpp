#pragma once

#include <optional>

#include "bas/common/types.hpp"

namespace bas {

class DisAdapter {
 public:
  void FeedMockFrame(const BattlefieldSnapshot& snapshot);
  std::optional<BattlefieldSnapshot> Poll();

 private:
  std::optional<BattlefieldSnapshot> latest_snapshot_;
};

}  // namespace bas
