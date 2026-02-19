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
    std::cerr << "用法: bas_replay <回放文件路径>\n";
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
    std::cerr << "回放加载失败: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  if (batches.empty()) {
    std::cerr << "回放文件中没有有效帧\n";
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
    std::cerr << "回放未产生有效决策\n";
    return EXIT_FAILURE;
  }

  std::sort(latencies_ms.begin(), latencies_ms.end());
  const double avg_ms = std::accumulate(latencies_ms.begin(), latencies_ms.end(), 0.0) /
                        static_cast<double>(latencies_ms.size());
  const std::size_t p95_index = static_cast<std::size_t>(latencies_ms.size() * 0.95);
  const double p95_ms = latencies_ms[std::min(p95_index, latencies_ms.size() - 1)];
  const bas::ReplayMetricsResult metric_result = metrics.Finalize();

  std::cout << "回放文件: " << replay_file << "\n";
  std::cout << "模型后端: " << (backend == bas::ModelBackend::OpenAICompatible ? "OpenAI兼容接口" : "模拟后端")
            << "\n";
  std::cout << "帧数: " << batches.size() << "\n";
  std::cout << "决策循环次数: " << ticks << "\n";
  std::cout << "决策总数: " << decisions << "\n";
  std::cout << "缓存命中率: " << (100.0 * static_cast<double>(cache_hits) / static_cast<double>(decisions)) << "%\n";
  std::cout << "平均时延(毫秒): " << avg_ms << "\n";
  std::cout << "95分位时延(毫秒): " << p95_ms << "\n";
  std::cout << "初始我方兵力: " << metric_result.initial_friendly_count << "\n";
  std::cout << "最终存活我方兵力: " << metric_result.final_friendly_alive << "\n";
  std::cout << "生存率: " << metric_result.survival_rate << "%\n";
  std::cout << "敌方损失数: " << metric_result.total_hostile_losses << "\n";
  std::cout << "命中贡献率: " << metric_result.hit_contribution_rate << "%\n";
  for (const auto& [shooter, credit] : metric_result.shooter_kill_contribution) {
    std::cout << "射手毁伤贡献: " << shooter << "=" << credit << "\n";
  }

  return EXIT_SUCCESS;
}
