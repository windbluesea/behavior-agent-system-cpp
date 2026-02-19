#include "bas/system/replay_metrics.hpp"

#include <algorithm>
#include <unordered_set>

namespace bas {

ReplayMetricsEvaluator::ReplayMetricsEvaluator(std::int64_t kill_credit_window_ms)
    : kill_credit_window_ms_(kill_credit_window_ms) {}

void ReplayMetricsEvaluator::ObserveSnapshot(const BattlefieldSnapshot& snapshot) {
  if (!initialized_ && !snapshot.friendly_units.empty()) {
    initialized_ = true;
    initial_friendly_count_ = snapshot.friendly_units.size();
  }

  final_friendly_alive_ = 0;
  for (const auto& unit : snapshot.friendly_units) {
    friendly_alive_state_[unit.id] = unit.alive;
    if (unit.alive) {
      ++final_friendly_alive_;
    }
  }

  for (const auto& unit : snapshot.hostile_units) {
    const auto it = hostile_alive_state_.find(unit.id);
    const bool had_prev = (it != hostile_alive_state_.end());
    const bool was_alive = had_prev ? it->second : unit.alive;

    if (had_prev && was_alive && !unit.alive) {
      ++total_hostile_losses_;

      auto shots_it = shots_by_target_.find(unit.id);
      if (shots_it != shots_by_target_.end()) {
        std::unordered_set<std::string> unique_shooters;
        for (const auto& shot : shots_it->second) {
          if (snapshot.timestamp_ms - shot.timestamp_ms <= kill_credit_window_ms_) {
            unique_shooters.insert(shot.shooter_id);
          }
        }

        if (!unique_shooters.empty()) {
          const double credit = 1.0 / static_cast<double>(unique_shooters.size());
          for (const auto& shooter : unique_shooters) {
            shooter_kill_credit_[shooter] += credit;
          }
          credited_losses_ += 1.0;
        }
      }
    }

    hostile_alive_state_[unit.id] = unit.alive;
  }

  PruneShotHistory(snapshot.timestamp_ms);
}

void ReplayMetricsEvaluator::ObserveDecision(std::int64_t timestamp_ms, const DecisionPackage& decision) {
  for (const auto& assignment : decision.fire.assignments) {
    shots_by_target_[assignment.target_id].push_back({timestamp_ms, assignment.shooter_id});
  }
  PruneShotHistory(timestamp_ms);
}

ReplayMetricsResult ReplayMetricsEvaluator::Finalize() const {
  ReplayMetricsResult out;
  out.initial_friendly_count = initial_friendly_count_;
  out.final_friendly_alive = final_friendly_alive_;
  out.total_hostile_losses = total_hostile_losses_;
  out.survival_rate = (initial_friendly_count_ == 0)
                          ? 0.0
                          : (100.0 * static_cast<double>(final_friendly_alive_) / static_cast<double>(initial_friendly_count_));
  out.hit_contribution_rate =
      (total_hostile_losses_ == 0) ? 0.0 : (100.0 * credited_losses_ / static_cast<double>(total_hostile_losses_));
  out.shooter_kill_contribution = shooter_kill_credit_;
  return out;
}

void ReplayMetricsEvaluator::PruneShotHistory(std::int64_t now_ms) {
  for (auto it = shots_by_target_.begin(); it != shots_by_target_.end();) {
    auto& records = it->second;
    records.erase(std::remove_if(records.begin(), records.end(), [&](const ShotRecord& shot) {
                    return now_ms - shot.timestamp_ms > kill_credit_window_ms_;
                  }),
                  records.end());
    if (records.empty()) {
      it = shots_by_target_.erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace bas
