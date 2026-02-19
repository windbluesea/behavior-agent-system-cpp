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

int ReadIntEnvOrDefault(const char* key, int fallback) {
  const char* value = std::getenv(key);
  if (value == nullptr || *value == '\0') {
    return fallback;
  }
  try {
    return std::stoi(value);
  } catch (const std::exception&) {
    return fallback;
  }
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
    response.explanation = "没有可用候选决策";
    return response;
  }

  if (config_.backend == ModelBackend::Mock) {
    response.selected_index = 0;
    response.explanation = "模拟排序器选择候选0；后端=模拟，int8=" +
                           std::string(config_.use_int8 ? "开启" : "关闭");
    return response;
  }

  const std::string endpoint = ReadEnvOrDefault("BAS_QWEN_ENDPOINT", config_.endpoint);
  const std::string model_name = ReadEnvOrDefault("BAS_QWEN_MODEL", config_.model_name);
  const std::string api_key = config_.api_key.empty() ? ReadEnvOrDefault("BAS_QWEN_API_KEY", "") : config_.api_key;
  const int timeout_ms = std::max(500, ReadIntEnvOrDefault("BAS_QWEN_TIMEOUT_MS", config_.timeout_ms));

  std::ostringstream candidate_lines;
  for (std::size_t i = 0; i < request.candidate_summaries.size(); ++i) {
    candidate_lines << i << ": " << request.candidate_summaries[i] << "\n";
  }

  const std::string prompt =
      "任务：对战场仿真候选决策进行排序。"
      " 请严格返回JSON，包含selected_index与explanation两个字段。"
      " explanation请控制在60字以内。\n"
      "上下文：\n" +
      request.context + "\n候选方案：\n" + candidate_lines.str();

  const std::string payload =
      "{\"model\":\"" + EscapeJson(model_name) +
      "\",\"messages\":[{\"role\":\"system\",\"content\":\"你是战术决策排序助手。\"},"
      "{\"role\":\"user\",\"content\":\"" +
      EscapeJson(prompt) + "\"}],\"temperature\":0.1,\"max_tokens\":" + std::to_string(config_.max_tokens) + "}";

  const std::string temp_path = "/tmp/bas_model_request_" + std::to_string(::getpid()) + "_" +
                                std::to_string(static_cast<long long>(std::time(nullptr))) + ".json";

  {
    std::ofstream ofs(temp_path, std::ios::binary);
    if (!ofs) {
      response.selected_index = 0;
      response.explanation = "创建临时请求文件失败，回退到候选0";
      return response;
    }
    ofs << payload;
  }

  std::ostringstream command;
  command << "curl -sS --max-time " << (timeout_ms / 1000.0);
  command << " -H \"Content-Type: application/json\"";
  if (!api_key.empty()) {
    command << " -H \"Authorization: Bearer " << api_key << "\"";
  }
  command << " --data @" << temp_path << " \"" << endpoint << "\" 2>/dev/null";

  const std::string raw = RunCommand(command.str());
  std::remove(temp_path.c_str());

  if (raw.empty()) {
    response.selected_index = 0;
    response.explanation = "模型调用返回空响应，回退到候选0";
    return response;
  }

  const std::string content = ExtractAssistantContent(raw);
  if (content.empty()) {
    response.selected_index = 0;
    response.explanation = "模型响应解析失败，回退到候选0";
    return response;
  }

  response.selected_index = ParseSelectedIndex(content, request.candidate_summaries.size());
  response.explanation = ExtractExplanation(content);
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
  const std::regex content_re("\"content\"\\s*:\\s*\"((?:\\\\.|[^\"\\\\])*)\"");
  std::smatch match;
  if (!std::regex_search(json_text, match, content_re) || match.size() < 2) {
    return {};
  }
  return UnescapeJsonString(match[1].str());
}

std::string ModelRuntime::ExtractExplanation(const std::string& text) {
  const std::regex explanation_json_re("\"explanation\"\\s*:\\s*\"((?:\\\\.|[^\"\\\\])*)\"");
  std::smatch match;
  if (std::regex_search(text, match, explanation_json_re) && match.size() >= 2) {
    return UnescapeJsonString(match[1].str());
  }

  const std::regex explanation_plain_re("explanation\\s*[:=]\\s*(.+)");
  if (std::regex_search(text, match, explanation_plain_re) && match.size() >= 2) {
    return match[1].str();
  }

  return text;
}

std::size_t ModelRuntime::ParseSelectedIndex(const std::string& text, std::size_t max_index) {
  if (max_index == 0) {
    return 0;
  }

  const std::regex re("\"?selected_index\"?\\s*[:=]\\s*([0-9]+)");
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
