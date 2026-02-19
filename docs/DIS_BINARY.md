# DIS Binary Parser

`DisBinaryParser` parses raw IEEE 1278-style DIS PDUs from binary files.

## Supported PDU Types

- **Entity State PDU** (`pdu_type=1`)
  - parsed fields: entity id, force id, type, velocity, location, orientation, appearance
- **Fire PDU** (`pdu_type=2`)
  - parsed fields: firing entity id, target entity id, location

Unsupported PDU types are rejected in strict mode.

## Strict Validation

- each PDU must include a full 12-byte DIS header
- `length` must be >= 12
- `length` must not exceed remaining bytes in stream
- minimum payload sizes enforced:
  - entity state: 88 bytes
  - fire: 64 bytes

Malformed input causes parser failure with byte offset diagnostics.

## CLI Usage

Parse binary summary:
```bash
./build/bas_dis_parse data/scenarios/demo_dis.bin
```

Replay with metrics from binary:
```bash
./build/bas_replay data/scenarios/demo_dis.bin
```

Generate demo binary file:
```bash
python3 scripts/generate_demo_dis_binary.py data/scenarios/demo_dis.bin
```
