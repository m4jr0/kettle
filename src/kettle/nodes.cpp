// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#include "compiler.h"

#include <cstdint>
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

    const auto targetInput = c.resolveInput(node.id, TargetPin);

    const IrValueId targetValue = c.createValue();
    c.emitMove(targetValue, targetInput.value);

    const IrValueId positionValue = c.createValue();
    c.resolveOrLoadIntoOperand(positionValue, node, PositionPin, PositionProperty, ValueKind::Vec3);

    const uint16_t nativeIndex = c.natives.resolve(NativeSetPosition);

    c.emit({
        .op = IrOp::CallNative,
        .result = kInvalidIrValueId,
        .nativeIndex = nativeIndex,
        .args = {targetValue, positionValue},
    });
}

static void compileGetPlayer(Compiler& c, const NodeRecord& node)
{
    constexpr uint64_t NativeGetPlayer = hash("GetPlayer");
    constexpr uint64_t PlayerPin = hash("player");

    const IrValueId result = c.createValue();
    const uint16_t nativeIndex = c.natives.resolve(NativeGetPlayer);

    c.emit({
        .op = IrOp::CallNative,
        .result = result,
        .nativeIndex = nativeIndex,
        .args = {},
    });

    c.rememberValue(node.id, PlayerPin, result);
}

static void compileGetHealth(Compiler& c, const NodeRecord& node)
{
    constexpr uint64_t NativeGetHealth = hash("GetHealth");
    constexpr uint64_t PlayerPin = hash("player");
    constexpr uint64_t HealthPin = hash("health");

    const auto player = c.resolveInput(node.id, PlayerPin);
    const uint16_t nativeIndex = c.natives.resolve(NativeGetHealth);

    const IrValueId result = c.createValue();

    c.emit({
        .op = IrOp::CallNative,
        .result = result,
        .nativeIndex = nativeIndex,
        .args = {player.value},
    });

    c.rememberValue(node.id, HealthPin, result);
}

static void compileConstant(Compiler& c, const NodeRecord& node)
{
    constexpr uint64_t ValueTypeProp = hash("valueType");
    constexpr uint64_t ValueProp = hash("value");
    constexpr uint64_t ValuePin = hash("value");

    const ValueKind valueKind = resolveValueKind(c, node, ValueTypeProp);
    const Value value = c.materializePropertyAsValue(node, ValueProp, valueKind);

    const IrValueId valueId = c.createValue();

    c.emit({
        .op = IrOp::LoadValue,
        .dst = valueId,
        .value = value,
    });

    c.rememberValue(node.id, ValuePin, valueId);
}

static void compileCompare(Compiler& c, const NodeRecord& node)
{
    constexpr uint64_t APin = hash("a");
    constexpr uint64_t BPin = hash("b");
    constexpr uint64_t ResultPin = hash("result");

    const ValueKind valueKind = resolveValueKind(c, node, hash("valueType"));
    const OpCode op = resolveCompareOp(c, node, valueKind, hash("op"));

    const auto a = c.resolveInput(node.id, APin);
    const auto b = c.resolveInput(node.id, BPin);

    const IrValueId result = c.createValue();

    c.emit({
        .op = IrOp::Compare,
        .bytecodeOp = op,
        .dst = result,
        .src0 = a.value,
        .src1 = b.value,
    });

    c.rememberValue(node.id, ResultPin, result);
}

static void compilePrint(Compiler& c, const NodeRecord& node)
{
    constexpr uint64_t NativePrint = hash("Print");
    constexpr uint64_t MessageProperty = hash("message");
    constexpr uint64_t ValuePin = hash("value");

    auto arg = c.resolveOrLoadValueOperand(node, ValuePin, MessageProperty, ValueKind::StringId);
    const uint16_t nativeIndex = c.natives.resolve(NativePrint);

    c.emit({
        .op = IrOp::CallNative,
        .result = kInvalidIrValueId,
        .nativeIndex = nativeIndex,
        .args = {arg.value},
    });
}

