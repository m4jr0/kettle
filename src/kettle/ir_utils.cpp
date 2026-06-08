// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#include "ir_utils.h"

#include <algorithm>

namespace kettle
{
bool isValidIrValue(IrValueId value)
{
    return value != kInvalidIrValueId;
}

void collectIrUses(const IrInstruction& instruction, std::vector<IrValueId>& out)
{
    switch (instruction.op)
    {
    case IrOp::Move:
        if (isValidIrValue(instruction.src0))
            out.push_back(instruction.src0);
        break;

    case IrOp::Compare:
        if (isValidIrValue(instruction.src0))
            out.push_back(instruction.src0);
        if (isValidIrValue(instruction.src1))
            out.push_back(instruction.src1);
        break;

    case IrOp::JumpIfFalse:
    case IrOp::JumpIfTrue:
        if (isValidIrValue(instruction.src0))
            out.push_back(instruction.src0);
        break;

    case IrOp::CallNative:
        for (IrValueId arg : instruction.args)
        {
            if (isValidIrValue(arg))
                out.push_back(arg);
        }
        break;

    default:
        break;
    }
}

std::optional<IrValueId> getIrDef(const IrInstruction& instruction)
{
    switch (instruction.op)
    {
    case IrOp::LoadValue:
    case IrOp::Move:
    case IrOp::Compare:
        return instruction.dst;

    case IrOp::CallNative:
        if (isValidIrValue(instruction.result))
            return instruction.result;

        return std::nullopt;

    default:
        return std::nullopt;
    }
}

bool isIrTerminator(IrOp op)
{
    switch (op)
    {
    case IrOp::Jump:
    case IrOp::JumpIfFalse:
    case IrOp::JumpIfTrue:
    case IrOp::Return:
        return true;

    default:
        return false;
    }
}

bool irInstructionHasSideEffects(const IrInstruction& instruction)
{
    switch (instruction.op)
    {
    case IrOp::CallNative:
    case IrOp::Jump:
    case IrOp::JumpIfFalse:
    case IrOp::JumpIfTrue:
    case IrOp::Return:
        return true;

    default:
        return false;
    }
}
} // namespace kettle