# Task Breakdown

## Workstream 1: Data Ingestion
- owner: core-platform
- deliverables:
  - DIS-like PDU structures and adapter
  - event extraction for memory stream

## Workstream 2: Situation & Memory
- owner: reasoning
- deliverables:
  - tactical semantic tagging
  - temporal memory query and context assembly

## Workstream 3: Fire Decision
- owner: combat-logic
- deliverables:
  - target threat index model
  - weapon-target assignment strategy
  - focus/stagger fire coordination

## Workstream 4: Maneuver Decision
- owner: mobility
- deliverables:
  - threat-aware path planning
  - tactical movement selection
  - formation mode control (disperse/assemble)

## Workstream 5: Inference & Optimization
- owner: runtime
- deliverables:
  - mock and OpenAI-compatible model backend
  - cache integration and INT8 runtime flag
  - latency smoke validation

## Workstream 6: Validation & Documentation
- owner: quality
- deliverables:
  - unit/integration tests with CTest
  - CI workflow
  - architecture/deployment/API docs
