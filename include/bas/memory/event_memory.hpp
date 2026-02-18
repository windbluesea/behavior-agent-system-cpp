#pragma once

#include <deque>
#include <string>
#include <vector>

#include "bas/common/types.hpp"

namespace bas {

class EventMemory {
 public:
  explicit EventMemory(std::int64_t retention_ms = 10 * 60 * 1000);

  void AddEvent(const EventRecord& event);
  std::vector<EventRecord> QueryRecent(std::int64_t now_ms, std::int64_t window_ms) const;
  std::string BuildContext(std::int64_t now_ms, std::int64_t window_ms) const;

 private:
  void Trim(std::int64_t now_ms);

  std::deque<EventRecord> events_;
  std::int64_t retention_ms_;
};

}  // namespace bas
