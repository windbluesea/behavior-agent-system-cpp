#include "bas/decision/fire_control_engine.hpp"

#include <limits>

namespace bas {

namespace {

double WeaponRange(UnitType shooter) {
  switch (shooter) {
    case UnitType::Infantry:
      return 800.0;
    case UnitType::Armor:
      return 2500.0;
    case UnitType::Artillery:
      return 8000.0;
    case UnitType::AirDefense:
      return 3000.0;
    default:
      return 1000.0;
  }
}

const char* DefaultWeapon(UnitType shooter) {
  switch (shooter) {
    case UnitType::Infantry:
      return "rifle";
    case UnitType::Armor:
      return "tank_gun";
    case UnitType::Artillery:
      return "howitzer";
    case UnitType::AirDefense:
      return "sam";
    default:
      return "generic";
  }
}

}  // namespace

FireDecision FireControlEngine::Decide(const BattlefieldSnapshot& snapshot,
                                       const SituationSemantics&,
                                       const EventMemory&) const {
  FireDecision out;
  for (const auto& shooter : snapshot.friendly_units) {
    if (!shooter.alive) {
      continue;
    }

    const double range = WeaponRange(shooter.type);
    const char* weapon = DefaultWeapon(shooter.type);

    const EntityState* best_target = nullptr;
    double best_score = -std::numeric_limits<double>::infinity();

    for (const auto& target : snapshot.hostile_units) {
      if (!target.alive) {
        continue;
      }
      const double d = Distance(shooter.pose, target.pose);
      if (d > range) {
        continue;
      }

      const double score = TypeThreatWeight(target.type) * 0.55 +
                           (1.0 / (1.0 + d)) * 1000.0 * 0.25 +
                           target.threat_level * 0.20;
      if (score > best_score) {
        best_score = score;
        best_target = &target;
      }
    }

    if (best_target == nullptr) {
      continue;
    }

    TargetAssignment a;
    a.shooter_id = shooter.id;
    a.target_id = best_target->id;
    a.weapon_name = weapon;
    a.score = best_score;
    a.rationale = "highest weighted threat in effective range";
    out.assignments.push_back(a);
  }

  out.summary = "fire_assignments=" + std::to_string(out.assignments.size());
  return out;
}

double FireControlEngine::TypeThreatWeight(UnitType type) {
  switch (type) {
    case UnitType::Armor:
      return 95.0;
    case UnitType::Artillery:
      return 90.0;
    case UnitType::AirDefense:
      return 80.0;
    case UnitType::Infantry:
      return 55.0;
    default:
      return 40.0;
  }
}

}  // namespace bas
