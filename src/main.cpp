#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

#include "bas/dis/dis_adapter.hpp"
#include "bas/inference/model_runtime.hpp"
#include "bas/system/agent_pipeline.hpp"

namespace {

bas::DisPduBatch BuildDemoPdus(std::int64_t now_ms) {
  bas::DisPduBatch batch;
  batch.env = bas::EnvironmentState{900.0, 0.2, 0.3};

  batch.entity_updates.push_back({now_ms, "F-1", bas::Side::Friendly, bas::UnitType::Armor, {0.0, 0.0, 0.0}, 6.0,
                                  15.0, true, 0.35});
  batch.entity_updates.push_back({now_ms, "F-2", bas::Side::Friendly, bas::UnitType::Infantry, {-25.0, -10.0, 0.0},
                                  2.0, 20.0, true, 0.20});

  batch.entity_updates.push_back({now_ms, "H-1", bas::Side::Hostile, bas::UnitType::Armor, {380.0, 180.0, 0.0}, 9.0,
                                  210.0, true, 0.95});
  batch.entity_updates.push_back({now_ms, "H-2", bas::Side::Hostile, bas::UnitType::Artillery, {-160.0, 140.0, 0.0},
                                  3.5, 195.0, true, 0.82});

  batch.fire_events.push_back({now_ms - 45 * 1000, "H-2", "F-1", "howitzer", {-160.0, 140.0, 0.0}});
  return batch;
}

void PrintDecision(const bas::DecisionPackage& pkg) {
  std::cout << "Fire: " << pkg.fire.summary << "\n";
  std::cout << "Maneuver: " << pkg.maneuver.summary << "\n";
  std::cout << "Explain: " << pkg.explanation << "\n";
  std::cout << "FromCache: " << (pkg.from_cache ? "true" : "false") << "\n";

  for (const auto& threat : pkg.fire.threats) {
    std::cout << "  threat target=" << threat.target_id << " index=" << threat.index << " reason=" << threat.reason
              << "\n";
  }

  for (const auto& assignment : pkg.fire.assignments) {
    std::cout << "  fire shooter=" << assignment.shooter_id << " target=" << assignment.target_id
              << " weapon=" << assignment.weapon_name << " tactic=" << assignment.tactic
              << " offset_s=" << assignment.scheduled_offset_s << "\n";
  }

  for (const auto& action : pkg.maneuver.actions) {
    std::cout << "  move unit=" << action.unit_id << " action=" << action.action_name
              << " next=(" << action.next_pose.x << "," << action.next_pose.y << ")\n";
  }
}

}  // namespace

int main() {
  using Clock = std::chrono::steady_clock;
  const auto now = Clock::now().time_since_epoch();
  const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

  bas::DisAdapter adapter;
  adapter.Ingest(BuildDemoPdus(now_ms));

  const auto snapshot = adapter.Poll();
  if (!snapshot.has_value()) {
    std::cerr << "No snapshot available\n";
    return 1;
  }

  bas::ModelRuntime model_runtime;
  const char* backend_env = std::getenv("BAS_MODEL_BACKEND");
  const bas::ModelBackend backend =
      (backend_env != nullptr && std::string(backend_env) == "openai") ? bas::ModelBackend::OpenAICompatible
                                                                        : bas::ModelBackend::Mock;
  const int timeout_ms = (backend == bas::ModelBackend::OpenAICompatible) ? 60000 : 250;

  model_runtime.Configure(
      {backend, "Qwen1.5-1.8B-Chat", 192, true, "http://127.0.0.1:8000/v1/chat/completions", "", timeout_ms});

  bas::AgentPipeline pipeline({3000, 5 * 60 * 1000}, bas::FireControlEngine{}, bas::ManeuverEngine{}, model_runtime);

  const bas::DecisionPackage first = pipeline.Tick(*snapshot, adapter.DrainEvents());
  std::cout << "ModelBackend: "
            << (backend == bas::ModelBackend::OpenAICompatible ? "OpenAICompatible" : "Mock") << "\n";
  PrintDecision(first);

  const bas::DecisionPackage second = pipeline.Tick(*snapshot, {});
  std::cout << "--- second tick ---\n";
  PrintDecision(second);

  return 0;
}
