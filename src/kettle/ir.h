// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "bytecode.h"

namespace kettle
{
using IrBlockId = uint32_t;
static inline constexpr IrBlockId kInvalidIrBlockId = static_cast<IrBlockId>(-1);

using IrValueId = uint32_t;
static inline constexpr IrValueId kInvalidIrValueId = static_cast<IrValueId>(-1);

enum class IrOp : uint8_t
{
    Nop,

    LoadValue,
    Move,
    CallNative,

    Compare,

    Jump,
    JumpIfFalse,
    JumpIfTrue,

    Return,
};

struct IrInstruction
{
    IrOp op = IrOp::Nop;
    OpCode bytecodeOp = OpCode::Nop;

    IrValueId dst = kInvalidIrValueId;
    IrValueId src0 = kInvalidIrValueId;
    IrValueId src1 = kInvalidIrValueId;
    IrValueId result = kInvalidIrValueId;

    Value value{};

    uint16_t nativeIndex = 0;
    IrBlockId targetBlock = kInvalidIrBlockId;

    std::vector<IrValueId> args;
};

struct IrBlock
{
    IrBlockId id = kInvalidIrBlockId;
    std::vector<IrInstruction> instructions;
    std::vector<IrBlockId> successors;
};

struct IrProgram
{
    std::vector<IrBlock> blocks;
    IrBlockId entryBlock = kInvalidIrBlockId;
};

struct IrBuilder
{
    IrProgram program;
    IrBlockId currentBlock = kInvalidIrBlockId;

    IrBlockId createBlock();

    IrBlock& block(IrBlockId id);
    const IrBlock& block(IrBlockId id) const;

    void setCurrentBlock(IrBlockId id);

    void emit(IrInstruction instruction);
    void addSuccessor(IrBlockId from, IrBlockId to);
};

const char* toString(IrOp op);

void optimizeIr(IrProgram& ir);
void removeUnreachableBlocks(IrProgram& ir);

Program lowerIrToBytecode(const IrProgram& ir, std::vector<std::byte> constData = {});

std::string formatIrProgram(const IrProgram& ir);
void dumpIrProgram(const IrProgram& ir);
} // namespace kettle