# 行为智能体系统（C++）

本项目面向仿真对抗场景，采用“硬约束规则 + 小模型排序解释”的混合架构，实现实时火力分配与机动决策。

## 已实现能力
- DIS 风格态势接入：实体状态与开火事件
- 态势语义理解：战术标签推断
- 事件记忆：时间窗检索与上下文拼接
- 火力决策：威胁评估、武器匹配、集火/梯次射击
- 机动决策：规避与跃进、编队分散/集结
- 推理后端：Mock 与 OpenAI 兼容本地模型（Qwen）
- 决策缓存：常见态势快速复用
- 严格 DIS 二进制解析（Entity State / Fire PDU）
- 回放评估指标：命中贡献率、生存率、射手贡献

## 构建与运行
```bash
cmake -S . -B build -DBAS_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure

# 基础演示
./build/bas_demo

# 文本回放
./build/bas_replay data/scenarios/demo_replay.bas

# 生成并解析 DIS 二进制
python3 scripts/generate_demo_dis_binary.py data/scenarios/demo_dis.bin
./build/bas_dis_parse data/scenarios/demo_dis.bin
./build/bas_replay data/scenarios/demo_dis.bin
```

## 本地 Qwen 接入
支持你本地模型目录：`/home/sun/small/Qwen/Qwen1.5-1.8B-Chat`

环境变量：
- `BAS_MODEL_BACKEND`：`openai` 表示启用本地接口
- `BAS_QWEN_ENDPOINT`：默认 `http://127.0.0.1:8000/v1/chat/completions`
- `BAS_QWEN_MODEL`：默认 `Qwen1.5-1.8B-Chat`
- `BAS_QWEN_API_KEY`：可选
- `BAS_QWEN_TIMEOUT_MS`：请求超时（CPU 场景建议放大）

一键联调：
```bash
BAS_QWEN_STARTUP_TIMEOUT_S=900 BAS_QWEN_TIMEOUT_MS=180000 \
  scripts/run_qwen_demo.sh /home/sun/small/Qwen/Qwen1.5-1.8B-Chat
```

## 目录说明
- `include/bas/`：核心头文件
- `src/`：核心实现
- `tests/`：单元与集成测试
- `data/scenarios/`：回放样例
- `scripts/`：本地模型与回放工具
- `docs/`：设计、部署、测试文档

## 测试覆盖
- 内存与事件检索测试
- 火力分配与协同策略测试
- 机动动作选择测试
- 端到端决策管线测试
- 回放加载与回放决策测试
- 严格 DIS 二进制解析测试
- 回放指标（命中贡献/生存率）测试
- 延迟烟测（P95）

## 相关文档
- 架构说明：`docs/ARCHITECTURE.md`
- 部署说明：`docs/DEPLOYMENT.md`
- 接口说明：`docs/API.md`
- 测试说明：`docs/TESTING.md`
- 回放格式：`docs/REPLAY_FORMAT.md`
- DIS 二进制解析：`docs/DIS_BINARY.md`
- 路线图：`docs/ROADMAP.md`
