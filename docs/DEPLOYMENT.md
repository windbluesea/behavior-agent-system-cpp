# Deployment Guide

## 1) Build
```bash
cmake -S . -B build -DBAS_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## 2) Run Demo (Mock Model)
```bash
./build/bas_demo
```

## 3) Connect to Local Qwen (OpenAI-Compatible)
Set environment variables:
```bash
export BAS_MODEL_BACKEND="openai"
export BAS_QWEN_ENDPOINT="http://127.0.0.1:8000/v1/chat/completions"
export BAS_QWEN_MODEL="Qwen1.5-1.8B-Chat"
# export BAS_QWEN_API_KEY="..."   # optional
```

Then run:
```bash
./build/bas_demo
```

## 4) Recommended Runtime Settings
- decision cache TTL: 2-5 seconds
- memory window: 5 minutes
- model max tokens: 128-192
- model timeout: 200-300 ms

## 5) CI
GitHub Actions workflow is defined in `.github/workflows/ci.yml`.
