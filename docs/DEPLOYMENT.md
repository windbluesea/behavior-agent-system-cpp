# 部署与运行指南

## 1）构建与测试
```bash
cmake -S . -B build -DBAS_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## 2）运行基础演示（Mock 后端）
```bash
./build/bas_demo
```

## 3）运行文本回放（Mock 后端）
```bash
./build/bas_replay data/scenarios/demo_replay.bas
```
回放格式见：`docs/REPLAY_FORMAT.md`

回放输出包含：
- 命中贡献率
- 生存率
- 射手毁伤贡献

## 4）严格 DIS 二进制解析与回放
先生成示例二进制：
```bash
python3 scripts/generate_demo_dis_binary.py data/scenarios/demo_dis.bin
```

解析统计：
```bash
./build/bas_dis_parse data/scenarios/demo_dis.bin
```

直接回放并评估：
```bash
./build/bas_replay data/scenarios/demo_dis.bin
```

解析器说明见：`docs/DIS_BINARY.md`

## 5）接入本地 Qwen（OpenAI 兼容）
启动本地模型服务：
```bash
python3 scripts/qwen_openai_server.py --model-path /home/sun/small/Qwen/Qwen1.5-1.8B-Chat
```

设置环境变量：
```bash
export BAS_MODEL_BACKEND="openai"
export BAS_QWEN_ENDPOINT="http://127.0.0.1:8000/v1/chat/completions"
export BAS_QWEN_MODEL="Qwen1.5-1.8B-Chat"
export BAS_QWEN_TIMEOUT_MS="120000"
# export BAS_QWEN_API_KEY="..."   # 可选
```

运行演示：
```bash
./build/bas_demo
```

一键联调：
```bash
scripts/run_qwen_demo.sh /home/sun/small/Qwen/Qwen1.5-1.8B-Chat
```

若 CPU 启动较慢，可增加等待时间：
```bash
BAS_QWEN_STARTUP_TIMEOUT_S=900 scripts/run_qwen_demo.sh /home/sun/small/Qwen/Qwen1.5-1.8B-Chat
```

## 6）推荐运行参数
- 决策缓存 TTL：2~5 秒
- 事件记忆窗口：5 分钟
- 模型 `max_tokens`：128~192
- 模型超时：200~300 毫秒（CPU 场景可更高）

## 7）CI 说明
GitHub Actions 工作流位于：`.github/workflows/ci.yml`
