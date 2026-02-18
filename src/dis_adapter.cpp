#include "bas/dis/dis_adapter.hpp"

namespace bas {

void DisAdapter::FeedMockFrame(const BattlefieldSnapshot& snapshot) {
  latest_snapshot_ = snapshot;
}

std::optional<BattlefieldSnapshot> DisAdapter::Poll() {
  auto out = latest_snapshot_;
  latest_snapshot_.reset();
  return out;
}

}  // namespace bas
