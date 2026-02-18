#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace bas {

struct ModelConfig {
  std::string model_name = "local-1.5b";
  std::size_t max_tokens = 128;
  bool use_int8 = true;
};

struct ModelRequest {
  std::string context;
  std::vector<std::string> candidate_summaries;
};

struct ModelResponse {
  std::size_t selected_index = 0;
  std::string explanation;
};

class ModelRuntime {
 public:
  void Configure(const ModelConfig& config);
  ModelResponse RankAndExplain(const ModelRequest& request) const;

 private:
  ModelConfig config_;
};

}  // namespace bas
