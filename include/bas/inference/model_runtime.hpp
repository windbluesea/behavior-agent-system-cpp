#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace bas {

enum class ModelBackend { Mock, OpenAICompatible };

struct ModelConfig {
  ModelBackend backend = ModelBackend::Mock;
  std::string model_name = "Qwen1.5-1.8B-Chat";
  std::size_t max_tokens = 192;
  bool use_int8 = true;
  std::string endpoint = "http://127.0.0.1:8000/v1/chat/completions";
  std::string api_key;
  int timeout_ms = 250;
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
  static std::string EscapeJson(const std::string& text);
  static std::string RunCommand(const std::string& command);
  static std::string ExtractAssistantContent(const std::string& json_text);
  static std::string ExtractExplanation(const std::string& text);
  static std::size_t ParseSelectedIndex(const std::string& text, std::size_t max_index);

  ModelConfig config_;
};

}  // namespace bas
