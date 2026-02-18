#pragma once

#include <string>

#include "bas/cache/decision_cache.hpp"
#include "bas/decision/fire_control_engine.hpp"
#include "bas/decision/maneuver_engine.hpp"
#include "bas/inference/model_runtime.hpp"
#include "bas/memory/event_memory.hpp"
#include "bas/situation/situation_fusion.hpp"

namespace bas {

struct PipelineConfig {
  std::int64_t cache_ttl_ms = 3000;
  std::int64_t memory_window_ms = 5 * 60 * 1000;
};

class AgentPipeline {
 public:
  AgentPipeline(PipelineConfig config,
                FireControlEngine fire_engine,
                ManeuverEngine maneuver_engine,
                ModelRuntime model_runtime);

  DecisionPackage Tick(const BattlefieldSnapshot& snapshot, const std::vector<EventRecord>& dis_events);

 private:
  std::string BuildCacheKey(const BattlefieldSnapshot& snapshot) const;

  PipelineConfig config_;
  SituationFusion fusion_;
  EventMemory memory_;
  FireControlEngine fire_engine_;
  ManeuverEngine maneuver_engine_;
  ModelRuntime model_runtime_;
  DecisionCache cache_;
};

}  // namespace bas
