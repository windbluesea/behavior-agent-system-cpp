# Testing Guide

## Full Test Suite
```bash
cmake -S . -B build -DBAS_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Run Single Tests
```bash
./build/test_memory
./build/test_fire_control
./build/test_maneuver
./build/test_pipeline
./build/test_replay_loader
./build/test_replay_pipeline
./build/test_latency_smoke
```

## Replay Smoke Check
```bash
./build/bas_replay data/scenarios/demo_replay.bas
```

## Qwen Integration Check
```bash
BAS_QWEN_STARTUP_TIMEOUT_S=900 BAS_QWEN_TIMEOUT_MS=180000 \
  scripts/run_qwen_demo.sh /home/sun/small/Qwen/Qwen1.5-1.8B-Chat
```
