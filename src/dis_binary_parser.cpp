#include "bas/dis/dis_binary_parser.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace bas {

namespace {

constexpr std::size_t kDisHeaderLength = 12;

std::string BuildError(const std::string& msg, std::size_t offset) {
  std::ostringstream oss;
  oss << msg << " at byte offset " << offset;
  return oss.str();
}

double ClampThreat(double value) {
  if (value < 0.0) {
    return 0.0;
  }
  if (value > 1.0) {
    return 1.0;
  }
  return value;
}

}  // namespace

std::vector<DisPduBatch> DisBinaryParser::ParseFile(const std::string& path) const {
  std::ifstream ifs(path, std::ios::binary);
  if (!ifs) {
    throw std::runtime_error("Cannot open DIS binary file: " + path);
  }

  std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  if (bytes.empty()) {
    return {};
  }
  return ParseBytes(bytes);
}

std::vector<DisPduBatch> DisBinaryParser::ParseBytes(const std::vector<std::uint8_t>& bytes) const {
  std::map<std::uint32_t, DisPduBatch> by_timestamp;

  std::size_t offset = 0;
  while (offset < bytes.size()) {
    if (bytes.size() - offset < kDisHeaderLength) {
      throw std::runtime_error(BuildError("Incomplete DIS header", offset));
    }

    const DisPduHeader header = ParseHeader(bytes, offset);
    if (header.length < kDisHeaderLength) {
      throw std::runtime_error(BuildError("Invalid PDU length < 12", offset));
    }
    if (offset + header.length > bytes.size()) {
      throw std::runtime_error(BuildError("PDU length exceeds file size", offset));
    }

    if (header.pdu_type == 1) {
      by_timestamp[header.timestamp].entity_updates.push_back(ParseEntityStatePdu(bytes, offset, header.length));
    } else if (header.pdu_type == 2) {
      by_timestamp[header.timestamp].fire_events.push_back(ParseFirePdu(bytes, offset, header.length));
    } else {
      throw std::runtime_error(BuildError("Unsupported PDU type " + std::to_string(header.pdu_type), offset));
    }

    offset += header.length;
  }

  std::vector<DisPduBatch> batches;
  batches.reserve(by_timestamp.size());
  for (auto& [_, batch] : by_timestamp) {
    batches.push_back(std::move(batch));
  }
  return batches;
}

DisPduHeader DisBinaryParser::ParseHeader(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
  DisPduHeader h;
  h.protocol_version = bytes[offset + 0];
  h.exercise_id = bytes[offset + 1];
  h.pdu_type = bytes[offset + 2];
  h.protocol_family = bytes[offset + 3];
  h.timestamp = ReadU32BE(bytes, offset + 4);
  h.length = ReadU16BE(bytes, offset + 8);
  h.padding = ReadU16BE(bytes, offset + 10);
  return h;
}

DisEntityPdu DisBinaryParser::ParseEntityStatePdu(const std::vector<std::uint8_t>& bytes,
                                                  std::size_t offset,
                                                  std::size_t length) {
  if (length < 88) {
    throw std::runtime_error(BuildError("Entity State PDU too short", offset));
  }

  const DisPduHeader header = ParseHeader(bytes, offset);
  DisEntityPdu out;
  out.timestamp_ms = static_cast<std::int64_t>(header.timestamp);
  out.entity_id = ParseEntityId(bytes, offset + 12);
  out.side = ParseForceId(bytes[offset + 18]);
  out.type = ParseUnitType(bytes, offset + 20);

  const float vx = ReadF32BE(bytes, offset + 36);
  const float vy = ReadF32BE(bytes, offset + 40);
  const float vz = ReadF32BE(bytes, offset + 44);
  out.speed_mps = std::sqrt(static_cast<double>(vx) * static_cast<double>(vx) +
                            static_cast<double>(vy) * static_cast<double>(vy) +
                            static_cast<double>(vz) * static_cast<double>(vz));

  out.pose.x = ReadF64BE(bytes, offset + 48);
  out.pose.y = ReadF64BE(bytes, offset + 56);
  out.pose.z = ReadF64BE(bytes, offset + 64);

  const double psi_rad = static_cast<double>(ReadF32BE(bytes, offset + 72));
  out.heading_deg = psi_rad * (180.0 / 3.14159265358979323846);

  const std::uint32_t appearance = ReadU32BE(bytes, offset + 84);
  const std::uint32_t damage = (appearance >> 3U) & 0x3U;
  out.alive = (damage < 3U);

  double base_threat = 0.3;
  switch (out.type) {
    case UnitType::Armor:
      base_threat = 0.9;
      break;
    case UnitType::Artillery:
      base_threat = 0.85;
      break;
    case UnitType::AirDefense:
      base_threat = 0.8;
      break;
    case UnitType::Command:
      base_threat = 0.75;
      break;
    case UnitType::Infantry:
      base_threat = 0.55;
      break;
    default:
      base_threat = 0.3;
      break;
  }
  out.threat_level = ClampThreat(base_threat + out.speed_mps * 0.01);
  return out;
}

