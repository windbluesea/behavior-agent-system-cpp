#include "bas/cache/decision_cache.hpp"

namespace bas {

DecisionCache::DecisionCache(std::int64_t ttl_ms) : ttl_ms_(ttl_ms) {}

std::optional<DecisionPackage> DecisionCache::Get(const std::string& key, std::int64_t now_ms) const {
  const auto it = table_.find(key);
  if (it == table_.end()) {
    return std::nullopt;
  }
  if (now_ms - it->second.timestamp_ms > ttl_ms_) {
    return std::nullopt;
  }
  return it->second.value;
}

void DecisionCache::Put(const std::string& key, const DecisionPackage& value, std::int64_t now_ms) {
  table_[key] = Entry{now_ms, value};
}

void DecisionCache::Prune(std::int64_t now_ms) {
  for (auto it = table_.begin(); it != table_.end();) {
    if (now_ms - it->second.timestamp_ms > ttl_ms_) {
      it = table_.erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace bas
