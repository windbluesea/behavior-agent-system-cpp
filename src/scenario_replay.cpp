#include "bas/system/scenario_replay.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace bas {

namespace {

std::int64_t ParseInt64(const std::string& text, const std::string& field_name, int line_no) {
  try {
    return std::stoll(text);
  } catch (const std::exception&) {
    throw std::runtime_error("Invalid int64 for " + field_name + " at line " + std::to_string(line_no));
  }
}

double ParseDouble(const std::string& text, const std::string& field_name, int line_no) {
  try {
    return std::stod(text);
  } catch (const std::exception&) {
    throw std::runtime_error("Invalid double for " + field_name + " at line " + std::to_string(line_no));
  }
}

bool ParseBool(const std::string& text, const std::string& field_name, int line_no) {
  if (text == "1" || text == "true" || text == "TRUE") {
    return true;
  }
  if (text == "0" || text == "false" || text == "FALSE") {
    return false;
  }
  throw std::runtime_error("Invalid bool for " + field_name + " at line " + std::to_string(line_no));
}

}  // namespace

std::vector<DisPduBatch> ScenarioReplayLoader::LoadBatches(const std::string& path) const {
  std::ifstream ifs(path);
  if (!ifs) {
    throw std::runtime_error("Cannot open replay file: " + path);
  }

  std::unordered_map<std::int64_t, DisPduBatch> batches_by_ts;
  std::vector<std::int64_t> timestamps;

  std::string line;
  int line_no = 0;
  while (std::getline(ifs, line)) {
    ++line_no;
    line = Trim(line);
    if (line.empty() || line[0] == '#') {
      continue;
    }

    const auto fields = SplitCsv(line);
    if (fields.empty()) {
      continue;
    }

    const std::string rec_type = Trim(fields[0]);

    if (rec_type == "ENV") {
      if (fields.size() != 5) {
        throw std::runtime_error("ENV format error at line " + std::to_string(line_no));
      }
      const std::int64_t ts = ParseInt64(Trim(fields[1]), "timestamp", line_no);
      if (batches_by_ts.find(ts) == batches_by_ts.end()) {
        timestamps.push_back(ts);
      }
      EnvironmentState env;
      env.visibility_m = ParseDouble(Trim(fields[2]), "visibility_m", line_no);
      env.weather_risk = ParseDouble(Trim(fields[3]), "weather_risk", line_no);
      env.terrain_risk = ParseDouble(Trim(fields[4]), "terrain_risk", line_no);
      batches_by_ts[ts].env = env;
      continue;
    }

    if (rec_type == "ENTITY") {
      if (fields.size() != 12) {
        throw std::runtime_error("ENTITY format error at line " + std::to_string(line_no));
      }
      const std::int64_t ts = ParseInt64(Trim(fields[1]), "timestamp", line_no);
      if (batches_by_ts.find(ts) == batches_by_ts.end()) {
        timestamps.push_back(ts);
      }

      DisEntityPdu pdu;
      pdu.timestamp_ms = ts;
      pdu.entity_id = Trim(fields[2]);
      pdu.side = SideFromString(Trim(fields[3]));
      pdu.type = UnitTypeFromString(Trim(fields[4]));
      pdu.pose.x = ParseDouble(Trim(fields[5]), "x", line_no);
      pdu.pose.y = ParseDouble(Trim(fields[6]), "y", line_no);
      pdu.pose.z = ParseDouble(Trim(fields[7]), "z", line_no);
      pdu.speed_mps = ParseDouble(Trim(fields[8]), "speed_mps", line_no);
      pdu.heading_deg = ParseDouble(Trim(fields[9]), "heading_deg", line_no);
      pdu.alive = ParseBool(Trim(fields[10]), "alive", line_no);
      pdu.threat_level = ParseDouble(Trim(fields[11]), "threat_level", line_no);

      batches_by_ts[ts].entity_updates.push_back(pdu);
      continue;
    }

    if (rec_type == "FIRE") {
      if (fields.size() != 8) {
        throw std::runtime_error("FIRE format error at line " + std::to_string(line_no));
      }
      const std::int64_t ts = ParseInt64(Trim(fields[1]), "timestamp", line_no);
      if (batches_by_ts.find(ts) == batches_by_ts.end()) {
        timestamps.push_back(ts);
      }

      DisFirePdu pdu;
      pdu.timestamp_ms = ts;
      pdu.shooter_id = Trim(fields[2]);
      pdu.target_id = Trim(fields[3]);
      pdu.weapon_name = Trim(fields[4]);
      pdu.origin.x = ParseDouble(Trim(fields[5]), "x", line_no);
      pdu.origin.y = ParseDouble(Trim(fields[6]), "y", line_no);
      pdu.origin.z = ParseDouble(Trim(fields[7]), "z", line_no);

      batches_by_ts[ts].fire_events.push_back(pdu);
      continue;
    }

    throw std::runtime_error("Unknown record type at line " + std::to_string(line_no) + ": " + rec_type);
  }

  std::sort(timestamps.begin(), timestamps.end());
  timestamps.erase(std::unique(timestamps.begin(), timestamps.end()), timestamps.end());

  std::vector<DisPduBatch> batches;
  batches.reserve(timestamps.size());
  for (const auto ts : timestamps) {
    batches.push_back(batches_by_ts[ts]);
  }
  return batches;
}

std::vector<std::string> ScenarioReplayLoader::SplitCsv(const std::string& line) {
  std::vector<std::string> out;
  std::stringstream ss(line);
  std::string token;
  while (std::getline(ss, token, ',')) {
    out.push_back(token);
  }
  return out;
}

std::string ScenarioReplayLoader::Trim(const std::string& value) {
  std::size_t start = 0;
  while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
    ++start;
  }

  std::size_t end = value.size();
  while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
    --end;
  }
  return value.substr(start, end - start);
}

}  // namespace bas
