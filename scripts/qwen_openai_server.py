#!/usr/bin/env python3
import argparse
import json
import logging
import os
import time
import uuid
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Any, Dict, List

import torch
from transformers import AutoModelForCausalLM, AutoTokenizer

LOGGER = logging.getLogger("qwen_openai_server")


class ModelRunner:
    def __init__(self, model_path: str, max_new_tokens: int, temperature: float) -> None:
        self.model_path = model_path
        self.max_new_tokens = max_new_tokens
        self.temperature = temperature
        self.device = "cuda" if torch.cuda.is_available() else "cpu"

        started = time.time()
        LOGGER.info("正在加载模型：%s，设备：%s", model_path, self.device)
        self.tokenizer = AutoTokenizer.from_pretrained(model_path, trust_remote_code=True)
        dtype = torch.float16 if self.device == "cuda" else torch.float32
        self.model = AutoModelForCausalLM.from_pretrained(
            model_path,
            trust_remote_code=True,
            torch_dtype=dtype,
            device_map="auto" if self.device == "cuda" else None,
            low_cpu_mem_usage=True,
        )
        if self.device != "cuda":
            self.model = self.model.to(self.device)
        self.model.eval()
        LOGGER.info("模型加载完成，耗时 %.1f 秒", time.time() - started)

    def generate(self, messages: List[Dict[str, Any]], max_tokens: int, temperature: float) -> str:
        prompt_parts = []
        for msg in messages:
            role = msg.get("role", "user")
            content = msg.get("content", "")
            prompt_parts.append(f"[{role}] {content}")
        prompt_parts.append("[assistant]")
        prompt = "\n".join(prompt_parts)

        inputs = self.tokenizer(prompt, return_tensors="pt")
        inputs = {k: v.to(self.device) for k, v in inputs.items()}

        use_sampling = temperature > 1e-5
        out = self.model.generate(
            **inputs,
            max_new_tokens=max_tokens,
            do_sample=use_sampling,
            temperature=max(temperature, 1e-5),
            eos_token_id=self.tokenizer.eos_token_id,
            pad_token_id=self.tokenizer.eos_token_id,
        )
        new_tokens = out[0][inputs["input_ids"].shape[1] :]
        return self.tokenizer.decode(new_tokens, skip_special_tokens=True).strip()


class OpenAIHandler(BaseHTTPRequestHandler):
    runner: ModelRunner = None
    model_name: str = "Qwen1.5-1.8B-Chat"

    def _send_json(self, payload: Dict[str, Any], status: int = 200) -> None:
        data = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def _read_json(self) -> Dict[str, Any]:
        content_length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(content_length)
        if not body:
            return {}
        return json.loads(body.decode("utf-8"))

    def do_GET(self) -> None:
        if self.path == "/health":
            self._send_json({"status": "ok", "model": self.model_name})
            return
        if self.path == "/v1/models":
            self._send_json({
                "object": "list",
                "data": [{"id": self.model_name, "object": "model", "owned_by": "local"}],
            })
            return
        self._send_json({"error": "未找到接口"}, status=404)

    def do_POST(self) -> None:
        if self.path != "/v1/chat/completions":
            self._send_json({"error": "未找到接口"}, status=404)
            return

        try:
            req = self._read_json()
            messages = req.get("messages", [])
            if not isinstance(messages, list) or not messages:
                self._send_json({"error": "messages 字段必须为非空列表"}, status=400)
                return

            max_tokens = int(req.get("max_tokens", self.runner.max_new_tokens))
            max_tokens = max(1, min(max_tokens, 512))
            temperature = float(req.get("temperature", self.runner.temperature))

            started = time.time()
            content = self.runner.generate(messages, max_tokens=max_tokens, temperature=temperature)
            latency_ms = int((time.time() - started) * 1000)

            response = {
                "id": f"chatcmpl-{uuid.uuid4().hex[:12]}",
                "object": "chat.completion",
                "created": int(time.time()),
                "model": self.model_name,
                "choices": [
                    {
                        "index": 0,
                        "message": {"role": "assistant", "content": content},
                        "finish_reason": "stop",
                    }
                ],
                "usage": {"prompt_tokens": 0, "completion_tokens": 0, "total_tokens": 0},
                "latency_ms": latency_ms,
            }
            self._send_json(response)
        except Exception as exc:  # pylint: disable=broad-except
            LOGGER.exception("请求处理失败")
            self._send_json({"error": str(exc)}, status=500)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="本地Qwen模型的OpenAI兼容服务")
    parser.add_argument("--model-path", required=True, help="本地模型目录路径")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--model-name", default="Qwen1.5-1.8B-Chat")
    parser.add_argument("--max-new-tokens", type=int, default=192)
    parser.add_argument("--temperature", type=float, default=0.1)
    parser.add_argument("--log-level", default="INFO")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    logging.basicConfig(level=getattr(logging, args.log_level.upper(), logging.INFO))

    if not os.path.isdir(args.model_path):
        raise SystemExit(f"模型路径不存在: {args.model_path}")

    runner = ModelRunner(args.model_path, max_new_tokens=args.max_new_tokens, temperature=args.temperature)
    OpenAIHandler.runner = runner
    OpenAIHandler.model_name = args.model_name

    server = ThreadingHTTPServer((args.host, args.port), OpenAIHandler)
    LOGGER.info("服务已启动：http://%s:%d", args.host, args.port)
    server.serve_forever()


if __name__ == "__main__":
    main()
