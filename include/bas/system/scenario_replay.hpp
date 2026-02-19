#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "bas/dis/dis_adapter.hpp"

namespace bas {

struct ReplayStats {
  std::size_t frames = 0;
  std::size_t ticks = 0;
  std::size_t decisions = 0;
  std::size_t cache_hits = 0;
  double avg_latency_ms = 0.0;
  double p95_latency_ms = 0.0;
};

class ScenarioReplayLoader {
 public:
  std::vector<DisPduBatch> LoadBatches(const std::string& path) const;

 private:
  static std::vector<std::string> SplitCsv(const std::string& line);
  static std::string Trim(const std::string& value);
};

}  // namespace bas
