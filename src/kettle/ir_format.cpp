// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#include "ir.h"

#include <iomanip>
#include <sstream>

#include "ir_utils.h"
#include "log.h"

namespace kettle
{
static void formatIrInstruction(std::ostream& os, const IrInstruction& instruction)
{
    os << std::left << std::setw(14) << toString(instruction.op) << std::right;

    switch (instruction.op)
    {
    case IrOp::LoadValue:
        os << "dst=v" << instruction.dst << " kind=" << toString(instruction.value.kind) << " payload=" << instruction.value.payload;
        break;

    case IrOp::Move:
        os << "dst=v" << instruction.dst << " src=v" << instruction.src0;
        break;

    case IrOp::CallNative:
        if (isValidIrValue(instruction.result))
            os << "result=v" << instruction.result << " ";
        else
            os << "result=<none> ";

        os << "argCount=" << instruction.args.size() << " nativeIndex=" << instruction.nativeIndex;
        break;

    case IrOp::Compare:
        os << toString(instruction.bytecodeOp) << " dst=v" << instruction.dst << " lhs=v" << instruction.src0 << " rhs=v" << instruction.src1;
        break;

    case IrOp::Jump:
        os << "targetBlock=B" << instruction.targetBlock;
        break;

    case IrOp::JumpIfFalse:
    case IrOp::JumpIfTrue:
        os << "cond=v" << instruction.src0 << " targetBlock=B" << instruction.targetBlock;
        break;

    case IrOp::Return:
    case IrOp::Nop:
        break;
    }

    os << "\n";
}

std::string formatIrProgram(const IrProgram& ir)
{
    std::ostringstream oss;

    oss << "IR blocks: " << ir.blocks.size() << "\n";
    oss << "Entry: B" << ir.entryBlock << "\n";

    for (const IrBlock& block : ir.blocks)
    {
        oss << "\n";
        oss << "B" << block.id << ":\n";

        if (!block.successors.empty())
        {
            oss << "  successors:";

            for (IrBlockId successor : block.successors)
                oss << " B" << successor;

            oss << "\n";
        }

        for (size_t i = 0; i < block.instructions.size(); ++i)
        {
            oss << "  " << std::setw(3) << i << "  ";
            formatIrInstruction(oss, block.instructions[i]);
        }
    }

    return oss.str();
}

void dumpIrProgram(const IrProgram& ir)
{
    KETTLE_LOG_INFO("\n", formatIrProgram(ir));
}
} // namespace kettle