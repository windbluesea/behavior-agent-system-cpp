#include <cstdlib>
#include <fstream>
#include <iostream>

#include "bas/system/scenario_replay.hpp"

int main() {
  const std::string candidate_a = "data/scenarios/demo_replay.bas";
  const std::string candidate_b = "../data/scenarios/demo_replay.bas";
  const std::string replay_path = std::ifstream(candidate_a).good() ? candidate_a : candidate_b;

  bas::ScenarioReplayLoader loader;
  const auto batches = loader.LoadBatches(replay_path);

  if (batches.size() != 4) {
    std::cerr << "unexpected batch count: " << batches.size() << "\n";
    return EXIT_FAILURE;
  }

  if (batches.front().fire_events.size() != 1 || batches.front().entity_updates.size() != 0) {
    std::cerr << "first batch should contain only fire event\n";
    return EXIT_FAILURE;
  }

  if (!batches[1].env.has_value()) {
    std::cerr << "second batch missing env\n";
    return EXIT_FAILURE;
  }

  if (batches[1].entity_updates.size() != 4) {
    std::cerr << "second batch entity count mismatch\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
