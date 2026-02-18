#pragma once

#include <vector>

#include "bas/common/types.hpp"

namespace bas {

class SituationFusion {
 public:
  SituationSemantics Infer(const BattlefieldSnapshot& snapshot,
                           const std::vector<EventRecord>& recent_events) const;

 private:
  static int CountEnemyOnLeftFlank(const BattlefieldSnapshot& snapshot);
  static int CountNearbyArmor(const BattlefieldSnapshot& snapshot, double range_m);
};

}  // namespace bas
