#include "bas/decision/maneuver_engine.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <unordered_map>

namespace bas {

ManeuverEngine::ManeuverEngine(ManeuverConfig config) : config_(config) {}

ManeuverDecision ManeuverEngine::Decide(const BattlefieldSnapshot& snapshot,
                                        const SituationSemantics& semantics) const {
  ManeuverDecision out;
  if (snapshot.friendly_units.empty()) {
    out.summary = "机动动作数=0";
    return out;
  }

  const bool high_pressure = HasTag(semantics, "left_flank_exposed") || HasTag(semantics, "recent_enemy_artillery_activity");
  out.formation_mode = high_pressure ? "disperse" : "assemble";

  Pose centroid;
  for (const auto& unit : snapshot.friendly_units) {
    centroid.x += unit.pose.x;
    centroid.y += unit.pose.y;
    centroid.z += unit.pose.z;
  }
  centroid.x /= static_cast<double>(snapshot.friendly_units.size());
  centroid.y /= static_cast<double>(snapshot.friendly_units.size());
  centroid.z /= static_cast<double>(snapshot.friendly_units.size());

  for (const auto& unit : snapshot.friendly_units) {
    if (!unit.alive) {
      continue;
    }

    const EntityState* nearest = nullptr;
    double nearest_dist = std::numeric_limits<double>::infinity();
    for (const auto& enemy : snapshot.hostile_units) {
      const double d = Distance(unit.pose, enemy.pose);
      if (d < nearest_dist) {
        nearest_dist = d;
        nearest = &enemy;
      }
    }

    ManeuverAction action;
    action.unit_id = unit.id;

    if (nearest != nullptr && nearest_dist < config_.emergency_distance_m) {
      action.action_name = "emergency_evasion";
      action.next_pose = MoveAway(unit.pose, nearest->pose, config_.path_step_m * 1.5);
      action.path = {unit.pose, action.next_pose};
      action.rationale = "近距威胁触发紧急规避";
      out.actions.push_back(action);
      continue;
    }

    Pose goal = unit.pose;
    if (HasTag(semantics, "left_flank_exposed")) {
      goal.x -= 220.0;
      goal.y += 80.0;
      action.action_name = "flank_reinforce";
      action.rationale = "左翼受压，机动补位";
    } else if (HasTag(semantics, "enemy_armor_cluster_approaching")) {
      goal.y += 200.0;
      goal.x += 60.0;
      action.action_name = "occupy_advantageous_terrain";
      action.rationale = "抢占有利地形压制装甲集群";
    } else {
      goal.y += 160.0;
      action.action_name = "advance_bound";
      action.rationale = "威胁可控，实施跃进";
    }

    if (out.formation_mode == "disperse") {
      const Pose spread = MoveAway(unit.pose, centroid, 40.0);
      goal.x = (goal.x + spread.x) / 2.0;
      goal.y = (goal.y + spread.y) / 2.0;
    } else {
      goal.x = (goal.x * 0.8) + (centroid.x * 0.2);
      goal.y = (goal.y * 0.8) + (centroid.y * 0.2);
    }

    action.path = PlanPath(unit.pose, goal, snapshot);
    action.next_pose = action.path.empty() ? goal : action.path.back();
    out.actions.push_back(action);
  }

  out.summary = "机动动作数=" + std::to_string(out.actions.size()) +
                "，编队模式=" + (out.formation_mode == "disperse" ? "分散" : "集结");
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

double ManeuverEngine::ThreatField(const Pose& point, const BattlefieldSnapshot& snapshot) {
  double threat = 0.0;
  for (const auto& enemy : snapshot.hostile_units) {
    const double d = std::max(25.0, Distance(point, enemy.pose));
    threat += (enemy.threat_level * 120.0 + 20.0) / d;
    if (enemy.type == UnitType::Artillery) {
      threat += 12.0 / std::sqrt(d);
    }
  }
  threat += snapshot.env.terrain_risk * 5.0;
  return threat;
}

std::vector<Pose> ManeuverEngine::PlanPath(const Pose& start, const Pose& goal, const BattlefieldSnapshot& snapshot) const {
  std::vector<Pose> path;
  path.push_back(start);
  Pose current = start;

  static constexpr std::array<std::pair<double, double>, 8> kDirections = {{{1.0, 0.0},  {0.0, 1.0},  {-1.0, 0.0},
                                                                             {0.0, -1.0}, {0.7, 0.7}, {-0.7, 0.7},
                                                                             {-0.7, -0.7}, {0.7, -0.7}}};

  for (int step = 0; step < config_.path_horizon_steps; ++step) {
    Pose best_next = current;
    double best_cost = std::numeric_limits<double>::infinity();

    for (const auto& dir : kDirections) {
      Pose candidate{current.x + dir.first * config_.path_step_m, current.y + dir.second * config_.path_step_m,
                     current.z};
      const double goal_cost = Distance(candidate, goal) * 0.8;
      const double threat_cost = ThreatField(candidate, snapshot) * 35.0;
      const double smoothness_cost = Distance(candidate, current) * 0.2;
      const double total_cost = goal_cost + threat_cost + smoothness_cost;
      if (total_cost < best_cost) {
        best_cost = total_cost;
        best_next = candidate;
      }
    }

    current = best_next;
    path.push_back(current);
    if (Distance(current, goal) < config_.path_step_m) {
      break;
    }
  }

  if (Distance(path.back(), goal) > config_.path_step_m) {
    path.push_back(goal);
  }

  return path;
}

Pose ManeuverEngine::MoveAway(const Pose& self, const Pose& threat, double step) {
  const double dx = self.x - threat.x;
  const double dy = self.y - threat.y;
  const double norm = std::max(1.0, std::sqrt(dx * dx + dy * dy));
  Pose out = self;
  out.x += dx / norm * step;
  out.y += dy / norm * step;
  return out;
}

}  // namespace bas
