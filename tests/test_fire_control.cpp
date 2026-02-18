#include <cstdlib>
#include <iostream>

#include "bas/decision/fire_control_engine.hpp"

namespace {

bas::BattlefieldSnapshot BuildSnapshot() {
  bas::BattlefieldSnapshot snap;
  snap.timestamp_ms = 1000000;

  bas::EntityState f1;
  f1.id = "F-1";
  f1.side = bas::Side::Friendly;
  f1.type = bas::UnitType::Armor;
  f1.pose = {0.0, 0.0, 0.0};
  f1.weapons.push_back({"tank_gun", 2500.0, 0.7, 10, 0.0, {bas::UnitType::Armor, bas::UnitType::Artillery}});
  snap.friendly_units.push_back(f1);

  bas::EntityState f2 = f1;
  f2.id = "F-2";
  f2.type = bas::UnitType::Infantry;
  f2.pose = {-50.0, -30.0, 0.0};
  f2.weapons = {{"rifle", 800.0, 0.25, 100, 0.0, {bas::UnitType::Infantry}}};
  snap.friendly_units.push_back(f2);

  bas::EntityState h1;
  h1.id = "H-armor";
  h1.side = bas::Side::Hostile;
  h1.type = bas::UnitType::Armor;
  h1.pose = {500.0, 120.0, 0.0};
  h1.speed_mps = 10.0;
  h1.threat_level = 0.95;
  snap.hostile_units.push_back(h1);

  bas::EntityState h2;
  h2.id = "H-inf";
  h2.side = bas::Side::Hostile;
  h2.type = bas::UnitType::Infantry;
  h2.pose = {600.0, 200.0, 0.0};
  h2.speed_mps = 3.0;
  h2.threat_level = 0.2;
  snap.hostile_units.push_back(h2);

  return snap;
}

}  // namespace

int main() {
  bas::BattlefieldSnapshot snap = BuildSnapshot();
  bas::EventMemory memory;
  memory.AddEvent({snap.timestamp_ms - 20000, bas::EventType::WeaponFire, "H-armor", {}, "howitzer"});

  bas::FireControlEngine engine({true, true, 2, 70.0});
  bas::FireDecision decision = engine.Decide(snap, {}, memory);

  if (decision.assignments.empty()) {
    std::cerr << "no fire assignments\n";
    return EXIT_FAILURE;
  }
  if (decision.threats.empty() || decision.threats.front().target_id != "H-armor") {
    std::cerr << "threat ranking incorrect\n";
    return EXIT_FAILURE;
  }

  bool has_focus_or_stagger = false;
  for (const auto& a : decision.assignments) {
    if (a.tactic == "focus_fire" || a.tactic == "stagger_fire") {
      has_focus_or_stagger = true;
      break;
    }
  }
  if (!has_focus_or_stagger) {
    std::cerr << "coordination tactic missing\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
