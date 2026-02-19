# API 总览

## 核心输入
- `BattlefieldSnapshot`：包含我方/敌方实体状态与环境信息。
- `EventRecord`：用于时间记忆的事件（开火、接触、战术标签等）。

## 核心输出
- `DecisionPackage`
  - `fire`：威胁评估与射手-目标-武器分配结果。
  - `maneuver`：机动动作、路径与编队模式。
  - `explanation`：自然语言决策解释（模型生成）。
  - `from_cache`：是否命中决策缓存。

## 集成入口
- `AgentPipeline::Tick(snapshot, dis_events)`
  - 写入事件记忆
  - 融合战术语义
  - 执行火力与机动引擎
  - 调用模型排序解释
  - 读写决策缓存

## 回放支持
- `ScenarioReplayLoader::LoadBatches(path)`
  - 加载 `.bas` 文本回放（`ENV` / `ENTITY` / `FIRE`）
  - 生成按时间戳分组的 `DisPduBatch` 列表
- `DisBinaryParser::ParseFile(path)`
  - 严格解析 DIS 二进制 PDU（`Entity State` 与 `Fire`）
  - 生成按时间戳分组的 `DisPduBatch` 列表

## 回放评估指标
- `ReplayMetricsEvaluator`
  - `ObserveSnapshot(snapshot)`：统计存活状态变化
  - `ObserveDecision(ts, decision)`：记录毁伤贡献历史
  - `Finalize()` 输出：
    - `survival_rate`（生存率）
    - `hit_contribution_rate`（命中贡献率）
    - `shooter_kill_contribution`（按射手统计的毁伤贡献）

## 模型推理后端
- `ModelBackend::Mock`：确定性模拟后端，适合单测与性能烟测。
- `ModelBackend::OpenAICompatible`：对接本地 OpenAI 兼容接口（如 Qwen 服务）。
