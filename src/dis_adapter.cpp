#include "bas/dis/dis_adapter.hpp"

#include <algorithm>

namespace bas {

void DisAdapter::FeedMockFrame(const BattlefieldSnapshot& snapshot) {
  entities_.clear();
  for (const auto& unit : snapshot.friendly_units) {
    entities_[unit.id] = unit;
  }
  for (const auto& unit : snapshot.hostile_units) {
    entities_[unit.id] = unit;
  }
  env_ = snapshot.env;
  latest_timestamp_ms_ = snapshot.timestamp_ms;
  has_update_ = true;
}

void DisAdapter::Ingest(const DisPduBatch& batch) {
  for (const auto& pdu : batch.entity_updates) {
    UpsertEntity(pdu);
    latest_timestamp_ms_ = std::max(latest_timestamp_ms_, pdu.timestamp_ms);
  }

  for (const auto& fire : batch.fire_events) {
    latest_timestamp_ms_ = std::max(latest_timestamp_ms_, fire.timestamp_ms);
    buffered_events_.push_back(
        {fire.timestamp_ms, EventType::WeaponFire, fire.shooter_id, fire.origin,
         "武器=" + fire.weapon_name + "，目标=" + fire.target_id});
  }

  if (batch.env.has_value()) {
    env_ = *batch.env;
  }
  has_update_ = has_update_ || !batch.entity_updates.empty() || !batch.fire_events.empty() || batch.env.has_value();
}

std::optional<BattlefieldSnapshot> DisAdapter::Poll() {
  if (!has_update_) {
    return std::nullopt;
  }
  has_update_ = false;
  return BuildSnapshot(latest_timestamp_ms_);
}

std::vector<EventRecord> DisAdapter::DrainEvents() {
  auto events = buffered_events_;
  buffered_events_.clear();
  return events;
}

void DisAdapter::UpsertEntity(const DisEntityPdu& pdu) {
  EntityState& state = entities_[pdu.entity_id];
  state.id = pdu.entity_id;
  state.side = pdu.side;
  state.type = pdu.type;
  state.pose = pdu.pose;
  state.speed_mps = pdu.speed_mps;
  state.heading_deg = pdu.heading_deg;
  state.alive = pdu.alive;
  state.threat_level = pdu.threat_level;

  if (state.weapons.empty()) {
    // 默认武器配置：即使未加载外部表，也能保障决策链路运行。
    switch (pdu.type) {
      case UnitType::Infantry:
        state.weapons.push_back({"rifle", 800.0, 0.25, 200, 0.0, {UnitType::Infantry}});
        break;
      case UnitType::Armor:
        state.weapons.push_back({"tank_gun", 2500.0, 0.65, 30, 0.0,
                                 {UnitType::Armor, UnitType::Artillery, UnitType::Command}});
        break;
      case UnitType::Artillery:
        state.weapons.push_back({"howitzer", 8000.0, 0.55, 20, 0.0,
                                 {UnitType::Armor, UnitType::Artillery, UnitType::Command}});
        break;
      case UnitType::AirDefense:
        state.weapons.push_back({"sam", 3500.0, 0.60, 12, 0.0, {UnitType::AirDefense}});
        break;
      default:
        state.weapons.push_back({"generic", 1000.0, 0.20, 50, 0.0, {}});
        break;
    }
  }
}

BattlefieldSnapshot DisAdapter::BuildSnapshot(std::int64_t timestamp_ms) const {
  BattlefieldSnapshot snapshot;
  snapshot.timestamp_ms = timestamp_ms;
  snapshot.env = env_;

  for (const auto& [_, entity] : entities_) {
    if (entity.side == Side::Friendly) {
      snapshot.friendly_units.push_back(entity);
    } else if (entity.side == Side::Hostile) {
      snapshot.hostile_units.push_back(entity);
    }
  }

  return snapshot;
}

}  // namespace bas
