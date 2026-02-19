#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "bas/inference/model_runtime.hpp"
#include "bas/system/agent_pipeline.hpp"

namespace {

bas::BattlefieldSnapshot BuildSnapshot(std::int64_t t, int offset) {
  bas::BattlefieldSnapshot snap;
  snap.timestamp_ms = t;
  snap.env = {1000.0, 0.1, 0.2};

  bas::EntityState f;
  f.id = "F-1";
  f.side = bas::Side::Friendly;
  f.type = bas::UnitType::Armor;
  f.pose = {0.0 + offset, 0.0, 0.0};
  f.weapons.push_back({"tank_gun", 2500.0, 0.65, 10, 0.0, {bas::UnitType::Armor}});
  snap.friendly_units.push_back(f);

  bas::EntityState h;
  h.id = "H-1";
  h.side = bas::Side::Hostile;
  h.type = bas::UnitType::Armor;
  h.pose = {420.0 + offset, 160.0, 0.0};
  h.speed_mps = 8.0;
  h.threat_level = 0.9;
  snap.hostile_units.push_back(h);

  return snap;
}

}  // namespace

int main() {
  bas::ModelRuntime model;
  model.Configure({bas::ModelBackend::Mock, "Qwen1.5-1.8B-Chat", 128, true,
                   "http://127.0.0.1:8000/v1/chat/completions", "", 250});

  bas::AgentPipeline pipeline({100, 5 * 60 * 1000}, bas::FireControlEngine{}, bas::ManeuverEngine{}, model);

  std::vector<double> latencies_ms;
  latencies_ms.reserve(300);

  for (int i = 0; i < 300; ++i) {
    const auto t0 = std::chrono::steady_clock::now();
    const bas::BattlefieldSnapshot snap = BuildSnapshot(1000000 + i * 10, i % 7);
    const bas::DecisionPackage result = pipeline.Tick(snap, {});
    const auto t1 = std::chrono::steady_clock::now();

    if (result.fire.assignments.empty() || result.maneuver.actions.empty()) {
      std::cerr << "时延测试期间出现无效决策\n";
      return EXIT_FAILURE;
    }

    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    latencies_ms.push_back(ms);
  }

  std::sort(latencies_ms.begin(), latencies_ms.end());
  const std::size_t p95_index = static_cast<std::size_t>(latencies_ms.size() * 0.95);
  const double p95_ms = latencies_ms[std::min(p95_index, latencies_ms.size() - 1)];

  if (p95_ms > 100.0) {
    std::cerr << "时延目标未达成，P95=" << p95_ms << "毫秒\n";
    return EXIT_FAILURE;
  }

  std::cout << "95分位时延(毫秒)=" << p95_ms << "\n";
  return EXIT_SUCCESS;
}
