// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#include "ir.h"

#include <cstdint>
#include <stdexcept>
#include <unordered_map>

#include "ir_analysis.h"
#include "ir_utils.h"

namespace kettle
{
struct RegisterAllocator
{
    uint8_t nextRegister = 0;
    std::vector<uint8_t> freeRegisters;
    std::unordered_map<IrValueId, uint8_t> valueToRegister;

    uint8_t allocateAny()
    {
        if (!freeRegisters.empty())
        {
            const uint8_t reg = freeRegisters.back();
            freeRegisters.pop_back();
            return reg;
        }

        if (nextRegister > VM::MaxRegister)
            throw std::runtime_error("too many VM registers");

        return nextRegister++;
    }

    uint8_t define(IrValueId value)
    {
        if (!isValidIrValue(value))
            throw std::runtime_error("invalid IR destination value");

        if (valueToRegister.contains(value))
            throw std::runtime_error("IR value defined twice");

        const uint8_t reg = allocateAny();
        valueToRegister[value] = reg;
        return reg;
    }

    void defineAtRegister(IrValueId value, uint8_t reg)
    {
        if (!isValidIrValue(value))
            throw std::runtime_error("invalid IR destination value");

        if (valueToRegister.contains(value))
            throw std::runtime_error("IR value defined twice");

        valueToRegister[value] = reg;
    }

    uint8_t use(IrValueId value) const
    {
        auto it = valueToRegister.find(value);

        if (it == valueToRegister.cend())
            throw std::runtime_error("use of undefined IR value");

        return it->second;
    }

