// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#include "compiler.h"

#include <cstring>
#include <stdexcept>

namespace kettle
{
static ValueKind resolveValueKind(Compiler& c, const NodeRecord& node, uint64_t propKey)
{
    Value valueKindValue = c.materializePropertyAsValue(node, propKey, ValueKind::StringId);
    uint64_t rawValueKind = valueKindValue.payload;

    switch (rawValueKind)
    {
    case hash("bool"):
        return ValueKind::Bool;

    case hash("int8"):
        return ValueKind::Int8;
    case hash("int16"):
        return ValueKind::Int16;
    case hash("int32"):
        return ValueKind::Int32;
    case hash("int64"):
        return ValueKind::Int64;

    case hash("uint8"):
        return ValueKind::UInt8;
    case hash("uint16"):
        return ValueKind::UInt16;
    case hash("uint32"):
        return ValueKind::UInt32;
    case hash("uint64"):
        return ValueKind::UInt64;

    case hash("float32"):
        return ValueKind::Float32;
    case hash("float64"):
        return ValueKind::Float64;

    case hash("string"):
        return ValueKind::StringId;
    case hash("object"):
        return ValueKind::ObjectHandle;

    case hash("vec2"):
        return ValueKind::Vec2;
    case hash("vec3"):
        return ValueKind::Vec3;
    case hash("vec4"):
        return ValueKind::Vec4;

    case hash("blob"):
        return ValueKind::Blob;

    default:
        throw std::runtime_error("unknown value kind");
    }
}


static OpCode resolveCompareOp(Compiler& c, const NodeRecord& node, ValueKind valueKind, uint64_t propKey)
{
    Value cmpOpValue = c.materializePropertyAsValue(node, propKey, ValueKind::StringId);
    const uint64_t rawCmpOp = cmpOpValue.payload;

#define KETTLE_COMPARE_NUMERIC(KIND, SUFFIX) \
    case ValueKind::KIND: \
        switch (rawCmpOp) \
        { \
        case hash("<"): \
            return OpCode::Less##SUFFIX; \
        case hash("<="): \
            return OpCode::LessEqual##SUFFIX; \
        case hash("=="): \
            return OpCode::Equal##SUFFIX; \
        case hash("!="): \
            return OpCode::NotEqual##SUFFIX; \
        case hash(">="): \
            return OpCode::GreaterEqual##SUFFIX; \
        case hash(">"): \
            return OpCode::Greater##SUFFIX; \
        default: \
            throw std::runtime_error("unsupported numeric compare operator"); \
        }

#define KETTLE_COMPARE_EQUAL_ONLY(KIND, SUFFIX) \
    case ValueKind::KIND: \
        switch (rawCmpOp) \
        { \
        case hash("=="): \
            return OpCode::Equal##SUFFIX; \
        case hash("!="): \
            return OpCode::NotEqual##SUFFIX; \
        default: \
            throw std::runtime_error("only == and != are supported for this value kind"); \
        }

    switch (valueKind)
    {
        KETTLE_COMPARE_EQUAL_ONLY(Bool, Bool)

        KETTLE_COMPARE_NUMERIC(Int8, Int8)
        KETTLE_COMPARE_NUMERIC(Int16, Int16)
        KETTLE_COMPARE_NUMERIC(Int32, Int32)
        KETTLE_COMPARE_NUMERIC(Int64, Int64)

        KETTLE_COMPARE_NUMERIC(UInt8, UInt8)
        KETTLE_COMPARE_NUMERIC(UInt16, UInt16)
        KETTLE_COMPARE_NUMERIC(UInt32, UInt32)
        KETTLE_COMPARE_NUMERIC(UInt64, UInt64)

        KETTLE_COMPARE_NUMERIC(Float32, Float32)
        KETTLE_COMPARE_NUMERIC(Float64, Float64)

        KETTLE_COMPARE_EQUAL_ONLY(StringId, StringId)
        KETTLE_COMPARE_EQUAL_ONLY(ObjectHandle, ObjectHandle)

        KETTLE_COMPARE_EQUAL_ONLY(Vec2, Vec2)
        KETTLE_COMPARE_EQUAL_ONLY(Vec3, Vec3)
        KETTLE_COMPARE_EQUAL_ONLY(Vec4, Vec4)

    case ValueKind::None:
        throw std::runtime_error("cannot compare none values");

    case ValueKind::Blob:
        throw std::runtime_error("blob compare is not supported");

    default:
        throw std::runtime_error("unknown value kind for compare");
    }

#undef KETTLE_COMPARE_NUMERIC
#undef KETTLE_COMPARE_EQUAL_ONLY
}

static void compileSetPosition(Compiler& c, const NodeRecord& node)
{
    constexpr uint64_t NativeSetPosition = hash("SetPosition");
    constexpr uint64_t TargetPin = hash("target");
    constexpr uint64_t PositionPin = hash("position");
    constexpr uint64_t PositionProperty = hash("position");

    const uint8_t targetInputReg = c.resolveInputRegister(node.id, TargetPin);

    const uint8_t targetReg = c.allocRegisters(2);
    const uint8_t positionReg = static_cast<uint8_t>(targetReg + 1);

    c.emitMove(targetReg, targetInputReg);
    c.resolveOrLoadInto(positionReg, node, PositionPin, PositionProperty, ValueKind::Vec3);

    const uint16_t nativeIndex = c.natives.resolve(NativeSetPosition);

    c.emit({
        .op = OpCode::CallNative,
        .a = targetReg,
        .b = 2,
        .c = nativeIndex,
    });
}

static void compileGetPlayer(Compiler& c, const NodeRecord& node)
{
    constexpr uint64_t NativeGetPlayer = hash("GetPlayer");
    constexpr uint64_t PlayerPin = hash("player");

    uint8_t resultReg = c.allocRegister();
    uint16_t nativeIndex = c.natives.resolve(NativeGetPlayer);

    c.emit({
        .op = OpCode::CallNative,
        .a = resultReg,
        .b = 0,
        .c = nativeIndex,
    });

    c.rememberValue(node.id, PlayerPin, resultReg);
}

static void compileGetHealth(Compiler& c, const NodeRecord& node)
{
    constexpr uint64_t NativeGetHealth = hash("GetHealth");
    constexpr uint64_t PlayerPin = hash("player");
    constexpr uint64_t HealthPin = hash("health");

    uint8_t playerReg = c.resolveInputRegister(node.id, PlayerPin);
    uint16_t nativeIndex = c.natives.resolve(NativeGetHealth);

    c.emit({
        .op = OpCode::CallNative,
        .a = playerReg,
        .b = 1,
        .c = nativeIndex,
    });

    c.rememberValue(node.id, HealthPin, playerReg);
}

static void compileConstant(Compiler& c, const NodeRecord& node)
{
    constexpr uint64_t ValueTypeProp = hash("valueType");
    constexpr uint64_t ValueProp = hash("value");
    constexpr uint64_t ValuePin = hash("value");

    const ValueKind valueKind = resolveValueKind(c, node, ValueTypeProp);
    const Value value = c.materializePropertyAsValue(node, ValueProp, valueKind);

    const uint8_t valueReg = c.allocRegister();

    c.emit({
        .op = OpCode::LoadValue,
        .a = valueReg,
        .b = static_cast<uint16_t>(value.kind),
        .c = value.payload,
    });

    c.rememberValue(node.id, ValuePin, valueReg);
}

static void compileCompare(Compiler& c, const NodeRecord& node)
{
    constexpr uint64_t APin = hash("a");
    constexpr uint64_t BPin = hash("b");
    constexpr uint64_t ResultPin = hash("result");

    const ValueKind valueKind = resolveValueKind(c, node, hash("valueType"));
    const OpCode op = resolveCompareOp(c, node, valueKind, hash("op"));

    const uint8_t aReg = c.resolveInputRegister(node.id, APin);
    const uint8_t bReg = c.resolveInputRegister(node.id, BPin);
    const uint8_t resultReg = c.allocRegister();

    c.emit({
        .op = op,
        .a = resultReg,
        .b = aReg,
        .c = bReg,
    });

    c.rememberValue(node.id, ResultPin, resultReg);
}

static void compilePrint(Compiler& c, const NodeRecord& node)
{
    constexpr uint64_t NativePrint = hash("Print");
    constexpr uint64_t MessageProperty = hash("message");
    constexpr uint64_t ValuePin = hash("value");

    const uint8_t argReg = c.resolveOrLoadValue(node, ValuePin, MessageProperty, ValueKind::StringId);
    const uint16_t nativeIndex = c.natives.resolve(NativePrint);

    c.emit({
        .op = OpCode::CallNative,
        .a = argReg,
        .b = 1,
        .c = nativeIndex,
    });
}

static void compileBranch(Compiler& c, const NodeRecord& node)
{
    constexpr uint64_t ConditionPin = hash("condition");
    constexpr uint64_t TruePin = hash("true");
    constexpr uint64_t FalsePin = hash("false");

    uint8_t condReg = c.resolveInputRegister(node.id, ConditionPin);

    const size_t jumpFalseIndex = c.program.code.size();
    c.emit({
        .op = OpCode::JumpIfFalse,
        .a = condReg,
        .b = 0,
        .c = 0,
    });

    if (const LinkRecord* trueLink = c.graph.findExecLink(node.id, TruePin))
    {
        const NodeRecord* trueNode = c.graph.findNode(trueLink->toNode);
        if (!trueNode)
            throw std::runtime_error("broken true branch");

        c.compileNodeWithDependencies(*trueNode);
    }

    const size_t jumpEndIndex = c.program.code.size();
    c.emit({
        .op = OpCode::Jump,
        .a = 0,
        .b = 0,
        .c = 0,
    });

    const size_t falseLabel = c.program.code.size();
    c.program.code[jumpFalseIndex].c = falseLabel;

    if (const LinkRecord* falseLink = c.graph.findExecLink(node.id, FalsePin))
    {
        const NodeRecord* falseNode = c.graph.findNode(falseLink->toNode);
        if (!falseNode)
            throw std::runtime_error("broken false branch");

        c.compileNodeWithDependencies(*falseNode);
    }

    const size_t endLabel = c.program.code.size();
    c.program.code[jumpEndIndex].c = endLabel;
}

void registerBuiltinNodeCompilers(NodeCompilerRegistry& registry)
{
    registry.add(hash("SetPosition"), compileSetPosition);
    registry.add(hash("GetPlayer"), compileGetPlayer);
    registry.add(hash("GetHealth"), compileGetHealth);
    registry.add(hash("Constant"), compileConstant);
    registry.add(hash("Compare"), compileCompare);
    registry.add(hash("Print"), compilePrint);
    registry.add(hash("Branch"), compileBranch);
}
} // namespace kettle