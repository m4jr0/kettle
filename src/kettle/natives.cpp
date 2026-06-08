// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#include "bytecode.h"
#include "graph_view.h"
#include "hash.h"

#include <cstring>
#include <iostream>
#include "compiler.h"

namespace kettle
{
struct NativeContext
{
    const GraphView* graph = nullptr;
    const Program* program = nullptr;
};

static NativeContext gNativeContext;

void setNativeContext(const GraphView* graph, const Program* program)
{
    gNativeContext.graph = graph;
    gNativeContext.program = program;
}

static void nativePrint(Value* registers, uint8_t firstArg, uint16_t argCount, Value*)
{
    if (argCount != 1)
        throw std::runtime_error("Print expects 1 arg");

    const Value& value = registers[firstArg];
    const char* prefix = "[Print] ";

    switch (value.kind)
    {
    case ValueKind::Bool:
        std::cout << prefix << (value.payload ? "true" : "false") << '\n';
        break;

    case ValueKind::Int8:
        std::cout << prefix << readInlineValue<int8_t>(value) << '\n';
        break;

    case ValueKind::Int16:
        std::cout << prefix << readInlineValue<int16_t>(value) << '\n';
        break;

    case ValueKind::Int32:
        std::cout << prefix << readInlineValue<int32_t>(value) << '\n';
        break;

    case ValueKind::Int64:
        std::cout << prefix << readInlineValue<int64_t>(value) << '\n';
        break;

    case ValueKind::UInt8:
        std::cout << prefix << readInlineValue<uint8_t>(value) << '\n';
        break;

    case ValueKind::UInt16:
        std::cout << prefix << readInlineValue<uint16_t>(value) << '\n';
        break;

    case ValueKind::UInt32:
        std::cout << prefix << readInlineValue<uint32_t>(value) << '\n';
        break;

    case ValueKind::UInt64:
        std::cout << prefix << readInlineValue<uint64_t>(value) << '\n';
        break;

    case ValueKind::Float32:
        std::cout << prefix << readInlineValue<float>(value) << '\n';
        break;

    case ValueKind::Float64:
        std::cout << prefix << readInlineValue<double>(value) << '\n';
        break;


    case ValueKind::StringId:
        std::cout << prefix << gNativeContext.graph->findDebugString(value.payload) << '\n';
        break;

    case ValueKind::ObjectHandle:
        std::cout << prefix << readInlineValue<ObjectHandle>(value) << '\n';
        break;

    case ValueKind::Vec2:
        std::cout << prefix << gNativeContext.program->readConstValue<Vec2>(value.payload) << '\n';
        break;

    case ValueKind::Vec3:
        std::cout << prefix << gNativeContext.program->readConstValue<Vec3>(value.payload) << '\n';
        break;

    case ValueKind::Vec4:
        std::cout << prefix << gNativeContext.program->readConstValue<Vec4>(value.payload) << '\n';
        break;

    case ValueKind::Blob:
        std::cout << prefix << readInlineValue<BlobHandle>(value) << '\n';
        break;

    default:
        throw std::runtime_error("Print unsupported value kind");
    }
}

static void nativeGetPlayer(Value*, uint8_t, uint16_t, Value* result)
{
    ObjectHandle player{1, 1};
    *result = Value{ValueKind::ObjectHandle, packObjectHandle(player)};
}

static void nativeGetHealth(Value* registers, uint8_t firstArg, uint16_t argCount, Value* result)
{
    static float health = 40.0f;

    if (argCount != 1)
        throw std::runtime_error("GetHealth expects 1 arg");

    const Value& player = registers[firstArg];

    if (player.kind != ValueKind::ObjectHandle)
        throw std::runtime_error("GetHealth expects object handle");

    uint32_t bits = 0;
    std::memcpy(&bits, &health, sizeof(float));
    health -= 10.0f;

    *result = Value{ValueKind::Float32, bits};
}

static void nativeSetPosition(Value* registers, uint8_t firstArg, uint16_t argCount, Value*)
{
    if (argCount != 2)
        throw std::runtime_error("SetPosition expects target and position");

    Value rawTarget = registers[firstArg + 0];
    Value rawPos = registers[firstArg + 1];

    if (rawTarget.kind != ValueKind::ObjectHandle)
        throw std::runtime_error("target must be object");

    if (rawPos.kind != ValueKind::Vec3)
        throw std::runtime_error("position must be Vec3");

    ObjectHandle target = unpackObjectHandle(rawTarget.payload);
    auto pos = gNativeContext.program->readConstValue<Vec3>(rawPos.payload);

    std::cout << "SetPosition (" << target << "):" << pos << '\n';
}

void registerBuiltinNatives(NativeRegistry& registry)
{
    registry.add(hash("Print"), nativePrint);
    registry.add(hash("SetPosition"), nativeSetPosition);
    registry.add(hash("GetPlayer"), nativeGetPlayer);
    registry.add(hash("GetHealth"), nativeGetHealth);
}
} // namespace kettle