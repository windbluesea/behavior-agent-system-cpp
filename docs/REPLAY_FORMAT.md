# Replay Format

The replay loader accepts `.bas` text files with comma-separated records.

## Record Types

- `ENV,timestamp_ms,visibility_m,weather_risk,terrain_risk`
- `ENTITY,timestamp_ms,id,side,type,x,y,z,speed_mps,heading_deg,alive,threat_level`
- `FIRE,timestamp_ms,shooter_id,target_id,weapon_name,x,y,z`

## Allowed Values

- `side`: `friendly`, `hostile`, `neutral`
- `type`: `infantry`, `armor`, `artillery`, `air_defense`, `command`
- `alive`: `1/0` or `true/false`

## Notes

- lines starting with `#` are comments
- records are grouped by `timestamp_ms` into replay frames
- sample file: `data/scenarios/demo_replay.bas`
