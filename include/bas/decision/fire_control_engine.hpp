#pragma once

#include "bas/common/types.hpp"
#include "bas/memory/event_memory.hpp"

namespace bas {

class FireControlEngine {
 public:
  FireDecision Decide(const BattlefieldSnapshot& snapshot,
                      const SituationSemantics& semantics,
                      const EventMemory& memory) const;

 private:
  static double TypeThreatWeight(UnitType type);
};

}  // namespace bas
