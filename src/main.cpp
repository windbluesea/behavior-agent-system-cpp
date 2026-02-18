#include <chrono>
#include <iostream>
#include <string>

#include "bas/cache/decision_cache.hpp"
#include "bas/decision/fire_control_engine.hpp"
#include "bas/decision/maneuver_engine.hpp"
#include "bas/dis/dis_adapter.hpp"
#include "bas/inference/model_runtime.hpp"
#include "bas/memory/event_memory.hpp"
#include "bas/situation/situation_fusion.hpp"

namespace {

bas::BattlefieldSnapshot BuildMockSnapshot(std::int64_t now_ms) {
  bas::BattlefieldSnapshot snap;
  snap.timestamp_ms = now_ms;

  snap.friendly_units.push_back({"F-1", bas::Side::Friendly, bas::UnitType::Armor, {0.0, 0.0, 0.0}, 6.0, 0.4, true});
  snap.friendly_units.push_back({"F-2", bas::Side::Friendly, bas::UnitType::Infantry, {-20.0, -15.0, 0.0}, 2.0, 0.3,
                                 true});

  snap.hostile_units.push_back({"H-1", bas::Side::Hostile, bas::UnitType::Armor, {450.0, 200.0, 0.0}, 8.5, 0.9, true});
  snap.hostile_units.push_back(
      {"H-2", bas::Side::Hostile, bas::UnitType::Artillery, {-180.0, 130.0, 0.0}, 3.0, 0.8, true});

  snap.env.visibility_m = 900.0;
  snap.env.weather_risk = 0.2;
  return snap;
}

std::string CacheKey(const bas::BattlefieldSnapshot& snapshot) {
  return "f=" + std::to_string(snapshot.friendly_units.size()) +
         "|h=" + std::to_string(snapshot.hostile_units.size()) +
         "|v=" + std::to_string(static_cast<int>(snapshot.env.visibility_m / 100));
}

}  // namespace

int main() {
  using Clock = std::chrono::steady_clock;
  const auto now = Clock::now().time_since_epoch();
  const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

  bas::DisAdapter dis;
  bas::SituationFusion fusion;
  bas::EventMemory memory;
  bas::FireControlEngine fire_engine;
  bas::ManeuverEngine maneuver_engine;
  bas::DecisionCache cache(5000);

  bas::ModelRuntime model;
  model.Configure({"local-1.5b", 128, true});

  dis.FeedMockFrame(BuildMockSnapshot(now_ms));
  const auto snapshot = dis.Poll();
  if (!snapshot.has_value()) {
    std::cerr << "No snapshot available\n";
    return 1;
  }

  const auto key = CacheKey(*snapshot);
  if (const auto cached = cache.Get(key, snapshot->timestamp_ms); cached.has_value()) {
    std::cout << "[cache-hit] " << cached->explanation << "\n";
    return 0;
  }

  const auto semantics = fusion.Infer(*snapshot);
  for (const auto& tag : semantics.tags) {
    memory.AddEvent({snapshot->timestamp_ms, "semantic_tag=" + tag.name});
  }

  bas::DecisionPackage pkg;
  pkg.fire = fire_engine.Decide(*snapshot, semantics, memory);
  pkg.maneuver = maneuver_engine.Decide(*snapshot, semantics);

  const bas::ModelRequest req{memory.BuildContext(snapshot->timestamp_ms, 5 * 60 * 1000),
                              {pkg.fire.summary + ";" + pkg.maneuver.summary}};
  const auto model_out = model.RankAndExplain(req);
  pkg.explanation = model_out.explanation;

  cache.Put(key, pkg, snapshot->timestamp_ms);

  std::cout << "Fire: " << pkg.fire.summary << "\n";
  std::cout << "Maneuver: " << pkg.maneuver.summary << "\n";
  std::cout << "Explain: " << pkg.explanation << "\n";

  for (const auto& a : pkg.fire.assignments) {
    std::cout << "  shooter=" << a.shooter_id << " target=" << a.target_id << " weapon=" << a.weapon_name
              << " score=" << a.score << "\n";
  }

  for (const auto& m : pkg.maneuver.actions) {
    std::cout << "  unit=" << m.unit_id << " action=" << m.action_name << " next=(" << m.next_pose.x << ","
              << m.next_pose.y << ")\n";
  }

  return 0;
}
