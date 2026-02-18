#pragma once

#include "bas/common/types.hpp"

namespace bas {

struct ManeuverConfig {
  double emergency_distance_m = 450.0;
  double path_step_m = 80.0;
  int path_horizon_steps = 8;
};

class ManeuverEngine {
 public:
  explicit ManeuverEngine(ManeuverConfig config = {});

  ManeuverDecision Decide(const BattlefieldSnapshot& snapshot,
                          const SituationSemantics& semantics) const;

 private:
  static bool HasTag(const SituationSemantics& semantics, const std::string& name);
  static double ThreatField(const Pose& point, const BattlefieldSnapshot& snapshot);
  std::vector<Pose> PlanPath(const Pose& start, const Pose& goal, const BattlefieldSnapshot& snapshot) const;
  static Pose MoveAway(const Pose& self, const Pose& threat, double step);

  ManeuverConfig config_;
};

}  // namespace bas
