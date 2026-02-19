# Behavior Agent System (C++)

C++ implementation of a lightweight behavior-intelligence agent system for battlefield simulation.
It follows a hybrid approach: hard tactical constraints first, 1.5B model ranking and explanation second.

## Implemented Capabilities
- DIS-like ingestion layer (`DisAdapter`) with entity/fire PDU batches
- Situation understanding with tactical semantic tags
- Event memory for temporal context and event recall
- Fire allocation engine with threat indexing, weapon-target matching, focus/stagger fire
- Maneuver engine with threat-aware path planning, formation disperse/assemble, emergency evasion
- Decision cache for repeated situations
- Model runtime adapter:
  - `Mock` backend (default for deterministic tests)
  - OpenAI-compatible local endpoint backend (for local Qwen deployment)

## Build & Test
```bash
cmake -S . -B build -DBAS_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/bas_demo
./build/bas_replay data/scenarios/demo_replay.bas
./build/bas_dis_parse data/scenarios/demo_dis.bin
```

## Using Local Qwen1.5-1.8B-Chat
The runtime can call an OpenAI-compatible endpoint.

Environment variables:
- `BAS_MODEL_BACKEND` (`openai` to enable endpoint call, default mock mode)
- `BAS_QWEN_ENDPOINT` (default `http://127.0.0.1:8000/v1/chat/completions`)
- `BAS_QWEN_MODEL` (default `Qwen1.5-1.8B-Chat`)
- `BAS_QWEN_API_KEY` (optional)
- `BAS_QWEN_TIMEOUT_MS` (request timeout, useful for CPU inference)

Example:
```bash
export BAS_MODEL_BACKEND=openai
export BAS_QWEN_ENDPOINT=http://127.0.0.1:8000/v1/chat/completions
export BAS_QWEN_MODEL=Qwen1.5-1.8B-Chat
export BAS_QWEN_TIMEOUT_MS=120000
./build/bas_demo
```

If you only have the model files locally, start the included lightweight OpenAI-compatible server:
```bash
python3 scripts/qwen_openai_server.py --model-path /home/sun/small/Qwen/Qwen1.5-1.8B-Chat
```

Or run one-step local validation:
```bash
scripts/run_qwen_demo.sh /home/sun/small/Qwen/Qwen1.5-1.8B-Chat
```
For CPU-only hosts, you can extend startup waiting time:
```bash
BAS_QWEN_STARTUP_TIMEOUT_S=900 scripts/run_qwen_demo.sh /home/sun/small/Qwen/Qwen1.5-1.8B-Chat
```

## Repository Layout
- `include/bas/common/` shared types
- `include/bas/dis/` DIS-like adapter interfaces
- `include/bas/situation/` semantic understanding
- `include/bas/memory/` event memory
- `include/bas/decision/` fire and maneuver engines
- `include/bas/inference/` model runtime
- `include/bas/system/` integrated decision pipeline
- `tests/` unit and integration tests
- `docs/` architecture and deployment docs
- `data/scenarios/` replay samples
- `scripts/` local model and replay utilities

## Validation Coverage
Current tests include:
- memory retention and retrieval
- fire allocation threat ranking and tactic assignment
- maneuver emergency action selection
- end-to-end pipeline and decision cache behavior
- latency smoke test (P95 target under 100 ms with mock backend)
- replay loader/pipeline smoke tests
- strict DIS binary parser tests
- replay metrics (hit contribution/survival rate) tests

Additional docs:
- replay file format: `docs/REPLAY_FORMAT.md`
- test operations: `docs/TESTING.md`
- strict DIS binary parser: `docs/DIS_BINARY.md`
