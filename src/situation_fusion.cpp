#include "bas/situation/situation_fusion.hpp"

#include <algorithm>

namespace bas {

SituationSemantics SituationFusion::Infer(const BattlefieldSnapshot& snapshot) const {
  SituationSemantics semantics;
  if (snapshot.friendly_units.empty() || snapshot.hostile_units.empty()) {
    semantics.tags.push_back({"insufficient_contact", 1.0});
    return semantics;
  }

  const auto min_x_it = std::min_element(snapshot.friendly_units.begin(), snapshot.friendly_units.end(),
                                         [](const EntityState& a, const EntityState& b) {
                                           return a.pose.x < b.pose.x;
                                         });
  int left_flank_threats = 0;
  int armor_near = 0;

  for (const auto& enemy : snapshot.hostile_units) {
    if (enemy.pose.x < min_x_it->pose.x + 250.0) {
      ++left_flank_threats;
    }
    if (enemy.type == UnitType::Armor) {
      for (const auto& friendly : snapshot.friendly_units) {
        if (Distance(enemy.pose, friendly.pose) < 2000.0) {
          ++armor_near;
          break;
        }
      }
    }
  }

  if (left_flank_threats > 0) {
    semantics.tags.push_back({"left_flank_exposed", std::min(1.0, left_flank_threats / 3.0)});
  }
  if (armor_near > 0) {
    semantics.tags.push_back({"enemy_armor_cluster_approaching", std::min(1.0, armor_near / 2.0)});
  }
  if (snapshot.env.visibility_m < 700.0) {
    semantics.tags.push_back({"low_visibility", 0.8});
  }

  if (semantics.tags.empty()) {
    semantics.tags.push_back({"stable_contact", 0.6});
  }
  return semantics;
}

}  // namespace bas
