# Copyright 2026 m4jr0. All Rights Reserved.
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE file.

import json
import struct
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable

MAGIC = 0x314C544B  # "KTL1"
VERSION = 1
ENDIAN = 0x01020304


# -----------------------------------------------------------------------------
# Kettle value types
# -----------------------------------------------------------------------------

KETTLE_TYPES = {
    "NONE": "none",
    "BOOL": "bool",
    "INT8": "int8",
    "UINT8": "uint8",
    "INT16": "int16",
    "UINT16": "uint16",
    "INT32": "int32",
    "UINT32": "uint32",
    "INT64": "int64",
    "UINT64": "uint64",
    "FLOAT32": "float32",
    "FLOAT64": "float64",
    "STRING": "string",
    "STRING_ID": "string_id",
    "OBJECT": "object",
    "OBJECT_HANDLE": "object_handle",
    "VEC2": "vec2",
    "VEC3": "vec3",
    "VEC4": "vec4",
    "BLOB": "blob",
}


@dataclass(frozen=True)
class ValueDef:
    id: int
    pack: Callable[[Any, "StringTable"], bytes]


def range_check(name: str, value: int, lo: int, hi: int) -> None:
    if value < lo or value > hi:
        raise RuntimeError(f"{name}={value} outside [{lo}, {hi}]")


def pack_none(value: Any, strings: "StringTable") -> bytes:
    return b""


def pack_bool(value: Any, strings: "StringTable") -> bytes:
    return struct.pack("<B", 1 if value else 0)


def pack_int8(value: Any, strings: "StringTable") -> bytes:
    value = int(value)
    range_check("int8", value, -128, 127)
    return struct.pack("<b", value)


def pack_uint8(value: Any, strings: "StringTable") -> bytes:
    value = int(value)
    range_check("uint8", value, 0, 255)
    return struct.pack("<B", value)


def pack_int16(value: Any, strings: "StringTable") -> bytes:
    value = int(value)
    range_check("int16", value, -32768, 32767)
    return struct.pack("<h", value)


def pack_uint16(value: Any, strings: "StringTable") -> bytes:
    value = int(value)
    range_check("uint16", value, 0, 65535)
    return struct.pack("<H", value)


def pack_int32(value: Any, strings: "StringTable") -> bytes:
    value = int(value)
    range_check("int32", value, -2147483648, 2147483647)
    return struct.pack("<i", value)


def pack_uint32(value: Any, strings: "StringTable") -> bytes:
    value = int(value)
    range_check("uint32", value, 0, 4294967295)
    return struct.pack("<I", value)


def pack_int64(value: Any, strings: "StringTable") -> bytes:
    return struct.pack("<q", int(value))


def pack_uint64(value: Any, strings: "StringTable") -> bytes:
    return struct.pack("<Q", int(value))


def pack_float32(value: Any, strings: "StringTable") -> bytes:
    return struct.pack("<f", float(value))


def pack_float64(value: Any, strings: "StringTable") -> bytes:
    return struct.pack("<d", float(value))


def pack_string(value: Any, strings: "StringTable") -> bytes:
    return struct.pack("<Q", strings.add(str(value)))


def pack_object_handle(value: Any, strings: "StringTable") -> bytes:
    index = int(value["index"])
    generation = int(value["generation"])

    range_check("object.index", index, 0, 4294967295)
    range_check("object.generation", generation, 0, 4294967295)

    packed = (generation << 32) | index
    return struct.pack("<Q", packed)


def pack_vec(value: Any, count: int, type_name: str) -> bytes:
    if not isinstance(value, list) or len(value) != count:
        raise RuntimeError(f"{type_name} requires {count} values")

    return struct.pack("<" + "f" * count, *[float(v) for v in value])


def pack_vec2(value: Any, strings: "StringTable") -> bytes:
    return pack_vec(value, 2, KETTLE_TYPES["VEC2"])


def pack_vec3(value: Any, strings: "StringTable") -> bytes:
    return pack_vec(value, 3, KETTLE_TYPES["VEC3"])


def pack_vec4(value: Any, strings: "StringTable") -> bytes:
    return pack_vec(value, 4, KETTLE_TYPES["VEC4"])


