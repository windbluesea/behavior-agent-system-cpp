#pragma once

#include <optional>
#include <unordered_map>
#include <vector>

#include "bas/common/types.hpp"

namespace bas {

struct DisEntityPdu {
  std::int64_t timestamp_ms = 0;
  std::string entity_id;
  Side side = Side::Neutral;
  UnitType type = UnitType::Unknown;
  Pose pose;
  double speed_mps = 0.0;
  double heading_deg = 0.0;
  bool alive = true;
  double threat_level = 0.0;
};

struct DisFirePdu {
  std::int64_t timestamp_ms = 0;
  std::string shooter_id;
  std::string target_id;
  std::string weapon_name;
  Pose origin;
};

struct DisPduBatch {
  std::vector<DisEntityPdu> entity_updates;
  std::vector<DisFirePdu> fire_events;
  std::optional<EnvironmentState> env;
};

class DisAdapter {
 public:
  void FeedMockFrame(const BattlefieldSnapshot& snapshot);
  void Ingest(const DisPduBatch& batch);
  std::optional<BattlefieldSnapshot> Poll();
  std::vector<EventRecord> DrainEvents();

 private:
  void UpsertEntity(const DisEntityPdu& pdu);
  BattlefieldSnapshot BuildSnapshot(std::int64_t timestamp_ms) const;

  std::unordered_map<std::string, EntityState> entities_;
  EnvironmentState env_;
  std::int64_t latest_timestamp_ms_ = 0;
  bool has_update_ = false;
  std::vector<EventRecord> buffered_events_;
};

}  // namespace bas
