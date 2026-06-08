// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#include "compiler.h"

#include <cstring>
#include <stdexcept>

#include "hash.h"

namespace kettle
{
void NodeCompilerRegistry::add(uint64_t nodeTypeId, NodeCompileFn fn, DependencyPolicy dependencyPolicy, FlowPolicy flowPolicy)
{
    entries[nodeTypeId] = NodeCompilerEntry{
        .fn = fn,
        .dependencyPolicy = dependencyPolicy,
        .flowPolicy = flowPolicy,
    };
}

const NodeCompilerEntry& NodeCompilerRegistry::resolve(uint64_t nodeTypeId) const
{
    auto it = entries.find(nodeTypeId);

    if (it == entries.cend())
        throw std::runtime_error("missing node compiler");

    return it->second;
}

IrValueId Compiler::createValue()
{
    return nextValue++;
}

void Compiler::emit(IrInstruction ins)
{
    ir.emit(ins);
}

void Compiler::emitMove(IrValueId dst, IrValueId src)
{
    emit({
        .op = IrOp::Move,
        .dst = dst,
        .src0 = src,
    });
}

void Compiler::emitJump(IrBlockId target)
{
    emit({
        .op = IrOp::Jump,
        .targetBlock = target,
    });

    ir.addSuccessor(ir.currentBlock, target);
}

void Compiler::emitJumpIfFalse(IrValueId conditionValue, IrBlockId target)
{
    emit({
        .op = IrOp::JumpIfFalse,
        .src0 = conditionValue,
        .targetBlock = target,
    });

    ir.addSuccessor(ir.currentBlock, target);
}

void Compiler::rememberValue(uint32_t nodeId, uint64_t pinId, IrValueId value)
{
    produced.push_back({
        .nodeId = nodeId,
        .pinId = pinId,
        .value = value,
    });
}

ValueOperand Compiler::resolveOrLoadValueOperand(const NodeRecord& node, uint64_t pinId, uint64_t propertyId, ValueKind expectedKind)
{
    if (auto input = tryResolveInput(node.id, pinId))
    {
        return {
            .value = input->value,
            .source = input,
            .temporary = false,
        };
    }

    const Value value = materializePropertyAsValue(node, propertyId, expectedKind);
    const IrValueId valueId = createValue();

    emit({
        .op = IrOp::LoadValue,
        .dst = valueId,
        .value = value,
    });

    return {
        .value = valueId,
        .source = std::nullopt,
        .temporary = true,
    };
}

ValueOperand Compiler::resolveOrLoadIntoOperand(
    IrValueId dstValue,
    const NodeRecord& node,
    uint64_t pinId,
    uint64_t propertyId,
    ValueKind expectedKind
)
{
    if (auto input = tryResolveInput(node.id, pinId))
    {
        emitMove(dstValue, input->value);

        return {
            .value = dstValue,
            .source = input,
            .temporary = false,
        };
    }

    const Value value = materializePropertyAsValue(node, propertyId, expectedKind);

    emit({
        .op = IrOp::LoadValue,
        .dst = dstValue,
        .value = value,
    });

    return {
        .value = dstValue,
        .source = std::nullopt,
        .temporary = false,
    };
}

ProducedValue* Compiler::findProducedValue(uint32_t nodeId, uint64_t pinId)
{
    for (ProducedValue& value : produced)
    {
        if (value.nodeId == nodeId && value.pinId == pinId)
            return &value;
    }

    return nullptr;
}

const ProducedValue* Compiler::findProducedValue(uint32_t nodeId, uint64_t pinId) const
{
    for (const ProducedValue& value : produced)
    {
        if (value.nodeId == nodeId && value.pinId == pinId)
            return &value;
    }

    return nullptr;
}

ResolvedInput Compiler::resolveInput(uint32_t nodeId, uint64_t inputPinId) const
{
    auto input = tryResolveInput(nodeId, inputPinId);
    if (!input)
        throw std::runtime_error("missing input link");

    return *input;
}

std::optional<ResolvedInput> Compiler::tryResolveInput(uint32_t nodeId, uint64_t inputPinId) const
{
    const LinkRecord* link = graph.findInputLink(nodeId, inputPinId);
    if (!link)
        return std::nullopt;

    const ProducedValue* value = findProducedValue(link->fromNode, link->fromPinId);

    if (!value)
        throw std::runtime_error("input source has not been compiled yet");

    return ResolvedInput{
        .sourceNodeId = link->fromNode,
        .sourcePinId = link->fromPinId,
        .value = value->value,
    };
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
    ir = IrBuilder{};
    ir.createBlock();
    ir.setCurrentBlock(ir.program.entryBlock);

    constexpr uint64_t EventBeginId = hash("EventBegin");
    constexpr uint64_t ExecPinId = hash("exec");
    constexpr uint64_t ThenPinId = hash("then");

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

        const NodeCompilerEntry& entry = nodeCompilers.resolve(current->typeId);

        if (entry.flowPolicy == FlowPolicy::OwnsContinuation)
            break;

        next = graph.findExecLink(current->id, ThenPinId);

        if (!next)
            next = graph.findExecLink(current->id, ExecPinId);
    }

    emit({
        .op = IrOp::Return,
    });

    optimizeIr(ir.program);
    program = lowerIrToBytecode(ir.program, std::move(program.constData));
}

void Compiler::compileNode(const NodeRecord& node)
{
    const NodeCompilerEntry& entry = nodeCompilers.resolve(node.typeId);
    entry.fn(*this, node);
}

void Compiler::compileNodeWithDependencies(const NodeRecord& node)
{
    CompileState& state = stateForNode(node.id);

    if (state == CompileState::Compiled)
        return;

    if (state == CompileState::Visiting)
        throw std::runtime_error("cycle in data dependencies");

    state = CompileState::Visiting;

    const NodeCompilerEntry& entry = nodeCompilers.resolve(node.typeId);

    if (entry.dependencyPolicy == DependencyPolicy::Auto)
    {
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
    }

    entry.fn(*this, node);
    state = CompileState::Compiled;
}

void Compiler::compileInputDependency(const NodeRecord& node, uint64_t inputPinId)
{
    const LinkRecord* link = graph.findInputLink(node.id, inputPinId);
    if (!link)
        throw std::runtime_error("missing input link");

    const NodeRecord* source = graph.findNode(link->fromNode);
    if (!source)
        throw std::runtime_error("broken data dependency");

    compileNodeWithDependencies(*source);
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
} // namespace kettle