#include <cstdlib>
#include <fstream>
#include <iostream>

#include "bas/dis/dis_adapter.hpp"
#include "bas/inference/model_runtime.hpp"
#include "bas/system/agent_pipeline.hpp"
#include "bas/system/scenario_replay.hpp"

int main() {
  const std::string candidate_a = "data/scenarios/demo_replay.bas";
  const std::string candidate_b = "../data/scenarios/demo_replay.bas";
  const std::string replay_path = std::ifstream(candidate_a).good() ? candidate_a : candidate_b;

  bas::ScenarioReplayLoader loader;
  const auto batches = loader.LoadBatches(replay_path);
  if (batches.empty()) {
    std::cerr << "empty replay\n";
    return EXIT_FAILURE;
  }

  bas::ModelRuntime model;
  model.Configure({bas::ModelBackend::Mock, "Qwen1.5-1.8B-Chat", 128, true,
                   "http://127.0.0.1:8000/v1/chat/completions", "", 250});

  bas::AgentPipeline pipeline({3000, 5 * 60 * 1000}, bas::FireControlEngine{}, bas::ManeuverEngine{}, model);
  bas::DisAdapter adapter;

  int decisions = 0;
  int valid_tactical_decisions = 0;
  for (const auto& batch : batches) {
    adapter.Ingest(batch);
    const auto snapshot = adapter.Poll();
    if (!snapshot.has_value()) {
      continue;
    }

    const auto decision = pipeline.Tick(*snapshot, adapter.DrainEvents());
    if (!decision.fire.assignments.empty() && !decision.maneuver.actions.empty()) {
      ++valid_tactical_decisions;
    }
    ++decisions;
  }

  if (decisions < 3) {
    std::cerr << "too few decisions: " << decisions << "\n";
    return EXIT_FAILURE;
  }
  if (valid_tactical_decisions < 2) {
    std::cerr << "too few valid tactical decisions: " << valid_tactical_decisions << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
