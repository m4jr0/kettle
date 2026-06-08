// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#pragma once

#include <optional>
#include <unordered_map>
#include <vector>

#include "bytecode.h"
#include "graph_view.h"
#include "hash.h"
#include "ir.h"

namespace kettle
{
enum class CompileState : uint8_t
{
    Unvisited,
    Visiting,
    Compiled,
};

struct ProducedValue
{
    uint32_t nodeId;
    uint64_t pinId;
    IrValueId value;
};

struct ResolvedInput
{
    uint32_t sourceNodeId;
    uint64_t sourcePinId;
    IrValueId value;
};

struct ValueOperand
{
    IrValueId value = kInvalidIrValueId;
    std::optional<ResolvedInput> source;
    bool temporary = false;
};

struct Compiler;

using NodeCompileFn = void (*)(Compiler&, const NodeRecord&);

enum class DependencyPolicy : uint8_t
{
    Auto,
    Manual,
};

enum class FlowPolicy : uint8_t
{
    Linear,
    OwnsContinuation,
};

struct NodeCompilerEntry
{
    NodeCompileFn fn = nullptr;
    DependencyPolicy dependencyPolicy = DependencyPolicy::Auto;
    FlowPolicy flowPolicy = FlowPolicy::Linear;
};

struct NodeCompilerRegistry
{
    std::unordered_map<uint64_t, NodeCompilerEntry> entries;

    void add(
        uint64_t nodeTypeId,
        NodeCompileFn fn,
        DependencyPolicy dependencyPolicy = DependencyPolicy::Auto,
        FlowPolicy flowPolicy = FlowPolicy::Linear
    );

    const NodeCompilerEntry& resolve(uint64_t nodeTypeId) const;
};

struct Compiler
{
    const GraphView& graph;
    const NativeRegistry& natives;
    const NodeCompilerRegistry& nodeCompilers;

    IrBuilder ir;
    Program program;
    IrValueId nextValue = 0;

    std::vector<ProducedValue> produced;
    std::vector<CompileState> compileStates;

    IrValueId createValue();

    void emit(IrInstruction ins);
    void emitMove(IrValueId dst, IrValueId src);
    void emitJump(IrBlockId target);
    void emitJumpIfFalse(IrValueId conditionValue, IrBlockId target);

    void rememberValue(uint32_t nodeId, uint64_t pinId, IrValueId value);

    ProducedValue* findProducedValue(uint32_t nodeId, uint64_t pinId);
    const ProducedValue* findProducedValue(uint32_t nodeId, uint64_t pinId) const;

    ResolvedInput resolveInput(uint32_t nodeId, uint64_t inputPinId) const;
    std::optional<ResolvedInput> tryResolveInput(uint32_t nodeId, uint64_t inputPinId) const;

    ValueOperand resolveOrLoadValueOperand(const NodeRecord& node, uint64_t pinId, uint64_t propertyId, ValueKind expectedKind);
    ValueOperand resolveOrLoadIntoOperand(IrValueId dstValue, const NodeRecord& node, uint64_t pinId, uint64_t propertyId, ValueKind expectedKind);

    Value materializePropertyAsValue(const NodeRecord& node, uint64_t propertyId, ValueKind expected);

    void compileFromEventBegin();
    void compileNode(const NodeRecord& node);
    void compileNodeWithDependencies(const NodeRecord& node);
    void compileInputDependency(const NodeRecord& node, uint64_t inputPinId);

    CompileState& stateForNode(uint32_t nodeId);
};

void registerBuiltinNodeCompilers(NodeCompilerRegistry& registry);
void registerBuiltinNatives(NativeRegistry& registry);
void setNativeContext(const GraphView* graph, const Program* program);
} // namespace kettle