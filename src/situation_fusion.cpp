#include "bas/situation/situation_fusion.hpp"

#include <algorithm>

namespace bas {

SituationSemantics SituationFusion::Infer(const BattlefieldSnapshot& snapshot,
                                          const std::vector<EventRecord>& recent_events) const {
  SituationSemantics semantics;
  if (snapshot.friendly_units.empty() || snapshot.hostile_units.empty()) {
    semantics.tags.push_back({"insufficient_contact", 1.0, "missing friendly or hostile tracks"});
    return semantics;
  }

  const int left_flank_threats = CountEnemyOnLeftFlank(snapshot);
  const int nearby_armor = CountNearbyArmor(snapshot, 2200.0);

  if (left_flank_threats > 0) {
    semantics.tags.push_back({"left_flank_exposed", std::min(1.0, left_flank_threats / 3.0),
                              "hostile concentration detected on left boundary"});
  }

  if (nearby_armor >= 2) {
    semantics.tags.push_back({"enemy_armor_cluster_approaching", std::min(1.0, nearby_armor / 4.0),
                              "multiple armor tracks in engagement envelope"});
  }

  if (snapshot.env.visibility_m < 700.0) {
    semantics.tags.push_back({"low_visibility", 0.85, "visual range below 700m"});
  }

  const bool recent_artillery_fire = std::any_of(recent_events.begin(), recent_events.end(), [](const EventRecord& ev) {
    return ev.type == EventType::WeaponFire && ev.message.find("howitzer") != std::string::npos;
  });
  if (recent_artillery_fire) {
    semantics.tags.push_back({"recent_enemy_artillery_activity", 0.75, "enemy artillery fire seen in memory window"});
  }

  if (semantics.tags.empty()) {
    semantics.tags.push_back({"stable_contact", 0.60, "no abnormal pressure indicators"});
  }

  return semantics;
}

int SituationFusion::CountEnemyOnLeftFlank(const BattlefieldSnapshot& snapshot) {
  const auto min_x_it = std::min_element(snapshot.friendly_units.begin(), snapshot.friendly_units.end(),
                                         [](const EntityState& a, const EntityState& b) {
                                           return a.pose.x < b.pose.x;
                                         });
  const double left_boundary = min_x_it->pose.x + 200.0;
  int count = 0;
  for (const auto& enemy : snapshot.hostile_units) {
    if (enemy.pose.x < left_boundary) {
      ++count;
    }
  }
  return count;
}

int SituationFusion::CountNearbyArmor(const BattlefieldSnapshot& snapshot, double range_m) {
  int count = 0;
  for (const auto& enemy : snapshot.hostile_units) {
    if (enemy.type != UnitType::Armor) {
      continue;
    }
    const bool in_range = std::any_of(snapshot.friendly_units.begin(), snapshot.friendly_units.end(),
                                      [&](const EntityState& friendly) {
                                        return Distance(enemy.pose, friendly.pose) <= range_m;
                                      });
    if (in_range) {
      ++count;
    }
  }
  return count;
}

}  // namespace bas
