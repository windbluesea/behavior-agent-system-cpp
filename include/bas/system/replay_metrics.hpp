#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "bas/common/types.hpp"

namespace bas {

struct ReplayMetricsResult {
  std::size_t initial_friendly_count = 0;
  std::size_t final_friendly_alive = 0;
  std::size_t total_hostile_losses = 0;
  double survival_rate = 0.0;
  double hit_contribution_rate = 0.0;
  std::unordered_map<std::string, double> shooter_kill_contribution;
};

class ReplayMetricsEvaluator {
 public:
  explicit ReplayMetricsEvaluator(std::int64_t kill_credit_window_ms = 120000);

  void ObserveSnapshot(const BattlefieldSnapshot& snapshot);
  void ObserveDecision(std::int64_t timestamp_ms, const DecisionPackage& decision);
  ReplayMetricsResult Finalize() const;

 private:
  struct ShotRecord {
    std::int64_t timestamp_ms = 0;
    std::string shooter_id;
  };

  void PruneShotHistory(std::int64_t now_ms);

  std::int64_t kill_credit_window_ms_;
  bool initialized_ = false;

  std::unordered_map<std::string, bool> friendly_alive_state_;
  std::unordered_map<std::string, bool> hostile_alive_state_;
  std::unordered_map<std::string, std::vector<ShotRecord>> shots_by_target_;
  std::unordered_map<std::string, double> shooter_kill_credit_;

  std::size_t initial_friendly_count_ = 0;
  std::size_t final_friendly_alive_ = 0;
  std::size_t total_hostile_losses_ = 0;
  double credited_losses_ = 0.0;
};

}  // namespace bas