def pack_blob(value: Any, strings: "StringTable") -> bytes:
    return json.dumps(value, separators=(",", ":")).encode("utf-8")


VALUE_DEFS = {
    KETTLE_TYPES["NONE"]: ValueDef(0, pack_none),
    KETTLE_TYPES["BOOL"]: ValueDef(1, pack_bool),
    KETTLE_TYPES["INT8"]: ValueDef(2, pack_int8),
    KETTLE_TYPES["INT16"]: ValueDef(3, pack_int16),
    KETTLE_TYPES["INT32"]: ValueDef(4, pack_int32),
    KETTLE_TYPES["INT64"]: ValueDef(5, pack_int64),
    KETTLE_TYPES["UINT8"]: ValueDef(6, pack_uint8),
    KETTLE_TYPES["UINT16"]: ValueDef(7, pack_uint16),
    KETTLE_TYPES["UINT32"]: ValueDef(8, pack_uint32),
    KETTLE_TYPES["UINT64"]: ValueDef(9, pack_uint64),
    KETTLE_TYPES["FLOAT32"]: ValueDef(10, pack_float32),
    KETTLE_TYPES["FLOAT64"]: ValueDef(11, pack_float64),
    KETTLE_TYPES["STRING"]: ValueDef(12, pack_string),
    KETTLE_TYPES["STRING_ID"]: ValueDef(12, pack_string),
    KETTLE_TYPES["OBJECT"]: ValueDef(13, pack_object_handle),
    KETTLE_TYPES["OBJECT_HANDLE"]: ValueDef(13, pack_object_handle),
    KETTLE_TYPES["VEC2"]: ValueDef(14, pack_vec2),
    KETTLE_TYPES["VEC3"]: ValueDef(15, pack_vec3),
    KETTLE_TYPES["VEC4"]: ValueDef(16, pack_vec4),
    KETTLE_TYPES["BLOB"]: ValueDef(17, pack_blob),
}


# -----------------------------------------------------------------------------
# Helpers
# -----------------------------------------------------------------------------


def write_file(path, data, *, binary=False, encoding="utf-8"):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)

    mode = "wb" if binary else "w"

    with path.open(mode, encoding=None if binary else encoding) as f:
        f.write(data)


def write_json(path, obj, *, indent=2):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)

    with path.open("w", encoding="utf-8") as f:
        json.dump(obj, f, indent=indent, ensure_ascii=False)


def fnv1a64(text: str) -> int:
    h = 14695981039346656037

    for b in text.encode("utf-8"):
        h ^= b
        h = (h * 1099511628211) & 0xFFFFFFFFFFFFFFFF

    return h


def align(x: int, a: int) -> int:
    return (x + a - 1) & ~(a - 1)


class StringTable:
    def __init__(self):
        self.data = bytearray()
        self.map: dict[int, str] = {}

    def add(self, text: str) -> int:
        h = fnv1a64(text)

        if h in self.map:
            if self.map[h] != text:
                raise RuntimeError(
                    f"Hash collision: 0x{h:016X} for " f"'{self.map[h]}' and '{text}'"
                )
            return h

        offset = len(self.data)
        encoded = text.encode("utf-8")
        header_size = struct.calcsize("<QIH")

        self.data += struct.pack("<QIH", h, offset + header_size, len(encoded))
        self.data += encoded
        self.data += b"\0"

        self.map[h] = text
        return h

    def debug_map(self) -> dict[str, str]:
        return {f"0x{h:016X}": text for h, text in sorted(self.map.items())}


def pack_typed_value(prop: dict[str, Any], strings: StringTable) -> tuple[int, bytes]:
    type_name = str(prop["type"]).lower()
    value = prop.get("value")

    value_def = VALUE_DEFS.get(type_name)
    if not value_def:
        raise RuntimeError(f"Unknown value type: {type_name}")

    return value_def.id, value_def.pack(value, strings)


# -----------------------------------------------------------------------------
# Main
# -----------------------------------------------------------------------------