    void release(IrValueId value)
    {
        auto it = valueToRegister.find(value);

        if (it == valueToRegister.end())
            return;

        freeRegisters.push_back(it->second);
        valueToRegister.erase(it);
    }
};

static uint8_t allocateContiguousScratch(RegisterAllocator& registers, uint16_t count)
{
    if (count == 0)
        throw std::runtime_error("cannot allocate zero scratch registers");

    if (static_cast<uint16_t>(registers.nextRegister) + count > VM::MaxRegister + 1)
        throw std::runtime_error("too many VM registers for call arguments");

    const uint8_t first = registers.nextRegister;
    registers.nextRegister = static_cast<uint8_t>(registers.nextRegister + count);
    return first;
}

static void releaseDeadValuesAfterInstruction(RegisterAllocator& registers, const IrInstruction& instruction, const IrValueSet& liveAfter)
{
    std::vector<IrValueId> uses;
    collectIrUses(instruction, uses);

    for (IrValueId value : uses)
    {
        // If the instruction just used v, and v is not live after this instruction,
        // then this was v's final use. Free its register.
        if (!liveAfter.contains(value))
            registers.release(value);
    }

    if (auto def = getIrDef(instruction))
    {
        // If v is not in liveAfter, then the result is never used. Release v immediately.
        if (!liveAfter.contains(*def))
            registers.release(*def);
    }
}

static void lowerInstruction(
    Program& program,
    const IrInstruction& instruction,
    const std::unordered_map<IrBlockId, size_t>& blockToPc,
    RegisterAllocator& registers
)
{
    switch (instruction.op)
    {
    case IrOp::Nop:
        program.code.push_back({.op = OpCode::Nop});
        return;

    case IrOp::LoadValue:
    {
        const uint8_t dst = registers.define(instruction.dst);

        program.code.push_back({
            .op = OpCode::LoadValue,
            .a = dst,
            .b = static_cast<uint16_t>(instruction.value.kind),
            .c = instruction.value.payload,
        });

        return;
    }

    case IrOp::Move:
    {
        const uint8_t dst = registers.define(instruction.dst);
        const uint8_t src = registers.use(instruction.src0);

        program.code.push_back({
            .op = OpCode::Move,
            .a = dst,
            .b = src,
            .c = 0,
        });

        return;
    }

    case IrOp::Compare:
    {
        const uint8_t dst = registers.define(instruction.dst);
        const uint8_t lhs = registers.use(instruction.src0);
        const uint8_t rhs = registers.use(instruction.src1);

        program.code.push_back({
            .op = instruction.bytecodeOp,
            .a = dst,
            .b = lhs,
            .c = rhs,
        });

        return;
    }

    case IrOp::CallNative:
    {
        const auto argCount = static_cast<uint16_t>(instruction.args.size());

        // CallNative uses register A as a call base:
        //
        //   args live in:
        //     A, A + 1, ..., A + argCount - 1
        //
        //   result is written back to:
        //     A
        //
        // So for zero-arg calls, A is only a result register (when it applies).
        // For calls with args, A is both the first argument register and,
        // if the native returns a value, the result register.
        uint8_t callBaseRegister = 0;

        if (argCount == 0)
        {
            if (isValidIrValue(instruction.result))
                callBaseRegister = registers.define(instruction.result);
            else
                callBaseRegister = 0; // Ignored for zero-arg void calls.
        }
        else
        {
            // Native arguments must be contiguous for the VM calling convention.
            callBaseRegister = allocateContiguousScratch(registers, argCount);

            for (uint16_t i = 0; i < argCount; ++i)
            {
                const uint8_t src = registers.use(instruction.args[i]);
                const uint8_t dst = static_cast<uint8_t>(callBaseRegister + i);

                program.code.push_back({
                    .op = OpCode::Move,
                    .a = dst,
                    .b = src,
                    .c = 0,
                });
            }

            // If the native produces a value, it will be stored in the call base
            // register after the call returns.
            if (isValidIrValue(instruction.result))
                registers.defineAtRegister(instruction.result, callBaseRegister);
        }

        program.code.push_back({
            .op = OpCode::CallNative,
            .a = callBaseRegister,
            .b = argCount,
            .c = instruction.nativeIndex,
        });

        return;
    }

    case IrOp::Jump:
    {
        const auto it = blockToPc.find(instruction.targetBlock);
        if (it == blockToPc.cend())
            throw std::runtime_error("invalid IR jump target");

        program.code.push_back({
            .op = OpCode::Jump,
            .a = 0,
            .b = 0,
            .c = static_cast<uint64_t>(it->second),
        });

        return;
    }

    case IrOp::JumpIfFalse:
    {
        const auto it = blockToPc.find(instruction.targetBlock);
        if (it == blockToPc.cend())
            throw std::runtime_error("invalid IR conditional jump target");

        const uint8_t cond = registers.use(instruction.src0);

        program.code.push_back({
            .op = OpCode::JumpIfFalse,
            .a = cond,
            .b = 0,
            .c = static_cast<uint64_t>(it->second),
        });

        return;
    }

    case IrOp::JumpIfTrue:
    {
        const auto it = blockToPc.find(instruction.targetBlock);
        if (it == blockToPc.cend())
            throw std::runtime_error("invalid IR conditional jump target");

        const uint8_t cond = registers.use(instruction.src0);

        program.code.push_back({
            .op = OpCode::JumpIfTrue,
            .a = cond,
            .b = 0,
            .c = static_cast<uint64_t>(it->second),
        });

        return;
    }

    case IrOp::Return:
        program.code.push_back({.op = OpCode::Return});
        return;
    }

    throw std::runtime_error("unknown IR op");
}

static size_t estimateLoweredInstructionCount(const IrInstruction& instruction)
{
    if (instruction.op == IrOp::CallNative)
        return 1 + instruction.args.size();

    return 1;
}

Program lowerIrToBytecode(const IrProgram& ir, std::vector<std::byte> constData)
{
    if (ir.entryBlock == kInvalidIrBlockId)
        throw std::runtime_error("IR program has no entry block");

    Program program;
    program.constData = std::move(constData);

    std::unordered_map<IrBlockId, size_t> blockToPc;

    size_t pc = 0;

    for (const IrBlock& block : ir.blocks)
    {
        blockToPc[block.id] = pc;

        for (const IrInstruction& ins : block.instructions)
            pc += estimateLoweredInstructionCount(ins);
    }

    // Calculate what survives between blocks.
    const std::vector<BlockLiveness> liveness = computeCfgLiveness(ir);

    RegisterAllocator registers;

    for (const IrBlock& block : ir.blocks)
    {
        for (size_t i = 0; i < block.instructions.size(); ++i)
        {
            const IrInstruction& instruction = block.instructions[i];

            lowerInstruction(program, instruction, blockToPc, registers);

            // Rewind from the block exit to find what survives after
            // one exact instruction.
            const IrValueSet liveAfter = computeLiveAfterInstruction(block, i, liveness);

            releaseDeadValuesAfterInstruction(registers, instruction, liveAfter);
        }
    }

    program.maxRegisterCount = registers.nextRegister;
    return program;
}
} // namespace kettle