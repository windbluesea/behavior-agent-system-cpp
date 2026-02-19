#include <cmath>
#include <cstdlib>
#include <iostream>

#include "bas/system/replay_metrics.hpp"

int main() {
  bas::ReplayMetricsEvaluator evaluator(120000);

  bas::BattlefieldSnapshot s1;
  s1.timestamp_ms = 1000;
  s1.friendly_units.push_back({"F-1", bas::Side::Friendly, bas::UnitType::Armor, {}, 0.0, 0.0, 0.3, true});
  s1.friendly_units.push_back({"F-2", bas::Side::Friendly, bas::UnitType::Infantry, {}, 0.0, 0.0, 0.2, true});
  s1.hostile_units.push_back({"H-1", bas::Side::Hostile, bas::UnitType::Armor, {}, 0.0, 0.0, 0.8, true});
  evaluator.ObserveSnapshot(s1);

  bas::DecisionPackage d1;
  d1.fire.assignments.push_back({"F-1", "H-1", "tank_gun", 0.0, 0.0, 0.0, "stagger_fire", ""});
  d1.fire.assignments.push_back({"F-2", "H-1", "rifle", 0.0, 0.0, 0.0, "focus_fire", ""});
  evaluator.ObserveDecision(1000, d1);

  bas::BattlefieldSnapshot s2;
  s2.timestamp_ms = 3000;
  s2.friendly_units.push_back({"F-1", bas::Side::Friendly, bas::UnitType::Armor, {}, 0.0, 0.0, 0.3, true});
  s2.friendly_units.push_back({"F-2", bas::Side::Friendly, bas::UnitType::Infantry, {}, 0.0, 0.0, 0.2, false});
  s2.hostile_units.push_back({"H-1", bas::Side::Hostile, bas::UnitType::Armor, {}, 0.0, 0.0, 0.8, false});
  evaluator.ObserveSnapshot(s2);

  const bas::ReplayMetricsResult result = evaluator.Finalize();

  if (result.initial_friendly_count != 2 || result.final_friendly_alive != 1) {
    std::cerr << "friendly counts mismatch\n";
    return EXIT_FAILURE;
  }

  if (std::fabs(result.survival_rate - 50.0) > 1e-6) {
    std::cerr << "survival rate mismatch\n";
    return EXIT_FAILURE;
  }

  if (result.total_hostile_losses != 1 || std::fabs(result.hit_contribution_rate - 100.0) > 1e-6) {
    std::cerr << "hostile loss or contribution mismatch\n";
    return EXIT_FAILURE;
  }

  const auto it_f1 = result.shooter_kill_contribution.find("F-1");
  const auto it_f2 = result.shooter_kill_contribution.find("F-2");
  if (it_f1 == result.shooter_kill_contribution.end() || it_f2 == result.shooter_kill_contribution.end()) {
    std::cerr << "missing shooter credit\n";
    return EXIT_FAILURE;
  }
  if (std::fabs(it_f1->second - 0.5) > 1e-6 || std::fabs(it_f2->second - 0.5) > 1e-6) {
    std::cerr << "unexpected shooter credit distribution\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
