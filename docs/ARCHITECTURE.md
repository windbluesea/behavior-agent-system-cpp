# Architecture

## System Positioning
The project targets real-time intelligent decisions in simulation with a lightweight 1.5B-scale model.
The architecture is intentionally hybrid:
1. deterministic hard constraints (weapon range, ammo, safety, geometry)
2. small-model candidate ranking and natural-language explanation

This keeps behavior stable while preserving adaptivity and interpretability.

## Decision Pipeline
1. **DIS Adapter** (`DisAdapter`)
   - Ingests DIS-like entity and fire PDUs
   - Builds a `BattlefieldSnapshot`
   - Emits event records into memory stream
2. **Situation Fusion** (`SituationFusion`)
   - Converts raw state into tactical tags
   - Example tags: `left_flank_exposed`, `enemy_armor_cluster_approaching`
3. **Event Memory** (`EventMemory`)
   - Maintains rolling event window
   - Supports temporal context in decision prompts
4. **Fire Control Engine** (`FireControlEngine`)
   - Computes target threat index
   - Selects shooter-target-weapon assignments
   - Supports focus fire and stagger fire
5. **Maneuver Engine** (`ManeuverEngine`)
   - Threat-aware local path planning
   - Tactical action selection and formation coordination
6. **Model Runtime** (`ModelRuntime`)
   - Ranks candidate summaries
   - Generates decision explanation
7. **Decision Cache** (`DecisionCache`)
   - Reuses recent decisions for similar situations

## Key Engineering Decisions
- Model output never bypasses hard constraints.
- Cache is keyed by coarse tactical features to reduce repeated compute.
- Runtime backend is pluggable (`Mock` / OpenAI-compatible local service).
- Path planning remains lightweight and deterministic for edge deployment.

## Performance Objective
- target: **P95 < 100 ms** end-to-end decision latency
- optimization levers:
  - INT8 inference mode flag
  - bounded planning horizon
  - short-lived decision cache
