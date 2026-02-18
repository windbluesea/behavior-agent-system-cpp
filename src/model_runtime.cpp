#include "bas/inference/model_runtime.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <unistd.h>

namespace bas {

namespace {

std::string ReadEnvOrDefault(const char* key, const std::string& fallback) {
  const char* value = std::getenv(key);
  if (value == nullptr || *value == '\0') {
    return fallback;
  }
  return value;
}

std::string UnescapeJsonString(const std::string& input) {
  std::string out;
  out.reserve(input.size());
  bool escaping = false;
  for (char c : input) {
    if (escaping) {
      switch (c) {
        case 'n':
          out.push_back('\n');
          break;
        case 't':
          out.push_back('\t');
          break;
        case 'r':
          out.push_back('\r');
          break;
        case '\\':
          out.push_back('\\');
          break;
        case '"':
          out.push_back('"');
          break;
        default:
          out.push_back(c);
          break;
      }
      escaping = false;
      continue;
    }

    if (c == '\\') {
      escaping = true;
      continue;
    }
    out.push_back(c);
  }
  return out;
}

}  // namespace

void ModelRuntime::Configure(const ModelConfig& config) {
  config_ = config;
}

ModelResponse ModelRuntime::RankAndExplain(const ModelRequest& request) const {
  ModelResponse response;
  if (request.candidate_summaries.empty()) {
    response.selected_index = 0;
    response.explanation = "no candidate decisions available";
    return response;
  }

  if (config_.backend == ModelBackend::Mock) {
    response.selected_index = 0;
    response.explanation = "mock-ranker selected candidate 0; backend=mock,int8=" +
                           std::string(config_.use_int8 ? "true" : "false");
    return response;
  }

  const std::string endpoint = ReadEnvOrDefault("BAS_QWEN_ENDPOINT", config_.endpoint);
  const std::string model_name = ReadEnvOrDefault("BAS_QWEN_MODEL", config_.model_name);
  const std::string api_key = config_.api_key.empty() ? ReadEnvOrDefault("BAS_QWEN_API_KEY", "") : config_.api_key;

  std::ostringstream candidate_lines;
  for (std::size_t i = 0; i < request.candidate_summaries.size(); ++i) {
    candidate_lines << i << ": " << request.candidate_summaries[i] << "\n";
  }

  const std::string prompt =
      "Task: rank tactical decision candidates for battlefield simulation."
      " Return strict JSON with keys selected_index and explanation."
      " Keep explanation under 60 words.\n"
      "Context:\n" +
      request.context + "\nCandidates:\n" + candidate_lines.str();

  const std::string payload =
      "{\"model\":\"" + EscapeJson(model_name) +
      "\",\"messages\":[{\"role\":\"system\",\"content\":\"You are a tactical decision ranker.\"},"
      "{\"role\":\"user\",\"content\":\"" +
      EscapeJson(prompt) + "\"}],\"temperature\":0.1,\"max_tokens\":" + std::to_string(config_.max_tokens) + "}";

  const std::string temp_path = "/tmp/bas_model_request_" + std::to_string(::getpid()) + "_" +
                                std::to_string(static_cast<long long>(std::time(nullptr))) + ".json";

  {
    std::ofstream ofs(temp_path, std::ios::binary);
    if (!ofs) {
      response.selected_index = 0;
      response.explanation = "failed to create temp request file; fallback candidate 0";
      return response;
    }
    ofs << payload;
  }

  std::ostringstream command;
  command << "curl -sS --max-time " << (config_.timeout_ms / 1000.0);
  command << " -H \"Content-Type: application/json\"";
  if (!api_key.empty()) {
    command << " -H \"Authorization: Bearer " << api_key << "\"";
  }
  command << " --data @" << temp_path << " \"" << endpoint << "\" 2>/dev/null";

  const std::string raw = RunCommand(command.str());
  std::remove(temp_path.c_str());

  if (raw.empty()) {
    response.selected_index = 0;
    response.explanation = "model call returned empty response; fallback candidate 0";
    return response;
  }

  const std::string content = ExtractAssistantContent(raw);
  if (content.empty()) {
    response.selected_index = 0;
    response.explanation = "model response parse failed; fallback candidate 0";
    return response;
  }

  response.selected_index = ParseSelectedIndex(content, request.candidate_summaries.size());
  response.explanation = content;
  return response;
}

std::string ModelRuntime::EscapeJson(const std::string& text) {
  std::ostringstream oss;
  for (char c : text) {
    switch (c) {
      case '"':
        oss << "\\\"";
        break;
      case '\\':
        oss << "\\\\";
        break;
      case '\n':
        oss << "\\n";
        break;
      case '\r':
        oss << "\\r";
        break;
      case '\t':
        oss << "\\t";
        break;
      default:
        oss << c;
        break;
    }
  }
  return oss.str();
}

std::string ModelRuntime::RunCommand(const std::string& command) {
  std::array<char, 512> buffer{};
  std::string output;
  FILE* pipe = popen(command.c_str(), "r");
  if (pipe == nullptr) {
    return output;
  }
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
    output += buffer.data();
  }
  pclose(pipe);
  return output;
}

std::string ModelRuntime::ExtractAssistantContent(const std::string& json_text) {
  const std::string key = "\"content\":\"";
  const std::size_t start = json_text.find(key);
  if (start == std::string::npos) {
    return {};
  }

  std::string escaped;
  escaped.reserve(json_text.size() - start);
  bool escaping = false;
  for (std::size_t i = start + key.size(); i < json_text.size(); ++i) {
    const char c = json_text[i];
    if (escaping) {
      escaped.push_back(c);
      escaping = false;
      continue;
    }
    if (c == '\\') {
      escaped.push_back(c);
      escaping = true;
      continue;
    }
    if (c == '"') {
      break;
    }
    escaped.push_back(c);
  }

  return UnescapeJsonString(escaped);
}

std::size_t ModelRuntime::ParseSelectedIndex(const std::string& text, std::size_t max_index) {
  if (max_index == 0) {
    return 0;
  }

  const std::regex re("selected_index\\s*[:=]\\s*([0-9]+)");
  std::smatch match;
  if (std::regex_search(text, match, re)) {
    try {
      const std::size_t value = static_cast<std::size_t>(std::stoul(match[1].str()));
      return std::min(value, max_index - 1);
    } catch (const std::exception&) {
      return 0;
    }
  }

  const std::regex fallback_re("\\b([0-9]+)\\b");
  if (std::regex_search(text, match, fallback_re)) {
    try {
      const std::size_t value = static_cast<std::size_t>(std::stoul(match[1].str()));
      return std::min(value, max_index - 1);
    } catch (const std::exception&) {
      return 0;
    }
  }
  return 0;
}

}  // namespace bas
