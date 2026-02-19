#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "bas/dis/dis_binary_parser.hpp"

namespace {

void PushU8(std::vector<std::uint8_t>& out, std::uint8_t value) { out.push_back(value); }

void PushU16BE(std::vector<std::uint8_t>& out, std::uint16_t value) {
  out.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
  out.push_back(static_cast<std::uint8_t>(value & 0xFFU));
}

void PushU32BE(std::vector<std::uint8_t>& out, std::uint32_t value) {
  out.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
  out.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
  out.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
  out.push_back(static_cast<std::uint8_t>(value & 0xFFU));
}

void PushF32BE(std::vector<std::uint8_t>& out, float value) {
  std::uint32_t raw = 0;
  std::memcpy(&raw, &value, sizeof(raw));
  PushU32BE(out, raw);
}

void PushF64BE(std::vector<std::uint8_t>& out, double value) {
  std::uint64_t raw = 0;
  std::memcpy(&raw, &value, sizeof(raw));
  out.push_back(static_cast<std::uint8_t>((raw >> 56U) & 0xFFU));
  out.push_back(static_cast<std::uint8_t>((raw >> 48U) & 0xFFU));
  out.push_back(static_cast<std::uint8_t>((raw >> 40U) & 0xFFU));
  out.push_back(static_cast<std::uint8_t>((raw >> 32U) & 0xFFU));
  out.push_back(static_cast<std::uint8_t>((raw >> 24U) & 0xFFU));
  out.push_back(static_cast<std::uint8_t>((raw >> 16U) & 0xFFU));
  out.push_back(static_cast<std::uint8_t>((raw >> 8U) & 0xFFU));
  out.push_back(static_cast<std::uint8_t>(raw & 0xFFU));
}

std::vector<std::uint8_t> BuildEntityPdu(std::uint32_t timestamp, bool alive) {
  std::vector<std::uint8_t> out;
  const std::uint16_t length = 144;

  // Header
  PushU8(out, 7);
  PushU8(out, 1);
  PushU8(out, 1);
  PushU8(out, 1);
  PushU32BE(out, timestamp);
  PushU16BE(out, length);
  PushU16BE(out, 0);

  // Entity ID
  PushU16BE(out, 1);
  PushU16BE(out, 1);
  PushU16BE(out, 1);

  // Force ID + articulation
  PushU8(out, 1);
  PushU8(out, 0);

  // Entity type (kind/domain/country/category/subcat/specific/extra)
  PushU8(out, 1);  // platform
  PushU8(out, 1);  // land
  PushU16BE(out, 225);
  PushU8(out, 1);  // category => armor
  PushU8(out, 0);
  PushU8(out, 0);
  PushU8(out, 0);

  // Alt entity type
  for (int i = 0; i < 8; ++i) PushU8(out, 0);

  // Linear velocity
  PushF32BE(out, 3.0f);
  PushF32BE(out, 4.0f);
  PushF32BE(out, 0.0f);

  // Location
  PushF64BE(out, 100.0);
  PushF64BE(out, 200.0);
  PushF64BE(out, 5.0);

  // Orientation psi/theta/phi
  PushF32BE(out, 0.5f);
  PushF32BE(out, 0.0f);
  PushF32BE(out, 0.0f);

  // Appearance
  std::uint32_t appearance = alive ? 0U : (3U << 3U);
  PushU32BE(out, appearance);

  // Remaining bytes to length
  while (out.size() < length) {
    PushU8(out, 0);
  }
  return out;
}

std::vector<std::uint8_t> BuildFirePdu(std::uint32_t timestamp) {
  std::vector<std::uint8_t> out;
  const std::uint16_t length = 96;

  // Header
  PushU8(out, 7);
  PushU8(out, 1);
  PushU8(out, 2);
  PushU8(out, 2);
  PushU32BE(out, timestamp);
  PushU16BE(out, length);
  PushU16BE(out, 0);

  // Firing Entity ID
  PushU16BE(out, 1);
  PushU16BE(out, 1);
  PushU16BE(out, 1);

  // Target Entity ID
  PushU16BE(out, 2);
  PushU16BE(out, 2);
  PushU16BE(out, 2);

  // Munition Entity ID + Event ID + Fire mission index
  for (int i = 0; i < 16; ++i) PushU8(out, 0);

  // Location in world coords
  PushF64BE(out, 101.0);
  PushF64BE(out, 202.0);
  PushF64BE(out, 3.0);

  while (out.size() < length) {
    PushU8(out, 0);
  }
  return out;
}

}  // namespace

int main() {
  std::vector<std::uint8_t> bytes;
  const auto entity_pdu = BuildEntityPdu(1000, true);
  const auto fire_pdu = BuildFirePdu(1000);
  bytes.insert(bytes.end(), entity_pdu.begin(), entity_pdu.end());
  bytes.insert(bytes.end(), fire_pdu.begin(), fire_pdu.end());

  bas::DisBinaryParser parser;
  const auto batches = parser.ParseBytes(bytes);

  if (batches.size() != 1) {
    std::cerr << "expected one timestamp batch\n";
    return EXIT_FAILURE;
  }

  if (batches[0].entity_updates.size() != 1 || batches[0].fire_events.size() != 1) {
    std::cerr << "unexpected entity/fire counts\n";
    return EXIT_FAILURE;
  }

  const auto& entity = batches[0].entity_updates[0];
  if (entity.entity_id != "1-1-1" || entity.side != bas::Side::Friendly || entity.type != bas::UnitType::Armor) {
    std::cerr << "entity basic fields mismatch\n";
    return EXIT_FAILURE;
  }
  if (!entity.alive || std::fabs(entity.speed_mps - 5.0) > 1e-6) {
    std::cerr << "entity alive/speed mismatch\n";
    return EXIT_FAILURE;
  }

  const auto& fire = batches[0].fire_events[0];
  if (fire.shooter_id != "1-1-1" || fire.target_id != "2-2-2") {
    std::cerr << "fire ids mismatch\n";
    return EXIT_FAILURE;
  }

  bool threw = false;
  try {
    std::vector<std::uint8_t> bad = bytes;
    bad.pop_back();
    static_cast<void>(parser.ParseBytes(bad));
  } catch (const std::exception&) {
    threw = true;
  }
  if (!threw) {
    std::cerr << "expected strict parser to reject malformed bytes\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
