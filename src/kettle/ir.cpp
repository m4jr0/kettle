// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#include "ir.h"

#include <stdexcept>

namespace kettle
{
IrBlockId IrBuilder::createBlock()
{
    const IrBlockId id = static_cast<IrBlockId>(program.blocks.size());

    program.blocks.push_back(
        IrBlock{
            .id = id,
        }
    );

    if (program.entryBlock == kInvalidIrBlockId)
        program.entryBlock = id;

    if (currentBlock == kInvalidIrBlockId)
        currentBlock = id;

    return id;
}

IrBlock& IrBuilder::block(IrBlockId id)
{
    if (id >= program.blocks.size())
        throw std::runtime_error("IR block out of range");

    return program.blocks[id];
}

const IrBlock& IrBuilder::block(IrBlockId id) const
{
    if (id >= program.blocks.size())
        throw std::runtime_error("IR block out of range");

    return program.blocks[id];
}

void IrBuilder::setCurrentBlock(IrBlockId id)
{
    if (id >= program.blocks.size())
        throw std::runtime_error("IR block out of range");

    currentBlock = id;
}

void IrBuilder::emit(IrInstruction instruction)
{
    if (currentBlock == kInvalidIrBlockId)
        throw std::runtime_error("cannot emit without current IR block");

    block(currentBlock).instructions.push_back(std::move(instruction));
}

void IrBuilder::addSuccessor(IrBlockId from, IrBlockId to)
{
    if (from >= program.blocks.size() || to >= program.blocks.size())
        throw std::runtime_error("IR successor block out of range");

    program.blocks[from].successors.push_back(to);
}

const char* toString(IrOp op)
{
    switch (op)
    {
    case IrOp::Nop:
        return "Nop";
    case IrOp::LoadValue:
        return "LoadValue";
    case IrOp::Move:
        return "Move";
    case IrOp::CallNative:
        return "CallNative";
    case IrOp::Compare:
        return "Compare";
    case IrOp::Jump:
        return "Jump";
    case IrOp::JumpIfFalse:
        return "JumpIfFalse";
    case IrOp::JumpIfTrue:
        return "JumpIfTrue";
    case IrOp::Return:
        return "Return";
    }

    return "Unknown";
}
} // namespace kettle