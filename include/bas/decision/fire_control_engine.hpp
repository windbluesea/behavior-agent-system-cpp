#pragma once

#include <unordered_map>

#include "bas/common/types.hpp"
#include "bas/memory/event_memory.hpp"

namespace bas {

struct FireControlConfig {
  bool enable_focus_fire = true;
  bool enable_stagger_fire = true;
  std::size_t max_shooters_per_target = 2;
  double focus_fire_threat_threshold = 78.0;
};

class FireControlEngine {
 public:
  explicit FireControlEngine(FireControlConfig config = {});

  FireDecision Decide(const BattlefieldSnapshot& snapshot,
                      const SituationSemantics& semantics,
                      const EventMemory& memory) const;

 private:
  static double TypeThreatWeight(UnitType type);
  static double ThreatIndex(const EntityState& target, double min_distance_m);
  static double WeaponFitScore(const WeaponState& weapon, const EntityState& shooter, const EntityState& target);

  FireControlConfig config_;
};

}  // namespace bas
