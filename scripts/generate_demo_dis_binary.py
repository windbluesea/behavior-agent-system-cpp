#!/usr/bin/env python3
import argparse
import math
import struct


def entity_pdu(ts, site, app, ent, force_id, category, x, y, z, speed, heading_deg, alive=True):
    length = 144
    header = struct.pack(">BBBBIHH", 7, 1, 1, 1, ts, length, 0)

    entity_id = struct.pack(">HHH", site, app, ent)
    force = struct.pack(">BB", force_id, 0)
    entity_type = struct.pack(">BBHBBBB", 1, 1, 225, category, 0, 0, 0)
    alt_entity_type = b"\x00" * 8

    vx = speed * math.cos(math.radians(heading_deg))
    vy = speed * math.sin(math.radians(heading_deg))
    velocity = struct.pack(">fff", vx, vy, 0.0)

    location = struct.pack(">ddd", x, y, z)
    psi = math.radians(heading_deg)
    orientation = struct.pack(">fff", psi, 0.0, 0.0)

    appearance = 0 if alive else (3 << 3)
    appearance_bytes = struct.pack(">I", appearance)

    payload = entity_id + force + entity_type + alt_entity_type + velocity + location + orientation + appearance_bytes
    if len(payload) > (length - 12):
        raise RuntimeError("实体PDU负载过长")
    payload += b"\x00" * ((length - 12) - len(payload))
    return header + payload


def fire_pdu(ts, shooter, target, x, y, z):
    length = 96
    header = struct.pack(">BBBBIHH", 7, 1, 2, 2, ts, length, 0)

    shooter_id = struct.pack(">HHH", *shooter)
    target_id = struct.pack(">HHH", *target)
    munition = b"\x00" * 6
    event_id = b"\x00" * 6
    fire_mission_index = struct.pack(">I", 0)
    location = struct.pack(">ddd", x, y, z)

    payload = shooter_id + target_id + munition + event_id + fire_mission_index + location
    payload += b"\x00" * ((length - 12) - len(payload))
    return header + payload


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("output", nargs="?", default="data/scenarios/demo_dis.bin")
    args = parser.parse_args()

    blob = b""
    blob += fire_pdu(950, (2, 2, 2), (1, 1, 1), -160.0, 140.0, 0.0)
    blob += entity_pdu(1000, 1, 1, 1, 1, 1, 0.0, 0.0, 0.0, 6.0, 15.0, alive=True)
    blob += entity_pdu(1000, 1, 1, 2, 1, 7, -25.0, -10.0, 0.0, 2.0, 20.0, alive=True)
    blob += entity_pdu(1000, 2, 2, 1, 2, 1, 380.0, 180.0, 0.0, 9.0, 210.0, alive=True)
    blob += entity_pdu(1000, 2, 2, 2, 2, 4, -160.0, 140.0, 0.0, 3.5, 195.0, alive=True)

    blob += entity_pdu(3000, 1, 1, 1, 1, 1, 45.0, 55.0, 0.0, 6.0, 35.0, alive=True)
    blob += entity_pdu(3000, 1, 1, 2, 1, 7, 10.0, 35.0, 0.0, 2.0, 40.0, alive=False)
    blob += entity_pdu(3000, 2, 2, 1, 2, 1, 270.0, 250.0, 0.0, 9.0, 230.0, alive=False)
    blob += entity_pdu(3000, 2, 2, 2, 2, 4, -200.0, 180.0, 0.0, 3.5, 205.0, alive=True)

    with open(args.output, "wb") as f:
        f.write(blob)

    print(f"已生成 {args.output}（{len(blob)} 字节）")


if __name__ == "__main__":
    main()
