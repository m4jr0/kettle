// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#pragma once

#include <cstdint>
#include <ostream>

namespace kettle
{
constexpr uint32_t KettleMagic = 0x314C544B; // "KTL1"
constexpr uint32_t KettleVersion = 1;
constexpr uint32_t KettleEndian = 0x01020304;

enum class PinDirection : uint8_t
{
    Input = 0,
    Output = 1,
};

enum class ValueKind : uint8_t
{
    None = 0,

    Bool = 1,

    Int8 = 2,
    Int16 = 3,
    Int32 = 4,
    Int64 = 5,

    UInt8 = 6,
    UInt16 = 7,
    UInt32 = 8,
    UInt64 = 9,

    Float32 = 10,
    Float64 = 11,

    StringId = 12,

    ObjectHandle = 13,

    Vec2 = 14,
    Vec3 = 15,
    Vec4 = 16,

    Blob = 17,
};

const char* toString(ValueKind kind);

struct Value
{
    ValueKind kind = ValueKind::None;
    uint64_t payload = 0;
};

struct ObjectHandle
{
    uint32_t index = 0;
    uint32_t generation = 0;
};

inline uint64_t packObjectHandle(ObjectHandle handle)
{
    return (uint64_t(handle.generation) << 32) | uint64_t(handle.index);
}

inline ObjectHandle unpackObjectHandle(uint64_t payload)
{
    return ObjectHandle{
        static_cast<uint32_t>(payload & 0xffffffffu),
        static_cast<uint32_t>(payload >> 32),
    };
}

struct Vec2
{
    float x = 0.0f;
    float y = 0.0f;
};

struct Vec3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Vec4
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;
};

struct BlobHandle
{
    uint32_t offset;
    uint32_t size;
};

inline uint64_t packBlobHandle(uint32_t offset, uint32_t size)
{
    return (static_cast<uint64_t>(size) << 32) | static_cast<uint64_t>(offset);
}

inline BlobHandle unpackBlobHandle(uint64_t payload)
{
    return {
        .offset = static_cast<uint32_t>(payload),
        .size = static_cast<uint32_t>((payload >> 32)),
    };
}

inline std::ostream& operator<<(std::ostream& os, const ObjectHandle& h)
{
    return os << "Object[" << h.index << "#" << h.generation << "]";
}

inline std::ostream& operator<<(std::ostream& os, const Vec2& v)
{
    return os << "Vec2(" << v.x << ", " << v.y << ")";
}

inline std::ostream& operator<<(std::ostream& os, const Vec3& v)
{
    return os << "Vec3(" << v.x << ", " << v.y << ", " << v.z << ")";
}

inline std::ostream& operator<<(std::ostream& os, const Vec4& v)
{
    return os << "Vec4(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
}

inline std::ostream& operator<<(std::ostream& os, const BlobHandle& h)
{
    return os << "Blob[offset=" << h.offset << ", size=" << h.size << "]";
}

#pragma pack(push, 1)

struct Header
{
    uint32_t magic;
    uint32_t version;
    uint32_t endian;

    uint32_t stringTableOffset;
    uint32_t stringTableSize;

    uint32_t nodesOffset;
    uint32_t nodeCount;

    uint32_t pinsOffset;
    uint32_t pinCount;

    uint32_t linksOffset;
    uint32_t linkCount;

    uint32_t propertiesOffset;
    uint32_t propertyCount;

    uint32_t propertyValuesOffset;
    uint32_t propertyValuesSize;

    uint32_t reserved0;
    uint32_t reserved1;
};

struct NodeRecord
{
    uint64_t typeId;
    uint64_t debugNameId;

    uint32_t id;
    uint32_t firstPin;
    uint32_t pinCount;

    uint32_t firstProperty;
    uint32_t propertyCount;

    uint32_t reserved0;
};

struct PinRecord
{
    uint32_t nodeId;
    uint8_t direction;
    uint8_t reserved0;
    uint16_t reserved1;

    uint64_t pinId;
    uint64_t valueTypeId;
};

struct LinkRecord
{
    uint32_t fromNode;
    uint32_t toNode;

    uint64_t fromPinId;
    uint64_t toPinId;
};

struct PropertyRecord
{
    uint64_t propertyId;

    uint8_t valueType;
    uint8_t reserved0;
    uint16_t reserved1;

    uint32_t valueOffset;
    uint32_t valueSize;
};

#pragma pack(pop)

static_assert(sizeof(Header) == 68);
static_assert(sizeof(NodeRecord) == 40);
static_assert(sizeof(PinRecord) == 24);
static_assert(sizeof(LinkRecord) == 24);
static_assert(sizeof(PropertyRecord) == 20);

} // namespace kettle