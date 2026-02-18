#pragma once

#include "bas/common/types.hpp"

namespace bas {

class SituationFusion {
 public:
  SituationSemantics Infer(const BattlefieldSnapshot& snapshot) const;
};

}  // namespace bas
