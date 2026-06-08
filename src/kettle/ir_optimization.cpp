// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#include "ir.h"

#include <stdexcept>
#include <vector>

namespace kettle
{
static void markReachableBlocks(const IrProgram& ir, std::vector<bool>& reachable)
{
    if (ir.entryBlock == kInvalidIrBlockId)
        throw std::runtime_error("IR program has no entry block");

    std::vector<IrBlockId> stack;
    stack.push_back(ir.entryBlock);

    while (!stack.empty())
    {
        const IrBlockId blockId = stack.back();
        stack.pop_back();

        if (blockId >= ir.blocks.size())
            throw std::runtime_error("invalid IR block during reachability");

        if (reachable[blockId])
            continue;

        reachable[blockId] = true;

        for (IrBlockId successor : ir.blocks[blockId].successors)
            stack.push_back(successor);
    }
}

void removeUnreachableBlocks(IrProgram& ir)
{
    if (ir.entryBlock == kInvalidIrBlockId)
        throw std::runtime_error("IR program has no entry block");

    std::vector<bool> reachable(ir.blocks.size(), false);
    markReachableBlocks(ir, reachable);

    std::vector<IrBlockId> remap(ir.blocks.size(), kInvalidIrBlockId);
    std::vector<IrBlock> newBlocks;

    for (const IrBlock& oldBlock : ir.blocks)
    {
        if (!reachable[oldBlock.id])
            continue;

        const IrBlockId newId = static_cast<IrBlockId>(newBlocks.size());
        remap[oldBlock.id] = newId;

        IrBlock newBlock = oldBlock;
        newBlock.id = newId;
        newBlocks.push_back(std::move(newBlock));
    }

    for (IrBlock& block : newBlocks)
    {
        for (IrBlockId& successor : block.successors)
        {
            if (successor >= remap.size() || remap[successor] == kInvalidIrBlockId)
                throw std::runtime_error("reachable block points to unreachable successor");

            successor = remap[successor];
        }

        for (IrInstruction& instruction : block.instructions)
        {
            switch (instruction.op)
            {
            case IrOp::Jump:
            case IrOp::JumpIfFalse:
            case IrOp::JumpIfTrue:
                if (instruction.targetBlock >= remap.size() || remap[instruction.targetBlock] == kInvalidIrBlockId)
                    throw std::runtime_error("jump targets unreachable block");

                instruction.targetBlock = remap[instruction.targetBlock];
                break;

            default:
                break;
            }
        }
    }

    ir.entryBlock = remap[ir.entryBlock];
    ir.blocks = std::move(newBlocks);
}

void optimizeIr(IrProgram& ir)
{
    removeUnreachableBlocks(ir);
    // TODO(m4jr0): Add other optimization passes (dead instructions, etc.).
}
} // namespace kettle