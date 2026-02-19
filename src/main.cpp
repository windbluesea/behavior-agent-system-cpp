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

const char* TacticNameToChinese(const std::string& tactic) {
  if (tactic == "focus_fire") {
    return "集火射击";
  }
  if (tactic == "stagger_fire") {
    return "梯次射击";
  }
  if (tactic == "single_shot") {
    return "单发射击";
  }
  return tactic.c_str();
}

const char* WeaponNameToChinese(const std::string& weapon) {
  if (weapon == "rifle") {
    return "步枪";
  }
  if (weapon == "tank_gun") {
    return "坦克炮";
  }
  if (weapon == "howitzer") {
    return "榴弹炮";
  }
  if (weapon == "sam") {
    return "防空导弹";
  }
  if (weapon == "generic") {
    return "通用武器";
  }
  return weapon.c_str();
}

const char* ActionNameToChinese(const std::string& action) {
  if (action == "emergency_evasion") {
    return "紧急规避";
  }
  if (action == "flank_reinforce") {
    return "侧翼增援";
  }
  if (action == "occupy_advantageous_terrain") {
    return "占据有利地形";
  }
  if (action == "advance_bound") {
    return "跃进前推";
  }
  return action.c_str();
}

void PrintDecision(const bas::DecisionPackage& pkg) {
  std::cout << "火力决策: " << pkg.fire.summary << "\n";
  std::cout << "机动决策: " << pkg.maneuver.summary << "\n";
  std::cout << "决策解释: " << pkg.explanation << "\n";
  std::cout << "是否命中缓存: " << (pkg.from_cache ? "是" : "否") << "\n";

  for (const auto& threat : pkg.fire.threats) {
    std::cout << "  威胁目标=" << threat.target_id << " 指数=" << threat.index << " 原因=" << threat.reason
              << "\n";
  }

  for (const auto& assignment : pkg.fire.assignments) {
    std::cout << "  火力单元=" << assignment.shooter_id << " 目标=" << assignment.target_id
              << " 武器=" << WeaponNameToChinese(assignment.weapon_name) << " 战术="
              << TacticNameToChinese(assignment.tactic)
              << " 延迟秒=" << assignment.scheduled_offset_s << "\n";
  }

  for (const auto& action : pkg.maneuver.actions) {
    std::cout << "  机动单元=" << action.unit_id << " 动作=" << ActionNameToChinese(action.action_name)
              << " 下一位置=(" << action.next_pose.x << "," << action.next_pose.y << ")\n";
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
    std::cerr << "未获取到态势快照\n";
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
  std::cout << "模型后端: " << (backend == bas::ModelBackend::OpenAICompatible ? "OpenAI兼容接口" : "模拟后端")
            << "\n";
  PrintDecision(first);

  const bas::DecisionPackage second = pipeline.Tick(*snapshot, {});
  std::cout << "--- 第二次决策循环 ---\n";
  PrintDecision(second);

  return 0;
}
