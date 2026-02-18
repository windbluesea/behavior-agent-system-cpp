#!/usr/bin/env bash
set -euo pipefail

MODEL_PATH="${1:-/home/sun/small/Qwen/Qwen1.5-1.8B-Chat}"
PORT="${BAS_QWEN_PORT:-8000}"
STARTUP_TIMEOUT_S="${BAS_QWEN_STARTUP_TIMEOUT_S:-600}"
REQUEST_TIMEOUT_MS="${BAS_QWEN_TIMEOUT_MS:-120000}"

python3 scripts/qwen_openai_server.py \
  --model-path "$MODEL_PATH" \
  --host 127.0.0.1 \
  --port "$PORT" \
  --model-name "Qwen1.5-1.8B-Chat" \
  --max-new-tokens 128 \
  --temperature 0.1 > /tmp/bas_qwen_server.log 2>&1 &
SERVER_PID=$!

cleanup() {
  kill "$SERVER_PID" >/dev/null 2>&1 || true
}
trap cleanup EXIT

for _ in $(seq 1 "${STARTUP_TIMEOUT_S}"); do
  if curl -fsS "http://127.0.0.1:${PORT}/health" >/dev/null; then
    break
  fi
  sleep 1
done

if ! curl -fsS "http://127.0.0.1:${PORT}/health" >/dev/null; then
  echo "Qwen server did not become ready. See /tmp/bas_qwen_server.log" >&2
  exit 1
fi

BAS_MODEL_BACKEND=openai \
BAS_QWEN_ENDPOINT="http://127.0.0.1:${PORT}/v1/chat/completions" \
BAS_QWEN_MODEL="Qwen1.5-1.8B-Chat" \
BAS_QWEN_TIMEOUT_MS="${REQUEST_TIMEOUT_MS}" \
./build/bas_demo
