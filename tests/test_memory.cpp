#include <cstdlib>
#include <iostream>

#include "bas/memory/event_memory.hpp"

int main() {
  bas::EventMemory memory(5 * 60 * 1000);
  const std::int64_t now_ms = 1000000;

  memory.AddEvent({now_ms - 6 * 60 * 1000, bas::EventType::SensorContact, "U-1", {}, "expired"});
  memory.AddEvent({now_ms - 30 * 1000, bas::EventType::WeaponFire, "U-2", {}, "fresh"});

  const auto recent = memory.QueryRecent(now_ms, 60 * 1000);
  if (recent.size() != 1 || recent.front().message != "fresh") {
    std::cerr << "近期事件查询失败\n";
    return EXIT_FAILURE;
  }

  const auto last_fire = memory.LastEventByType(bas::EventType::WeaponFire, now_ms, 60 * 1000);
  if (!last_fire.has_value() || last_fire->actor_id != "U-2") {
    std::cerr << "最近开火事件检索失败\n";
    return EXIT_FAILURE;
  }

  const std::string context = memory.BuildContext(now_ms, 60 * 1000);
  if (context.find("武器开火") == std::string::npos) {
    std::cerr << "上下文构建结果缺少事件类型\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
