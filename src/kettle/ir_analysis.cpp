// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#include "ir_analysis.h"

#include <stdexcept>

#include "ir_utils.h"

namespace kettle
{
std::vector<BlockLiveness> computeCfgLiveness(const IrProgram& ir)
{
    std::vector<BlockLiveness> liveness(ir.blocks.size());

    for (const IrBlock& block : ir.blocks)
    {
        BlockLiveness& live = liveness[block.id];

        for (const IrInstruction& ins : block.instructions)
        {
            std::vector<IrValueId> uses;
            collectIrUses(ins, uses);

            // A value belongs to USE(B) only if it is read before being
            // defined inside this block.
            //
            // Example:
            //
            //     v1 = LoadValue
            //     v2 = Compare(v0, v1)
            //
            // USE(B) = { v0 }
            // DEF(B) = { v1, v2 }
            //
            // v1 does not belong to USE(B) because the block defines it
            // before reading it.
            for (IrValueId value : uses)
            {
                if (!live.def.contains(value))
                    live.use.insert(value);
            }

            // Record values defined by this block.
            if (auto def = getIrDef(ins))
                live.def.insert(*def);
        }
    }

    // Solve CFG liveness until a fixed point is reached.
    //
    // Liveness is a backwards "may" analysis:
    //   * LiveOut(B) = union(LiveIn(successors))
    //   * LiveIn(B)  = Use(B) U (LiveOut(B) - Def(B))
    //     * A value must be alive before entering a block if
    //       the block reads it before creating it, or if some
    //       later block needs it and this block does not recreate it.
    //
    // Loops introduce cycles in the control-flow graph, so a single pass
    // is insufficient. Information propagates through the CFG until no
    // block changes anymore.

    bool changed = true;

    while (changed)
    {
        changed = false;

        // Walk blocks backwards.
        // Since liveness is a backward analysis, processing successors first
        // typically reaches convergence in fewer iterations.
        for (auto blockIt = ir.blocks.rbegin(); blockIt != ir.blocks.rend(); ++blockIt)
        {
            const IrBlock& block = *blockIt;
            BlockLiveness& live = liveness[block.id];

            IrValueSet newLiveOut;

            // Step 1: calculate all LiveOut(B), which are U(LiveIn(successors)).
            for (IrBlockId successor : block.successors)
            {
                if (successor >= liveness.size())
                    throw std::runtime_error("IR successor out of range during liveness");

                const IrValueSet& successorLiveIn = liveness[successor].liveIn;
                newLiveOut.insert(successorLiveIn.begin(), successorLiveIn.end());
            }

            // Step 2: calculate all LiveIn(B), which are Use(B) U (LiveOut(B) - Def(B)).

            // Any value read before being defined in this block must
            // already be alive when entering the block.
            IrValueSet newLiveIn = live.use; // Use(B) part.

            // Add LiveOut(B) - Def(B).
            // Values needed by successor blocks must also be alive
            // on entry, unless this block redefines them itself.
            for (IrValueId value : newLiveOut)
            {
                if (!live.def.contains(value)) // -Def(B) part.
                    newLiveIn.insert(value);   // LiveOut(B) part.
            }

            if (newLiveIn != live.liveIn || newLiveOut != live.liveOut)
            {
                live.liveIn = std::move(newLiveIn);
                live.liveOut = std::move(newLiveOut);
                changed = true;
            }
        }
    }

    return liveness;
}

IrValueSet computeLiveAfterInstruction(const IrBlock& block, size_t instructionIndex, const std::vector<BlockLiveness>& liveness)
{
    IrValueSet live = liveness[block.id].liveOut;

    // Walk backward from the last instruction down to the instruction after the one we care about.
    for (size_t i = block.instructions.size(); i-- > instructionIndex + 1;)
    {
        const IrInstruction& ins = block.instructions[i];

        // If the instruction defines a value, then before that instruction, that value did not need to exist yet.
        if (auto def = getIrDef(ins))
            live.erase(*def);

        std::vector<IrValueId> uses;
        collectIrUses(ins, uses);

        // If the instruction uses a value, then before that instruction, the value must be alive.
        for (IrValueId value : uses)
            live.insert(value);
    }

    return live;
}
} // namespace kettle