DisFirePdu DisBinaryParser::ParseFirePdu(const std::vector<std::uint8_t>& bytes, std::size_t offset, std::size_t length) {
  if (length < 64) {
    throw std::runtime_error(BuildError("Fire PDU too short", offset));
  }

  const DisPduHeader header = ParseHeader(bytes, offset);
  DisFirePdu out;
  out.timestamp_ms = static_cast<std::int64_t>(header.timestamp);
  out.shooter_id = ParseEntityId(bytes, offset + 12);
  out.target_id = ParseEntityId(bytes, offset + 18);

  out.weapon_name = "munition";
  out.origin.x = ReadF64BE(bytes, offset + 40);
  out.origin.y = ReadF64BE(bytes, offset + 48);
  out.origin.z = ReadF64BE(bytes, offset + 56);
  return out;
}

std::uint16_t DisBinaryParser::ReadU16BE(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
  return static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[offset]) << 8U) |
                                    static_cast<std::uint16_t>(bytes[offset + 1]));
}

std::uint32_t DisBinaryParser::ReadU32BE(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
  return (static_cast<std::uint32_t>(bytes[offset]) << 24U) | (static_cast<std::uint32_t>(bytes[offset + 1]) << 16U) |
         (static_cast<std::uint32_t>(bytes[offset + 2]) << 8U) | static_cast<std::uint32_t>(bytes[offset + 3]);
}

float DisBinaryParser::ReadF32BE(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
  const std::uint32_t raw = ReadU32BE(bytes, offset);
  float value = 0.0f;
  std::memcpy(&value, &raw, sizeof(value));
  return value;
}

double DisBinaryParser::ReadF64BE(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
  std::uint64_t raw = 0;
  for (int i = 0; i < 8; ++i) {
    raw = (raw << 8U) | static_cast<std::uint64_t>(bytes[offset + i]);
  }

  double value = 0.0;
  std::memcpy(&value, &raw, sizeof(value));
  return value;
}

std::string DisBinaryParser::ParseEntityId(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
  const std::uint16_t site = ReadU16BE(bytes, offset + 0);
  const std::uint16_t app = ReadU16BE(bytes, offset + 2);
  const std::uint16_t entity = ReadU16BE(bytes, offset + 4);
  return std::to_string(site) + "-" + std::to_string(app) + "-" + std::to_string(entity);
}

Side DisBinaryParser::ParseForceId(std::uint8_t force_id) {
  switch (force_id) {
    case 1:
      return Side::Friendly;
    case 2:
      return Side::Hostile;
    case 3:
      return Side::Neutral;
    default:
      return Side::Neutral;
  }
}

UnitType DisBinaryParser::ParseUnitType(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
  const std::uint8_t kind = bytes[offset + 0];
  const std::uint8_t domain = bytes[offset + 1];
  const std::uint8_t category = bytes[offset + 4];

  if (kind != 1) {
    return UnitType::Unknown;
  }

  if (domain == 1) {
    if (category <= 3) {
      return UnitType::Armor;
    }
    if (category >= 4 && category <= 6) {
      return UnitType::Artillery;
    }
    if (category >= 7 && category <= 9) {
      return UnitType::Infantry;
    }
  }
  if (domain == 2 || domain == 4) {
    return UnitType::AirDefense;
  }
  return UnitType::Unknown;
}

}  // namespace bas
