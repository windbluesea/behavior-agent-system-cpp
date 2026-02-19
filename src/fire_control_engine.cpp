#include "bas/decision/fire_control_engine.hpp"

#include <algorithm>
#include <limits>
#include <unordered_set>

namespace bas {

namespace {

double PreferredTargetBonus(const WeaponState& weapon, UnitType target_type) {
  return IsPreferredTarget(weapon, target_type) ? 1.15 : 0.85;
}

double MinDistanceToFriendlies(const EntityState& target, const std::vector<EntityState>& friendlies) {
  double min_distance = std::numeric_limits<double>::infinity();
  for (const auto& friendly : friendlies) {
    min_distance = std::min(min_distance, Distance(friendly.pose, target.pose));
  }
  return std::isfinite(min_distance) ? min_distance : 99999.0;
}

}  // namespace

FireControlEngine::FireControlEngine(FireControlConfig config) : config_(config) {}

FireDecision FireControlEngine::Decide(const BattlefieldSnapshot& snapshot,
                                       const SituationSemantics&,
                                       const EventMemory& memory) const {
  FireDecision out;
  if (snapshot.friendly_units.empty() || snapshot.hostile_units.empty()) {
    out.summary = "火力分配数=0";
    return out;
  }

  std::unordered_map<std::string, double> threat_by_target;
  for (const auto& target : snapshot.hostile_units) {
    if (!target.alive) {
      continue;
    }
    const double min_distance = MinDistanceToFriendlies(target, snapshot.friendly_units);
    const double threat_index = ThreatIndex(target, min_distance);
    threat_by_target[target.id] = threat_index;
    out.threats.push_back({target.id, threat_index,
                           std::string("类型=") + UnitTypeToString(target.type) + "，距离=" +
                               std::to_string(static_cast<int>(min_distance)) + "米"});
  }

  std::sort(out.threats.begin(), out.threats.end(), [](const ThreatEstimate& a, const ThreatEstimate& b) {
    return a.index > b.index;
  });

  std::unordered_map<std::string, std::size_t> assigned_shooters_per_target;
  for (const auto& shooter : snapshot.friendly_units) {
    if (!shooter.alive || shooter.weapons.empty()) {
      continue;
    }

    const EntityState* best_target = nullptr;
    const WeaponState* best_weapon = nullptr;
    double best_score = -std::numeric_limits<double>::infinity();

    for (const auto& target : snapshot.hostile_units) {
      if (!target.alive) {
        continue;
      }
      for (const auto& weapon : shooter.weapons) {
        const double shot_score = WeaponFitScore(weapon, shooter, target) * threat_by_target[target.id];
        if (shot_score > best_score) {
          best_score = shot_score;
          best_target = &target;
          best_weapon = &weapon;
        }
      }
    }

    if (best_target == nullptr || best_weapon == nullptr || best_score <= 0.0) {
      continue;
    }

    TargetAssignment a;
    a.shooter_id = shooter.id;
    a.target_id = best_target->id;
    a.weapon_name = best_weapon->name;
    a.score = best_score;
    a.expected_kill_prob = best_weapon->kill_probability;
    a.rationale = "当前配置下可获得最高威胁压制收益";
    out.assignments.push_back(a);
    ++assigned_shooters_per_target[a.target_id];
  }

  if (config_.enable_focus_fire && !out.threats.empty() && out.threats.front().index >= config_.focus_fire_threat_threshold) {
    const std::string& priority_target = out.threats.front().target_id;
    for (auto& assignment : out.assignments) {
      if (assigned_shooters_per_target[priority_target] >= config_.max_shooters_per_target) {
        break;
      }
      if (assignment.target_id == priority_target) {
        continue;
      }
      assignment.target_id = priority_target;
      assignment.tactic = "focus_fire";
      assignment.rationale = "首要目标威胁超阈值，执行集火";
      ++assigned_shooters_per_target[priority_target];
    }
  }

  if (config_.enable_stagger_fire) {
    std::sort(out.assignments.begin(), out.assignments.end(), [](const TargetAssignment& a, const TargetAssignment& b) {
      return a.score > b.score;
    });
    for (std::size_t i = 0; i < out.assignments.size(); ++i) {
      out.assignments[i].scheduled_offset_s = static_cast<double>(i) * 1.25;
      if (out.assignments[i].tactic == "single_shot") {
        out.assignments[i].tactic = "stagger_fire";
      }
    }
  }

  const auto recent_fire = memory.LastEventByType(EventType::WeaponFire, snapshot.timestamp_ms, 5 * 60 * 1000);
  out.summary = "火力分配数=" + std::to_string(out.assignments.size()) +
                "，最高威胁目标=" + (out.threats.empty() ? std::string("无") : out.threats.front().target_id) +
                "，近期火力记忆=" + (recent_fire.has_value() ? std::string("有") : std::string("无"));
  return out;
}

double FireControlEngine::TypeThreatWeight(UnitType type) {
  switch (type) {
    case UnitType::Armor:
      return 95.0;
    case UnitType::Artillery:
      return 92.0;
    case UnitType::Command:
      return 88.0;
    case UnitType::AirDefense:
      return 80.0;
    case UnitType::Infantry:
      return 55.0;
    default:
      return 40.0;
  }
}

double FireControlEngine::ThreatIndex(const EntityState& target, double min_distance_m) {
  const double type_term = TypeThreatWeight(target.type) * 0.50;
  const double proximity_term = (1.0 / (1.0 + min_distance_m)) * 1000.0 * 0.25;
  const double speed_term = std::min(20.0, target.speed_mps) * 1.2;
  const double explicit_term = std::clamp(target.threat_level, 0.0, 1.0) * 25.0;
  return type_term + proximity_term + speed_term + explicit_term;
}

double FireControlEngine::WeaponFitScore(const WeaponState& weapon,
                                         const EntityState& shooter,
                                         const EntityState& target) {
  if (weapon.ammo <= 0 || weapon.ready_in_s > 0.0) {
    return -1.0;
  }

  const double distance = Distance(shooter.pose, target.pose);
  if (distance > weapon.range_m || weapon.range_m <= 0.0) {
    return -1.0;
  }

  const double range_factor = 1.0 - (distance / weapon.range_m) * 0.6;
  const double preference = PreferredTargetBonus(weapon, target.type);
  const double weapon_quality = std::clamp(weapon.kill_probability, 0.0, 1.0);
  return std::max(0.0, range_factor * preference * (0.6 + weapon_quality));
}

}  // namespace bas
