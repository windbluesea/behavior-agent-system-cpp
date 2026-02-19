#include "bas/situation/situation_fusion.hpp"

#include <algorithm>

namespace bas {

SituationSemantics SituationFusion::Infer(const BattlefieldSnapshot& snapshot,
                                          const std::vector<EventRecord>& recent_events) const {
  SituationSemantics semantics;
  if (snapshot.friendly_units.empty() || snapshot.hostile_units.empty()) {
    semantics.tags.push_back({"insufficient_contact", 1.0, "缺少敌我有效接触信息"});
    return semantics;
  }

  const int left_flank_threats = CountEnemyOnLeftFlank(snapshot);
  const int nearby_armor = CountNearbyArmor(snapshot, 2200.0);

  if (left_flank_threats > 0) {
    semantics.tags.push_back({"left_flank_exposed", std::min(1.0, left_flank_threats / 3.0),
                              "左翼边界出现敌方集中态势"});
  }

  if (nearby_armor >= 2) {
    semantics.tags.push_back({"enemy_armor_cluster_approaching", std::min(1.0, nearby_armor / 4.0),
                              "交战范围内出现多条装甲目标轨迹"});
  }

  if (snapshot.env.visibility_m < 700.0) {
    semantics.tags.push_back({"low_visibility", 0.85, "可视距离低于700米"});
  }

  const bool recent_artillery_fire = std::any_of(recent_events.begin(), recent_events.end(), [](const EventRecord& ev) {
    return ev.type == EventType::WeaponFire && ev.message.find("howitzer") != std::string::npos;
  });
  if (recent_artillery_fire) {
    semantics.tags.push_back({"recent_enemy_artillery_activity", 0.75, "记忆窗口内出现敌方炮兵火力活动"});
  }

  if (semantics.tags.empty()) {
    semantics.tags.push_back({"stable_contact", 0.60, "当前未发现异常战术压力"});
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
