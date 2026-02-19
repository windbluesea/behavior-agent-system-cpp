#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "bas/dis/dis_adapter.hpp"

namespace bas {

struct DisPduHeader {
  std::uint8_t protocol_version = 0;
  std::uint8_t exercise_id = 0;
  std::uint8_t pdu_type = 0;
  std::uint8_t protocol_family = 0;
  std::uint32_t timestamp = 0;
  std::uint16_t length = 0;
  std::uint16_t padding = 0;
};

class DisBinaryParser {
 public:
  std::vector<DisPduBatch> ParseFile(const std::string& path) const;
  std::vector<DisPduBatch> ParseBytes(const std::vector<std::uint8_t>& bytes) const;

 private:
  static DisPduHeader ParseHeader(const std::vector<std::uint8_t>& bytes, std::size_t offset);
  static DisEntityPdu ParseEntityStatePdu(const std::vector<std::uint8_t>& bytes, std::size_t offset, std::size_t length);
  static DisFirePdu ParseFirePdu(const std::vector<std::uint8_t>& bytes, std::size_t offset, std::size_t length);

  static std::uint16_t ReadU16BE(const std::vector<std::uint8_t>& bytes, std::size_t offset);
  static std::uint32_t ReadU32BE(const std::vector<std::uint8_t>& bytes, std::size_t offset);
  static float ReadF32BE(const std::vector<std::uint8_t>& bytes, std::size_t offset);
  static double ReadF64BE(const std::vector<std::uint8_t>& bytes, std::size_t offset);

  static std::string ParseEntityId(const std::vector<std::uint8_t>& bytes, std::size_t offset);
  static Side ParseForceId(std::uint8_t force_id);
  static UnitType ParseUnitType(const std::vector<std::uint8_t>& bytes, std::size_t offset);
};

}  // namespace bas