def main() -> None:
    if len(sys.argv) != 3:
        print("Usage: python tools/pack_graph.py graph.json tmp/graph.ktl")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]

    with open(input_path, "r", encoding="utf-8") as f:
        graph = json.load(f)

    graph_version = int(graph.get("version", -1))
    if graph_version != VERSION:
        raise RuntimeError(
            f"Unsupported graph version: {graph_version}. Expected {VERSION}."
        )

    strings = StringTable()

    node_records: list[bytes] = []
    pin_records: list[bytes] = []
    link_records: list[bytes] = []
    property_records: list[bytes] = []
    property_values = bytearray()

    nodes = graph.get("nodes", [])
    links = graph.get("links", [])

    for node in nodes:
        node_id = int(node["id"])

        type_id = strings.add(node["type"])
        debug_name_id = strings.add(node.get("name", ""))

        first_pin = len(pin_records)

        for pin in node.get("pins", []):
            direction = 1 if pin["direction"] == "output" else 0

            pin_records.append(
                struct.pack(
                    "<IB3xQQ",
                    node_id,
                    direction,
                    strings.add(pin["name"]),
                    strings.add(pin.get("type", "any")),
                )
            )

        first_property = len(property_records)
        properties = node.get("properties", {})

        for key, prop in properties.items():
            property_id = strings.add(key)
            value_kind, raw = pack_typed_value(prop, strings)

            value_offset = len(property_values)
            property_values += raw

            padded = align(len(property_values), 8)
            property_values += b"\0" * (padded - len(property_values))

            property_records.append(
                struct.pack(
                    "<QB3xII",
                    property_id,
                    value_kind,
                    value_offset,
                    len(raw),
                )
            )

        node_records.append(
            struct.pack(
                "<QQIIIIII",
                type_id,
                debug_name_id,
                node_id,
                first_pin,
                len(node.get("pins", [])),
                first_property,
                len(properties),
                0,
            )
        )

    for link in links:
        link_records.append(
            struct.pack(
                "<IIQQ",
                int(link["from_node"]),
                int(link["to_node"]),
                strings.add(link["from_pin"]),
                strings.add(link["to_pin"]),
            )
        )

    header_size = struct.calcsize("<17I")

    string_table_offset = header_size
    string_table_size = len(strings.data)

    nodes_offset = align(string_table_offset + string_table_size, 16)
    node_bytes = b"".join(node_records)

    pins_offset = align(nodes_offset + len(node_bytes), 16)
    pin_bytes = b"".join(pin_records)

    links_offset = align(pins_offset + len(pin_bytes), 16)
    link_bytes = b"".join(link_records)

    properties_offset = align(links_offset + len(link_bytes), 16)
    property_bytes = b"".join(property_records)

    property_values_offset = align(properties_offset + len(property_bytes), 16)
    property_values_size = len(property_values)

    header = struct.pack(
        "<17I",
        MAGIC,
        VERSION,
        ENDIAN,
        string_table_offset,
        string_table_size,
        nodes_offset,
        len(node_records),
        pins_offset,
        len(pin_records),
        links_offset,
        len(link_records),
        properties_offset,
        len(property_records),
        property_values_offset,
        property_values_size,
        0,
        0,
    )

    out = bytearray()
    out += header

    out += strings.data
    out += b"\0" * (nodes_offset - len(out))

    out += node_bytes
    out += b"\0" * (pins_offset - len(out))

    out += pin_bytes
    out += b"\0" * (links_offset - len(out))

    out += link_bytes
    out += b"\0" * (properties_offset - len(out))

    out += property_bytes
    out += b"\0" * (property_values_offset - len(out))

    out += property_values

    write_file(output_path, out, binary=True)

    debug_string_path = output_path + ".strings.json"
    write_json(debug_string_path, strings.debug_map())

    print(f"Wrote {output_path}")
    print(f"Nodes: {len(node_records)}")
    print(f"Pins: {len(pin_records)}")
    print(f"Links: {len(link_records)}")
    print(f"Properties: {len(property_records)}")
    print(f"String table: {string_table_size} bytes")
    print(f"Property values: {property_values_size} bytes")
    print(f"Total: {len(out)} bytes")
    print(f"String debug map: {debug_string_path}")


if __name__ == "__main__":
    main()
