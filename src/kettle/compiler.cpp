// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#include "compiler.h"

#include <cstring>
#include <stdexcept>

#include "hash.h"

namespace kettle
{
void NodeCompilerRegistry::add(uint64_t nodeTypeId, NodeCompileFn fn)
{
    entries[nodeTypeId] = fn;
}

NodeCompileFn NodeCompilerRegistry::resolve(uint64_t nodeTypeId) const
{
    auto it = entries.find(nodeTypeId);

    if (it == entries.cend())
        throw std::runtime_error("missing node compiler");

    return it->second;
}

uint8_t Compiler::allocRegister()
{
    return allocRegisters(1);
}

uint8_t Compiler::allocRegisters(uint8_t count)
{
    if (count == 0)
        throw std::runtime_error("cannot allocate zero registers");

    if (static_cast<uint16_t>(nextRegister) + count > VM::MaxRegister + 1)
        throw std::runtime_error("too many registers");

    const uint8_t alloc = nextRegister;
    nextRegister += count;

    program.registerCount = nextRegister;
    return alloc;
}

void Compiler::emit(Instruction ins)
{
    program.code.push_back(ins);
}

void Compiler::emitMove(uint8_t dst, uint8_t src)
{
    emit({
        .op = OpCode::Move,
        .a = dst,
        .b = src,
        .c = 0,
    });
}

void Compiler::rememberValue(uint32_t nodeId, uint64_t pinId, uint8_t reg)
{
    produced.emplace_back(nodeId, pinId, reg);
}

uint8_t Compiler::resolveInputRegister(uint32_t nodeId, uint64_t inputPinId) const
{
    auto reg = tryResolveInputRegister(nodeId, inputPinId);

    if (!reg)
        throw std::runtime_error("missing input link");

    return *reg;
}

std::optional<uint8_t> Compiler::tryResolveInputRegister(uint32_t nodeId, uint64_t inputPinId) const
{
    const LinkRecord* link = graph.findInputLink(nodeId, inputPinId);

    if (!link)
        return std::nullopt;

    for (const ProducedValue& value : produced)
    {
        if (value.nodeId == link->fromNode && value.pinId == link->fromPinId)
            return value.reg;
    }

    throw std::runtime_error("input source has not been compiled yet");
}

uint8_t Compiler::resolveOrLoadValue(const NodeRecord& node, uint64_t pinId, uint64_t propertyId, ValueKind expectedKind)
{
    if (auto inputReg = tryResolveInputRegister(node.id, pinId))
        return *inputReg;

    const Value value = materializePropertyAsValue(node, propertyId, expectedKind);
    const uint8_t reg = allocRegister();

    emit({
        .op = OpCode::LoadValue,
        .a = reg,
        .b = static_cast<uint16_t>(value.kind),
        .c = value.payload,
    });

    return reg;
}

void Compiler::resolveOrLoadInto(uint8_t dstReg, const NodeRecord& node, uint64_t pinId, uint64_t propertyId, ValueKind expectedKind)
{
    if (auto inputReg = tryResolveInputRegister(node.id, pinId))
    {
        emitMove(dstReg, *inputReg);
        return;
    }

    const Value value = materializePropertyAsValue(node, propertyId, expectedKind);

    emit({
        .op = OpCode::LoadValue,
        .a = dstReg,
        .b = static_cast<uint16_t>(value.kind),
        .c = value.payload,
    });
}

Value Compiler::materializePropertyAsValue(const NodeRecord& node, uint64_t propertyId, ValueKind expected)
{
    const PropertyRecord* property = graph.findProperty(node, propertyId);

    if (!property)
        throw std::runtime_error("missing property");

    if (static_cast<ValueKind>(property->valueType) != expected)
        throw std::runtime_error("property type mismatch");

    switch (expected)
    {
    case ValueKind::Bool:
        return {expected, graph.readPropertyValue<uint8_t>(*property)};

    case ValueKind::Int8:
        return {expected, static_cast<uint64_t>(graph.readPropertyValue<int8_t>(*property))};

    case ValueKind::Int16:
        return {expected, static_cast<uint64_t>(graph.readPropertyValue<int16_t>(*property))};

    case ValueKind::Int32:
        return {expected, static_cast<uint64_t>(graph.readPropertyValue<int32_t>(*property))};

    case ValueKind::Int64:
        return {expected, static_cast<uint64_t>(graph.readPropertyValue<int64_t>(*property))};

    case ValueKind::UInt8:
        return {expected, graph.readPropertyValue<uint8_t>(*property)};

    case ValueKind::UInt16:
        return {expected, graph.readPropertyValue<uint16_t>(*property)};

    case ValueKind::UInt32:
        return {expected, graph.readPropertyValue<uint32_t>(*property)};

    case ValueKind::UInt64:
        return {expected, graph.readPropertyValue<uint64_t>(*property)};

    case ValueKind::Float32:
    {
        auto f = graph.readPropertyValue<float>(*property);
        uint32_t bits = 0;
        std::memcpy(&bits, &f, sizeof(float));
        return {expected, bits};
    }

    case ValueKind::Float64:
    {
        auto d = graph.readPropertyValue<double>(*property);
        uint64_t bits = 0;
        std::memcpy(&bits, &d, sizeof(double));
        return {expected, bits};
    }

    case ValueKind::StringId:
        return {expected, graph.readPropertyValue<uint64_t>(*property)};

    case ValueKind::ObjectHandle:
        return {expected, graph.readPropertyValue<uint64_t>(*property)};

    case ValueKind::Vec2:
    {
        if (property->valueSize != sizeof(Vec2))
            throw std::runtime_error("Vec2 size mismatch");

        const auto v = graph.readPropertyValue<Vec2>(*property);
        const uint32_t offset = program.addConstData(&v, sizeof(Vec2));
        return {expected, offset};
    }

    case ValueKind::Vec3:
    {
        if (property->valueSize != sizeof(Vec3))
            throw std::runtime_error("Vec3 size mismatch");

        const auto v = graph.readPropertyValue<Vec3>(*property);
        const uint32_t offset = program.addConstData(&v, sizeof(Vec3));
        return {expected, offset};
    }

    case ValueKind::Vec4:
    {
        if (property->valueSize != sizeof(Vec4))
            throw std::runtime_error("Vec4 size mismatch");

        const auto v = graph.readPropertyValue<Vec4>(*property);
        const uint32_t offset = program.addConstData(&v, sizeof(Vec4));
        return {expected, offset};
    }

    case ValueKind::Blob:
    {
        const uint8_t* src = graph.propertyValues + property->valueOffset;
        const uint32_t offset = program.addConstData(src, property->valueSize);

        return {expected, packBlobHandle(offset, property->valueSize)};
    }

    default:
        throw std::runtime_error("unsupported property kind for register value");
    }
}

void Compiler::compileFromEventBegin()
{
    constexpr uint64_t EventBeginId = hash("EventBegin");
    constexpr uint64_t ExecPinId = hash("exec");
    constexpr uint64_t ThenPinId = hash("then");
    constexpr uint64_t Branch = hash("Branch");

    const NodeRecord* current = nullptr;
    compileStates.assign(graph.header->nodeCount, CompileState::Unvisited);

    for (uint32_t i = 0; i < graph.header->nodeCount; ++i)
    {
        if (graph.nodes[i].typeId == EventBeginId)
        {
            current = &graph.nodes[i];
            break;
        }
    }

    if (!current)
        throw std::runtime_error("missing EventBegin node");

    const LinkRecord* next = graph.findExecLink(current->id, ExecPinId);

    while (next)
    {
        current = graph.findNode(next->toNode);

        if (!current)
            throw std::runtime_error("broken exec link");

        compileNodeWithDependencies(*current);

        if (current->typeId == Branch)
            break;

        next = graph.findExecLink(current->id, ThenPinId);

        if (!next)
            next = graph.findExecLink(current->id, ExecPinId);
    }

    emit({OpCode::Return, 0, 0, 0});
}

void Compiler::compileNodeWithDependencies(const NodeRecord& node)
{
    CompileState& state = stateForNode(node.id);

    if (state == CompileState::Compiled)
        return;

    if (state == CompileState::Visiting)
        throw std::runtime_error("cycle in data dependencies");

    state = CompileState::Visiting;

    const PinRecord* firstPin = graph.pins + node.firstPin;

    for (uint32_t i = 0; i < node.pinCount; ++i)
    {
        const PinRecord& pin = firstPin[i];

        if (pin.direction != static_cast<uint8_t>(PinDirection::Input))
            continue;

        if (pin.valueTypeId == hash("exec"))
            continue;

        const LinkRecord* link = graph.findInputLink(node.id, pin.pinId);
        if (!link)
            continue;

        const NodeRecord* source = graph.findNode(link->fromNode);
        if (!source)
            throw std::runtime_error("broken data dependency");

        compileNodeWithDependencies(*source);
    }

    compileNode(node);
    state = CompileState::Compiled;
}

CompileState& Compiler::stateForNode(uint32_t nodeId)
{
    for (uint32_t i = 0; i < graph.header->nodeCount; ++i)
    {
        if (graph.nodes[i].id == nodeId)
            return compileStates[i];
    }

    throw std::runtime_error("node state not found");
}

void Compiler::compileNode(const NodeRecord& node)
{
    NodeCompileFn fn = nodeCompilers.resolve(node.typeId);
    return fn(*this, node);
}
} // namespace kettle