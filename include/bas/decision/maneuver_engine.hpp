#pragma once

#include "bas/common/types.hpp"

namespace bas {

class ManeuverEngine {
 public:
  ManeuverDecision Decide(const BattlefieldSnapshot& snapshot,
                          const SituationSemantics& semantics) const;

 private:
  static bool HasTag(const SituationSemantics& semantics, const std::string& name);
};

}  // namespace bas
