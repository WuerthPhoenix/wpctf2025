#!/usr/bin/env python3

with open("flag.txt", "r") as f:
    flag = f.read().strip()


MUL = 0x7F3A21C9
ADD_BASE = 0x1337


def transform_byte(i: int, b: int) -> int:
    v = (b * MUL) & 0xFFFFFFFF
    rot = i % 13
    v = ((v << rot) | (v >> (32 - rot))) & 0xFFFFFFFF
    v = (v + (ADD_BASE * (i + 1))) & 0xFFFFFFFF
    return v


encoded = [transform_byte(i, ord(b)) for i, b in enumerate(flag)]
print(", ".join(str(v) for v in encoded))
