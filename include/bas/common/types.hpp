#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace bas {

enum class Side { Friendly, Hostile, Neutral };

enum class UnitType { Infantry, Armor, Artillery, AirDefense, Command, Unknown };

enum class EventType { WeaponFire, SensorContact, TacticalTag, UnitLoss, Unknown };

struct Pose {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

struct WeaponState {
  std::string name;
  double range_m = 0.0;
  double kill_probability = 0.0;
  int ammo = 0;
  double ready_in_s = 0.0;
  std::vector<UnitType> preferred_targets;
};

struct EntityState {
  std::string id;
  Side side = Side::Neutral;
  UnitType type = UnitType::Unknown;
  Pose pose;
  double speed_mps = 0.0;
  double heading_deg = 0.0;
  double threat_level = 0.0;
  bool alive = true;
  std::string formation_group = "default";
  std::vector<WeaponState> weapons;
};

struct EnvironmentState {
  double visibility_m = 1500.0;
  double weather_risk = 0.0;
  double terrain_risk = 0.0;
};

struct BattlefieldSnapshot {
  std::int64_t timestamp_ms = 0;
  std::vector<EntityState> friendly_units;
  std::vector<EntityState> hostile_units;
  EnvironmentState env;
};

struct TacticalTag {
  std::string name;
  double confidence = 0.0;
  std::string reason;
};

struct SituationSemantics {
  std::vector<TacticalTag> tags;
};

struct EventRecord {
  std::int64_t timestamp_ms = 0;
  EventType type = EventType::Unknown;
  std::string actor_id;
  Pose pose;
  std::string message;
};

struct ThreatEstimate {
  std::string target_id;
  double index = 0.0;
  std::string reason;
};

struct TargetAssignment {
  std::string shooter_id;
  std::string target_id;
  std::string weapon_name;
  double score = 0.0;
  double expected_kill_prob = 0.0;
  double scheduled_offset_s = 0.0;
  std::string tactic = "single_shot";
  std::string rationale;
};

struct FireDecision {
  std::vector<ThreatEstimate> threats;
  std::vector<TargetAssignment> assignments;
  std::string summary;
};

struct ManeuverAction {
  std::string unit_id;
  std::string action_name;
  std::vector<Pose> path;
  Pose next_pose;
  std::string rationale;
};

struct ManeuverDecision {
  std::vector<ManeuverAction> actions;
  std::string formation_mode;
  std::string summary;
};

struct DecisionPackage {
  FireDecision fire;
  ManeuverDecision maneuver;
  std::string explanation;
  bool from_cache = false;
};

inline double Distance(const Pose& a, const Pose& b) {
  const double dx = a.x - b.x;
  const double dy = a.y - b.y;
  const double dz = a.z - b.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

inline UnitType UnitTypeFromString(const std::string& text) {
  if (text == "infantry") {
    return UnitType::Infantry;
  }
  if (text == "armor") {
    return UnitType::Armor;
  }
  if (text == "artillery") {
    return UnitType::Artillery;
  }
  if (text == "air_defense") {
    return UnitType::AirDefense;
  }
  if (text == "command") {
    return UnitType::Command;
  }
  return UnitType::Unknown;
}

inline Side SideFromString(const std::string& text) {
  if (text == "friendly") {
    return Side::Friendly;
  }
  if (text == "hostile") {
    return Side::Hostile;
  }
  return Side::Neutral;
}

inline const char* SideToString(Side side) {
  switch (side) {
    case Side::Friendly:
      return "friendly";
    case Side::Hostile:
      return "hostile";
    default:
      return "neutral";
  }
}

inline const char* UnitTypeToString(UnitType type) {
  switch (type) {
    case UnitType::Infantry:
      return "infantry";
    case UnitType::Armor:
      return "armor";
    case UnitType::Artillery:
      return "artillery";
    case UnitType::AirDefense:
      return "air_defense";
    case UnitType::Command:
      return "command";
    default:
      return "unknown";
  }
}

inline const char* EventTypeToString(EventType type) {
  switch (type) {
    case EventType::WeaponFire:
      return "weapon_fire";
    case EventType::SensorContact:
      return "sensor_contact";
    case EventType::TacticalTag:
      return "tactical_tag";
    case EventType::UnitLoss:
      return "unit_loss";
    default:
      return "unknown";
  }
}

inline bool IsPreferredTarget(const WeaponState& weapon, UnitType target) {
  if (weapon.preferred_targets.empty()) {
    return true;
  }
  return std::find(weapon.preferred_targets.begin(), weapon.preferred_targets.end(), target) !=
         weapon.preferred_targets.end();
}

}  // namespace bas
