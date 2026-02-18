#pragma once

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace bas {

enum class Side { Friendly, Hostile, Neutral };

enum class UnitType { Infantry, Armor, Artillery, AirDefense, Unknown };

struct Pose {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

struct EntityState {
  std::string id;
  Side side = Side::Neutral;
  UnitType type = UnitType::Unknown;
  Pose pose;
  double speed_mps = 0.0;
  double threat_level = 0.0;
  bool alive = true;
};

struct EnvironmentState {
  double visibility_m = 1500.0;
  double weather_risk = 0.0;
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
};

struct SituationSemantics {
  std::vector<TacticalTag> tags;
};

struct EventRecord {
  std::int64_t timestamp_ms = 0;
  std::string message;
};

struct TargetAssignment {
  std::string shooter_id;
  std::string target_id;
  std::string weapon_name;
  double score = 0.0;
  std::string rationale;
};

struct FireDecision {
  std::vector<TargetAssignment> assignments;
  std::string summary;
};

struct ManeuverAction {
  std::string unit_id;
  std::string action_name;
  Pose next_pose;
  std::string rationale;
};

struct ManeuverDecision {
  std::vector<ManeuverAction> actions;
  std::string summary;
};

struct DecisionPackage {
  FireDecision fire;
  ManeuverDecision maneuver;
  std::string explanation;
};

inline double Distance(const Pose& a, const Pose& b) {
  const double dx = a.x - b.x;
  const double dy = a.y - b.y;
  const double dz = a.z - b.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
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
    default:
      return "unknown";
  }
}

}  // namespace bas
