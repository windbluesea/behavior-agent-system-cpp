# Behavior Agent System (C++)

Lightweight C++ skeleton for a behavior-intelligence agent system with a 1.5B-model-oriented architecture.

## Scope
- Real-time battlefield state ingest from DIS-like adapter
- Situation understanding and tactical semantic tagging
- Event memory for short-term temporal reasoning
- Fire allocation and maneuver decision engines
- Model-runtime slot for 1.5B model ranking + explanation
- Cache layer for repeated tactical patterns

## Quick Start
```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/bas_demo
```

## Repository Layout
- `include/bas/` core interfaces and data structures
- `src/` module implementations and demo loop
- `docs/ARCHITECTURE.md` module-level design notes

## Next Milestones
1. Replace mock DIS feeder with real PDU parser.
2. Add weapon/terrain configuration loaders.
3. Integrate INT8 runtime backend and latency benchmark.
4. Add scenario replay test suite.
