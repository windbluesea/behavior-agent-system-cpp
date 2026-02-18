#include <cstdlib>
#include <iostream>

#include "bas/decision/maneuver_engine.hpp"

int main() {
  bas::BattlefieldSnapshot snap;
  snap.timestamp_ms = 1000000;

  bas::EntityState f1;
  f1.id = "F-1";
  f1.side = bas::Side::Friendly;
  f1.type = bas::UnitType::Infantry;
  f1.pose = {0.0, 0.0, 0.0};
  snap.friendly_units.push_back(f1);

  bas::EntityState h1;
  h1.id = "H-1";
  h1.side = bas::Side::Hostile;
  h1.type = bas::UnitType::Armor;
  h1.pose = {100.0, 80.0, 0.0};
  h1.threat_level = 0.9;
  snap.hostile_units.push_back(h1);

  bas::ManeuverEngine engine({450.0, 80.0, 6});
  bas::ManeuverDecision decision = engine.Decide(snap, {});

  if (decision.actions.empty()) {
    std::cerr << "no maneuver action\n";
    return EXIT_FAILURE;
  }
  if (decision.actions.front().action_name != "emergency_evasion") {
    std::cerr << "expected emergency evasion\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
