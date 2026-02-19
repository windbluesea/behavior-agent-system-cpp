# DIS 二进制解析器

`DisBinaryParser` 用于解析符合 IEEE 1278 风格的 DIS 二进制 PDU。

## 支持的 PDU 类型

- **Entity State PDU**（`pdu_type=1`）
  - 解析字段：实体编号、阵营、类型、速度、位置、姿态、外观
- **Fire PDU**（`pdu_type=2`）
  - 解析字段：射手编号、目标编号、发射位置

在严格模式下，不支持的 PDU 类型会直接报错。

## 严格校验规则

- 每条 PDU 必须包含完整 12 字节头部
- `length` 必须 >= 12
- `length` 不得超过剩余字节数
- 负载最小长度约束：
  - Entity State：88 字节
  - Fire：64 字节

输入畸形时，解析器会返回包含字节偏移的错误信息。

## 命令行用法

解析二进制统计信息：
```bash
./build/bas_dis_parse data/scenarios/demo_dis.bin
```

对二进制回放并输出指标：
```bash
./build/bas_replay data/scenarios/demo_dis.bin
```

生成示例二进制文件：
```bash
python3 scripts/generate_demo_dis_binary.py data/scenarios/demo_dis.bin
```
