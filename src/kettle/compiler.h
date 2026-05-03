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

namespace kettle
{
enum class CompileState : uint8_t
{
    Unvisited,
    Visiting,
    Compiled,
};

struct Compiler;

using NodeCompileFn = void (*)(Compiler&, const NodeRecord&);

struct NodeCompilerRegistry
{
    std::unordered_map<uint64_t, NodeCompileFn> entries;

    void add(uint64_t nodeTypeId, NodeCompileFn fn);
    NodeCompileFn resolve(uint64_t nodeTypeId) const;
};

struct Compiler
{
    const GraphView& graph;
    const NativeRegistry& natives;
    const NodeCompilerRegistry& nodeCompilers;

    Program program;
    uint8_t nextRegister = 0;

    struct ProducedValue
    {
        uint32_t nodeId;
        uint64_t pinId;
        uint8_t reg;
    };

    std::vector<ProducedValue> produced;
    std::vector<CompileState> compileStates;

    uint8_t allocRegister();
    uint8_t allocRegisters(uint8_t count);
    void emit(Instruction ins);
    void emitMove(uint8_t dst, uint8_t src);

    void rememberValue(uint32_t nodeId, uint64_t pinId, uint8_t reg);
    uint8_t resolveInputRegister(uint32_t nodeId, uint64_t inputPinId) const;
    std::optional<uint8_t> tryResolveInputRegister(uint32_t nodeId, uint64_t inputPinId) const;
    uint8_t resolveOrLoadValue(const NodeRecord& node, uint64_t pinId, uint64_t propertyId, ValueKind expectedKind);
    void resolveOrLoadInto(uint8_t dstReg, const NodeRecord& node, uint64_t pinId, uint64_t propertyId, ValueKind expectedKind);

    Value materializePropertyAsValue(const NodeRecord& node, uint64_t propertyId, ValueKind expected);

    void compileFromEventBegin();
    void compileNode(const NodeRecord& node);
    void compileNodeWithDependencies(const NodeRecord& node);

    CompileState& stateForNode(uint32_t nodeId);
};

void registerBuiltinNodeCompilers(NodeCompilerRegistry& registry);
void registerBuiltinNatives(NativeRegistry& registry);
void setNativeContext(const GraphView* graph, const Program* program);
} // namespace kettle