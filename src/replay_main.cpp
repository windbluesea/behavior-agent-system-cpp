#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include "bas/dis/dis_binary_parser.hpp"
#include "bas/dis/dis_adapter.hpp"
#include "bas/inference/model_runtime.hpp"
#include "bas/system/agent_pipeline.hpp"
#include "bas/system/replay_metrics.hpp"
#include "bas/system/scenario_replay.hpp"

namespace {

bas::ModelBackend ResolveBackend() {
  const char* backend_env = std::getenv("BAS_MODEL_BACKEND");
  if (backend_env != nullptr && std::string(backend_env) == "openai") {
    return bas::ModelBackend::OpenAICompatible;
  }
  return bas::ModelBackend::Mock;
}

bool IsBinaryReplay(const std::string& path) {
  const auto pos = path.find_last_of('.');
  if (pos == std::string::npos) {
    return false;
  }
  const std::string ext = path.substr(pos + 1);
  return ext == "bin" || ext == "dis" || ext == "disbin";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: bas_replay <replay_file>\n";
    return EXIT_FAILURE;
  }

  const std::string replay_file = argv[1];

  std::vector<bas::DisPduBatch> batches;
  try {
    if (IsBinaryReplay(replay_file)) {
      bas::DisBinaryParser parser;
      batches = parser.ParseFile(replay_file);
    } else {
      bas::ScenarioReplayLoader loader;
      batches = loader.LoadBatches(replay_file);
    }
  } catch (const std::exception& e) {
    std::cerr << "Replay load failed: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  if (batches.empty()) {
    std::cerr << "Replay has no frames\n";
    return EXIT_FAILURE;
  }

  bas::ModelRuntime model_runtime;
  const bas::ModelBackend backend = ResolveBackend();
  const int timeout_ms = (backend == bas::ModelBackend::OpenAICompatible) ? 120000 : 250;
  model_runtime.Configure(
      {backend, "Qwen1.5-1.8B-Chat", 192, true, "http://127.0.0.1:8000/v1/chat/completions", "", timeout_ms});

  bas::AgentPipeline pipeline({3000, 5 * 60 * 1000}, bas::FireControlEngine{}, bas::ManeuverEngine{}, model_runtime);
  bas::DisAdapter adapter;
  bas::ReplayMetricsEvaluator metrics;

  std::vector<double> latencies_ms;
  std::size_t ticks = 0;
  std::size_t decisions = 0;
  std::size_t cache_hits = 0;

  for (const auto& batch : batches) {
    adapter.Ingest(batch);
    const auto snapshot = adapter.Poll();
    if (!snapshot.has_value()) {
      continue;
    }

    metrics.ObserveSnapshot(*snapshot);

    const auto t0 = std::chrono::steady_clock::now();
    const bas::DecisionPackage decision = pipeline.Tick(*snapshot, adapter.DrainEvents());
    const auto t1 = std::chrono::steady_clock::now();
    metrics.ObserveDecision(snapshot->timestamp_ms, decision);

    ++ticks;
    ++decisions;
    if (decision.from_cache) {
      ++cache_hits;
    }

    latencies_ms.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
  }

  if (decisions == 0 || latencies_ms.empty()) {
    std::cerr << "Replay produced no decisions\n";
    return EXIT_FAILURE;
  }

  std::sort(latencies_ms.begin(), latencies_ms.end());
  const double avg_ms = std::accumulate(latencies_ms.begin(), latencies_ms.end(), 0.0) /
                        static_cast<double>(latencies_ms.size());
  const std::size_t p95_index = static_cast<std::size_t>(latencies_ms.size() * 0.95);
  const double p95_ms = latencies_ms[std::min(p95_index, latencies_ms.size() - 1)];
  const bas::ReplayMetricsResult metric_result = metrics.Finalize();

  std::cout << "ReplayFile: " << replay_file << "\n";
  std::cout << "ModelBackend: "
            << (backend == bas::ModelBackend::OpenAICompatible ? "OpenAICompatible" : "Mock") << "\n";
  std::cout << "Frames: " << batches.size() << "\n";
  std::cout << "Ticks: " << ticks << "\n";
  std::cout << "Decisions: " << decisions << "\n";
  std::cout << "CacheHitRate: " << (100.0 * static_cast<double>(cache_hits) / static_cast<double>(decisions)) << "%\n";
  std::cout << "AvgLatencyMs: " << avg_ms << "\n";
  std::cout << "P95LatencyMs: " << p95_ms << "\n";
  std::cout << "InitialFriendlyCount: " << metric_result.initial_friendly_count << "\n";
  std::cout << "FinalFriendlyAlive: " << metric_result.final_friendly_alive << "\n";
  std::cout << "SurvivalRate: " << metric_result.survival_rate << "%\n";
  std::cout << "HostileLosses: " << metric_result.total_hostile_losses << "\n";
  std::cout << "HitContributionRate: " << metric_result.hit_contribution_rate << "%\n";
  for (const auto& [shooter, credit] : metric_result.shooter_kill_contribution) {
    std::cout << "ShooterKillCredit: " << shooter << "=" << credit << "\n";
  }

  return EXIT_SUCCESS;
}
