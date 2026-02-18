#include <cstdlib>
#include <iostream>

#include "bas/dis/dis_adapter.hpp"
#include "bas/inference/model_runtime.hpp"
#include "bas/system/agent_pipeline.hpp"

namespace {

bas::DisPduBatch BuildBatch(std::int64_t t) {
  bas::DisPduBatch batch;
  batch.env = bas::EnvironmentState{900.0, 0.1, 0.2};
  batch.entity_updates.push_back(
      {t, "F-1", bas::Side::Friendly, bas::UnitType::Armor, {0.0, 0.0, 0.0}, 5.0, 0.0, true, 0.4});
  batch.entity_updates.push_back(
      {t, "H-1", bas::Side::Hostile, bas::UnitType::Armor, {400.0, 120.0, 0.0}, 8.0, 180.0, true, 0.9});
  batch.fire_events.push_back({t - 60000, "H-1", "F-1", "howitzer", {400.0, 120.0, 0.0}});
  return batch;
}

}  // namespace

int main() {
  bas::DisAdapter adapter;
  const std::int64_t now_ms = 1000000;
  adapter.Ingest(BuildBatch(now_ms));

  const auto snapshot = adapter.Poll();
  if (!snapshot.has_value()) {
    std::cerr << "missing snapshot\n";
    return EXIT_FAILURE;
  }

  bas::ModelRuntime model;
  model.Configure({bas::ModelBackend::Mock, "Qwen1.5-1.8B-Chat", 128, true,
                   "http://127.0.0.1:8000/v1/chat/completions", "", 250});

  bas::AgentPipeline pipeline({3000, 5 * 60 * 1000}, bas::FireControlEngine{}, bas::ManeuverEngine{}, model);

  const bas::DecisionPackage first = pipeline.Tick(*snapshot, adapter.DrainEvents());
  if (first.from_cache || first.fire.assignments.empty() || first.maneuver.actions.empty()) {
    std::cerr << "first tick result invalid\n";
    return EXIT_FAILURE;
  }

  const bas::DecisionPackage second = pipeline.Tick(*snapshot, {});
  if (!second.from_cache) {
    std::cerr << "expected cache hit on second tick\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
