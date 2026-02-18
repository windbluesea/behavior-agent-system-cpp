#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

#include "bas/common/types.hpp"

namespace bas {

class DecisionCache {
 public:
  explicit DecisionCache(std::int64_t ttl_ms = 3000);

  std::optional<DecisionPackage> Get(const std::string& key, std::int64_t now_ms) const;
  void Put(const std::string& key, const DecisionPackage& value, std::int64_t now_ms);
  void Prune(std::int64_t now_ms);

 private:
  struct Entry {
    std::int64_t timestamp_ms = 0;
    DecisionPackage value;
  };

  std::unordered_map<std::string, Entry> table_;
  std::int64_t ttl_ms_;
};

}  // namespace bas
