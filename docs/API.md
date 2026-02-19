# API Overview

## Core Input
- `BattlefieldSnapshot`: friendly/hostile entities + environment state
- `EventRecord`: temporal memory events (weapon fire, contacts, tags)

## Core Output
- `DecisionPackage`
  - `fire`: threats and shooter-target assignments
  - `maneuver`: tactical movement actions and paths
  - `explanation`: natural-language explanation (model generated)
  - `from_cache`: cache-hit flag

## Integration Entry
- `AgentPipeline::Tick(snapshot, dis_events)`
  - feeds event memory
  - fuses semantics
  - runs fire + maneuver engines
  - calls model ranker
  - writes/reads decision cache

## Replay Support
- `ScenarioReplayLoader::LoadBatches(path)`
  - loads `.bas` replay records (`ENV`/`ENTITY`/`FIRE`)
  - produces timestamp-grouped `DisPduBatch` frames

## Model Runtime Backends
- `ModelBackend::Mock`
- `ModelBackend::OpenAICompatible`