static void compileBranch(Compiler& c, const NodeRecord& node)
{
    constexpr uint64_t ConditionPin = hash("condition");
    constexpr uint64_t TruePin = hash("true");
    constexpr uint64_t FalsePin = hash("false");

    const auto cond = c.resolveInput(node.id, ConditionPin);

    const IrBlockId trueBlock = c.ir.createBlock();
    const IrBlockId falseBlock = c.ir.createBlock();
    const IrBlockId endBlock = c.ir.createBlock();

    c.emitJumpIfFalse(cond.value, falseBlock);
    c.emitJump(trueBlock);
    c.ir.setCurrentBlock(trueBlock);

    if (const LinkRecord* trueLink = c.graph.findExecLink(node.id, TruePin))
    {
        const NodeRecord* trueNode = c.graph.findNode(trueLink->toNode);
        if (!trueNode)
            throw std::runtime_error("broken true branch");

        c.compileNodeWithDependencies(*trueNode);
    }

    c.emitJump(endBlock);

    c.ir.setCurrentBlock(falseBlock);

    if (const LinkRecord* falseLink = c.graph.findExecLink(node.id, FalsePin))
    {
        const NodeRecord* falseNode = c.graph.findNode(falseLink->toNode);
        if (!falseNode)
            throw std::runtime_error("broken false branch");

        c.compileNodeWithDependencies(*falseNode);
    }

    c.emitJump(endBlock);

    c.ir.setCurrentBlock(endBlock);
}

static void compileWhile(Compiler& c, const NodeRecord& node)
{
    constexpr uint64_t ConditionPin = hash("condition");
    constexpr uint64_t BodyPin = hash("body");
    constexpr uint64_t ThenPin = hash("then");

    const IrBlockId conditionBlock = c.ir.createBlock();
    const IrBlockId bodyBlock = c.ir.createBlock();
    const IrBlockId afterBlock = c.ir.createBlock();

    c.emitJump(conditionBlock);

    c.ir.setCurrentBlock(conditionBlock);

    c.compileInputDependency(node, ConditionPin);

    const auto cond = c.resolveInput(node.id, ConditionPin);

    c.emitJumpIfFalse(cond.value, afterBlock);
    c.emitJump(bodyBlock);
    c.ir.setCurrentBlock(bodyBlock);

    if (const LinkRecord* bodyLink = c.graph.findExecLink(node.id, BodyPin))
    {
        const NodeRecord* bodyNode = c.graph.findNode(bodyLink->toNode);
        if (!bodyNode)
            throw std::runtime_error("broken while body");

        c.compileNodeWithDependencies(*bodyNode);
    }

    c.emitJump(conditionBlock);

    c.ir.setCurrentBlock(afterBlock);

    if (const LinkRecord* thenLink = c.graph.findExecLink(node.id, ThenPin))
    {
        const NodeRecord* thenNode = c.graph.findNode(thenLink->toNode);
        if (!thenNode)
            throw std::runtime_error("broken while continuation");

        c.compileNodeWithDependencies(*thenNode);
    }
}

void registerBuiltinNodeCompilers(NodeCompilerRegistry& registry)
{
    registry.add(hash("SetPosition"), compileSetPosition);
    registry.add(hash("GetPlayer"), compileGetPlayer);
    registry.add(hash("GetHealth"), compileGetHealth);
    registry.add(hash("Constant"), compileConstant);
    registry.add(hash("Compare"), compileCompare);
    registry.add(hash("Print"), compilePrint);
    registry.add(hash("Branch"), compileBranch, DependencyPolicy::Auto, FlowPolicy::OwnsContinuation);
    registry.add(hash("While"), compileWhile, DependencyPolicy::Manual, FlowPolicy::OwnsContinuation);
}
} // namespace kettle