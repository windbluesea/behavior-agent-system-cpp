# 回放格式说明

回放加载器支持 `.bas` 文本文件，采用逗号分隔记录。

## 记录类型

- `ENV,timestamp_ms,visibility_m,weather_risk,terrain_risk`
- `ENTITY,timestamp_ms,id,side,type,x,y,z,speed_mps,heading_deg,alive,threat_level`
- `FIRE,timestamp_ms,shooter_id,target_id,weapon_name,x,y,z`

## 字段取值

- `side`：`friendly` / `hostile` / `neutral`
- `type`：`infantry` / `armor` / `artillery` / `air_defense` / `command`
- `alive`：`1/0` 或 `true/false`

## 规则与说明

- 以 `#` 开头的行为注释
- 系统按 `timestamp_ms` 将记录归并为时间帧
- 示例文件：`data/scenarios/demo_replay.bas`
- `alive` 由 `1 -> 0` 的变化会用于计算回放指标：
  - 敌方损失归因（命中贡献）
  - 我方生存率
