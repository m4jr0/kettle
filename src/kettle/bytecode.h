// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#pragma once

#include "format.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace kettle
{
enum class OpCode : uint8_t
{
    Nop = 0,

    LoadValue,
    Move,
    CallNative,

    // Bool.
    EqualBool,
    NotEqualBool,

    // Signed integers.
    LessInt8,
    LessEqualInt8,
    EqualInt8,
    NotEqualInt8,
    GreaterEqualInt8,
    GreaterInt8,

    LessInt16,
    LessEqualInt16,
    EqualInt16,
    NotEqualInt16,
    GreaterEqualInt16,
    GreaterInt16,

    LessInt32,
    LessEqualInt32,
    EqualInt32,
    NotEqualInt32,
    GreaterEqualInt32,
    GreaterInt32,

    LessInt64,
    LessEqualInt64,
    EqualInt64,
    NotEqualInt64,
    GreaterEqualInt64,
    GreaterInt64,

    // Unsigned integers.
    LessUInt8,
    LessEqualUInt8,
    EqualUInt8,
    NotEqualUInt8,
    GreaterEqualUInt8,
    GreaterUInt8,

    LessUInt16,
    LessEqualUInt16,
    EqualUInt16,
    NotEqualUInt16,
    GreaterEqualUInt16,
    GreaterUInt16,

    LessUInt32,
    LessEqualUInt32,
    EqualUInt32,
    NotEqualUInt32,
    GreaterEqualUInt32,
    GreaterUInt32,

    LessUInt64,
    LessEqualUInt64,
    EqualUInt64,
    NotEqualUInt64,
    GreaterEqualUInt64,
    GreaterUInt64,

    // Floating point.
    LessFloat32,
    LessEqualFloat32,
    EqualFloat32,
    NotEqualFloat32,
    GreaterEqualFloat32,
    GreaterFloat32,

    LessFloat64,
    LessEqualFloat64,
    EqualFloat64,
    NotEqualFloat64,
    GreaterEqualFloat64,
    GreaterFloat64,

    // IDs / handles.
    EqualStringId,
    NotEqualStringId,

    EqualObjectHandle,
    NotEqualObjectHandle,

    // Vectors.
    EqualVec2,
    NotEqualVec2,

    EqualVec3,
    NotEqualVec3,

    EqualVec4,
    NotEqualVec4,

    // Control flow.
    Jump,
    JumpIfFalse,
    JumpIfTrue,

    Return,
};

const char* toString(OpCode op);


template<typename T>
inline T readInlineValue(const Value& value)
{
    return static_cast<T>(value.payload);
}

template<>
inline float readInlineValue<float>(const Value& value)
{
    auto bits = static_cast<uint32_t>(value.payload);
    float out{};
    std::memcpy(&out, &bits, sizeof(float));
    return out;
}

template<>
inline double readInlineValue<double>(const Value& value)
{
    uint64_t bits = value.payload;
    double out{};
    std::memcpy(&out, &bits, sizeof(double));
    return out;
}

template<>
inline ObjectHandle readInlineValue<ObjectHandle>(const Value& value)
{
    return unpackObjectHandle(value.payload);
}

template<>
inline BlobHandle readInlineValue<BlobHandle>(const Value& value)
{
    return unpackBlobHandle(value.payload);
}

struct Instruction
{
    OpCode op = OpCode::Nop;
    uint8_t a = 0;
    uint16_t b = 0;
    uint64_t c = 0;
};

struct Program
{
    std::vector<Instruction> code;
    std::vector<std::byte> constData;
    uint8_t maxRegisterCount = 0;

    uint32_t addConstData(const void* data, uint32_t size);

    template<typename T>
    T readConstValue(uint64_t payload) const
    {
        const auto offset = static_cast<uint32_t>(payload);

        if (offset + sizeof(T) > constData.size())
            throw std::runtime_error("const read out of bounds");

        T out{};
        std::memcpy(&out, constData.data() + offset, sizeof(T));
        return out;
    }
};

using NativeFn = void (*)(Value* registers, uint8_t firstArg, uint16_t argCount, Value* result);

struct NativeEntry
{
    uint64_t id = 0;
    NativeFn fn = nullptr;
};

struct NativeRegistry
{
    static inline constexpr uint16_t MaxEntryCount = static_cast<uint16_t>(-1);
    std::vector<NativeEntry> entries;

    uint16_t add(uint64_t id, NativeFn fn);
    uint16_t resolve(uint64_t id) const;
    const NativeEntry& get(uint16_t index) const;
};

struct VM
{
    static inline constexpr auto MaxRegister = 255;
    Value registers[MaxRegister + 1]{};
    const NativeRegistry* natives = nullptr;
    std::vector<std::byte> scratchData;

    void execute(const Program& program);
};

std::string formatProgram(const Program& program);
void dumpProgram(const Program& program);
} // namespace kettle