#include "bas/decision/maneuver_engine.hpp"

namespace bas {

namespace {

Pose MoveAway(const Pose& self, const Pose& threat, double step) {
  const double dx = self.x - threat.x;
  const double dy = self.y - threat.y;
  const double norm = std::max(1.0, std::sqrt(dx * dx + dy * dy));
  Pose out = self;
  out.x += dx / norm * step;
  out.y += dy / norm * step;
  return out;
}

}  // namespace

ManeuverDecision ManeuverEngine::Decide(const BattlefieldSnapshot& snapshot,
                                        const SituationSemantics& semantics) const {
  ManeuverDecision out;
  for (const auto& unit : snapshot.friendly_units) {
    if (!unit.alive) {
      continue;
    }

    const EntityState* nearest = nullptr;
    double nearest_dist = 1e18;
    for (const auto& enemy : snapshot.hostile_units) {
      const double d = Distance(unit.pose, enemy.pose);
      if (d < nearest_dist) {
        nearest_dist = d;
        nearest = &enemy;
      }
    }

    ManeuverAction action;
    action.unit_id = unit.id;

    if (nearest != nullptr && nearest_dist < 800.0) {
      action.action_name = "emergency_evasion";
      action.next_pose = MoveAway(unit.pose, nearest->pose, 120.0);
      action.rationale = "close threat detected";
    } else if (HasTag(semantics, "left_flank_exposed")) {
      action.action_name = "flank_reinforce";
      action.next_pose = unit.pose;
      action.next_pose.x -= 80.0;
      action.rationale = "reinforce exposed flank";
    } else {
      action.action_name = "advance_bound";
      action.next_pose = unit.pose;
      action.next_pose.y += 60.0;
      action.rationale = "maintain pressure with bounded move";
    }

    out.actions.push_back(action);
  }

  out.summary = "maneuver_actions=" + std::to_string(out.actions.size());
  return out;
}

bool ManeuverEngine::HasTag(const SituationSemantics& semantics, const std::string& name) {
  for (const auto& tag : semantics.tags) {
    if (tag.name == name) {
      return true;
    }
  }
  return false;
}

}  // namespace bas
