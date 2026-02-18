#include "bas/memory/event_memory.hpp"

#include <sstream>

namespace bas {

EventMemory::EventMemory(std::int64_t retention_ms) : retention_ms_(retention_ms) {}

void EventMemory::AddEvent(const EventRecord& event) {
  events_.push_back(event);
  Trim(event.timestamp_ms);
}

std::vector<EventRecord> EventMemory::QueryRecent(std::int64_t now_ms, std::int64_t window_ms) const {
  std::vector<EventRecord> out;
  for (auto it = events_.rbegin(); it != events_.rend(); ++it) {
    if (now_ms - it->timestamp_ms > window_ms) {
      break;
    }
    out.push_back(*it);
  }
  return out;
}

std::string EventMemory::BuildContext(std::int64_t now_ms, std::int64_t window_ms) const {
  std::ostringstream oss;
  const auto recent = QueryRecent(now_ms, window_ms);
  for (const auto& ev : recent) {
    oss << "[t=" << ev.timestamp_ms << "] " << ev.message << "\n";
  }
  return oss.str();
}

void EventMemory::Trim(std::int64_t now_ms) {
  while (!events_.empty() && now_ms - events_.front().timestamp_ms > retention_ms_) {
    events_.pop_front();
  }
}

}  // namespace bas
