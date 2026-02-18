# Architecture Notes

## Decision Pipeline
1. DIS Adapter ingests real-time battlefield snapshot.
2. Situation Fusion converts raw states into tactical semantic tags.
3. Event Memory tracks recent events for temporal context.
4. Fire + Maneuver engines generate candidate actions.
5. Model Runtime ranks candidates and emits explanation text.
6. Decision Cache stores outcomes for common situations.

## Design Principles
- Hard constraints first: range, ammo, geometry, and safety rules are enforced before model selection.
- Model as ranker: the 1.5B model chooses among constrained candidates and generates rationale.
- Real-time target: optimize for P95 latency under 100 ms with INT8 + cache.
