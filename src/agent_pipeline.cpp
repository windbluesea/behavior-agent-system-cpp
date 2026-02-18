#include "bas/system/agent_pipeline.hpp"

#include <algorithm>
#include <sstream>

namespace bas {

AgentPipeline::AgentPipeline(PipelineConfig config,
                             FireControlEngine fire_engine,
                             ManeuverEngine maneuver_engine,
                             ModelRuntime model_runtime)
    : config_(config),
      memory_(config.memory_window_ms * 2),
      fire_engine_(std::move(fire_engine)),
      maneuver_engine_(std::move(maneuver_engine)),
      model_runtime_(std::move(model_runtime)),
      cache_(config.cache_ttl_ms) {}

DecisionPackage AgentPipeline::Tick(const BattlefieldSnapshot& snapshot, const std::vector<EventRecord>& dis_events) {
  cache_.Prune(snapshot.timestamp_ms);
  const std::string cache_key = BuildCacheKey(snapshot);

  if (const auto cached = cache_.Get(cache_key, snapshot.timestamp_ms); cached.has_value()) {
    DecisionPackage pkg = *cached;
    pkg.from_cache = true;
    return pkg;
  }

  memory_.AddEvents(dis_events);
  const auto recent_events = memory_.QueryRecent(snapshot.timestamp_ms, config_.memory_window_ms);

  const auto semantics = fusion_.Infer(snapshot, recent_events);
  for (const auto& tag : semantics.tags) {
    memory_.AddEvent({snapshot.timestamp_ms, EventType::TacticalTag, "fusion", {}, tag.name + ":" + tag.reason});
  }

  DecisionPackage pkg;
  pkg.fire = fire_engine_.Decide(snapshot, semantics, memory_);
  pkg.maneuver = maneuver_engine_.Decide(snapshot, semantics);

  const std::string memory_context = memory_.BuildContext(snapshot.timestamp_ms, config_.memory_window_ms);
  const std::vector<std::string> candidates = {
      "Candidate-A aggressive: " + pkg.fire.summary + ";" + pkg.maneuver.summary,
      "Candidate-B conservative: prioritize cover and defer long-range fire when confidence is low"};

  const ModelRequest request{memory_context, candidates};
  const ModelResponse model_response = model_runtime_.RankAndExplain(request);
  pkg.explanation = "selected_index=" + std::to_string(model_response.selected_index) + "; " + model_response.explanation;
  pkg.from_cache = false;

  cache_.Put(cache_key, pkg, snapshot.timestamp_ms);
  return pkg;
}

std::string AgentPipeline::BuildCacheKey(const BattlefieldSnapshot& snapshot) const {
  std::ostringstream oss;
  oss << "f=" << snapshot.friendly_units.size() << "|h=" << snapshot.hostile_units.size();
  oss << "|v=" << static_cast<int>(snapshot.env.visibility_m / 100.0);

  for (const auto& unit : snapshot.friendly_units) {
    oss << "|" << unit.id << "@" << static_cast<int>(unit.pose.x / 100.0) << "," << static_cast<int>(unit.pose.y / 100.0);
  }
  for (const auto& unit : snapshot.hostile_units) {
    oss << "|" << unit.id << "@" << static_cast<int>(unit.pose.x / 100.0) << "," << static_cast<int>(unit.pose.y / 100.0);
  }
  return oss.str();
}

}  // namespace bas
