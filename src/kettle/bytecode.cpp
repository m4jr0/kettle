// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#include "bytecode.h"

#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>

#include "log.h"
#include "value_utils.h"

namespace kettle
{
const char* toString(OpCode op)
{
    switch (op)
    {
    case OpCode::Nop:
        return "Nop";
    case OpCode::LoadValue:
        return "LoadValue";
    case OpCode::Move:
        return "Move";
    case OpCode::CallNative:
        return "CallNative";

    // Bool.
    case OpCode::EqualBool:
        return "EqualBool";
    case OpCode::NotEqualBool:
        return "NotEqualBool";

    // Signed integers.
    case OpCode::LessInt8:
        return "LessInt8";
    case OpCode::LessEqualInt8:
        return "LessEqualInt8";
    case OpCode::EqualInt8:
        return "EqualInt8";
    case OpCode::NotEqualInt8:
        return "NotEqualInt8";
    case OpCode::GreaterEqualInt8:
        return "GreaterEqualInt8";
    case OpCode::GreaterInt8:
        return "GreaterInt8";

    case OpCode::LessInt16:
        return "LessInt16";
    case OpCode::LessEqualInt16:
        return "LessEqualInt16";
    case OpCode::EqualInt16:
        return "EqualInt16";
    case OpCode::NotEqualInt16:
        return "NotEqualInt16";
    case OpCode::GreaterEqualInt16:
        return "GreaterEqualInt16";
    case OpCode::GreaterInt16:
        return "GreaterInt16";

    case OpCode::LessInt32:
        return "LessInt32";
    case OpCode::LessEqualInt32:
        return "LessEqualInt32";
    case OpCode::EqualInt32:
        return "EqualInt32";
    case OpCode::NotEqualInt32:
        return "NotEqualInt32";
    case OpCode::GreaterEqualInt32:
        return "GreaterEqualInt32";
    case OpCode::GreaterInt32:
        return "GreaterInt32";

    case OpCode::LessInt64:
        return "LessInt64";
    case OpCode::LessEqualInt64:
        return "LessEqualInt64";
    case OpCode::EqualInt64:
        return "EqualInt64";
    case OpCode::NotEqualInt64:
        return "NotEqualInt64";
    case OpCode::GreaterEqualInt64:
        return "GreaterEqualInt64";
    case OpCode::GreaterInt64:
        return "GreaterInt64";

    // Unsigned integers.
    case OpCode::LessUInt8:
        return "LessUInt8";
    case OpCode::LessEqualUInt8:
        return "LessEqualUInt8";
    case OpCode::EqualUInt8:
        return "EqualUInt8";
    case OpCode::NotEqualUInt8:
        return "NotEqualUInt8";
    case OpCode::GreaterEqualUInt8:
        return "GreaterEqualUInt8";
    case OpCode::GreaterUInt8:
        return "GreaterUInt8";

    case OpCode::LessUInt16:
        return "LessUInt16";
    case OpCode::LessEqualUInt16:
        return "LessEqualUInt16";
    case OpCode::EqualUInt16:
        return "EqualUInt16";
    case OpCode::NotEqualUInt16:
        return "NotEqualUInt16";
    case OpCode::GreaterEqualUInt16:
        return "GreaterEqualUInt16";
    case OpCode::GreaterUInt16:
        return "GreaterUInt16";

    case OpCode::LessUInt32:
        return "LessUInt32";
    case OpCode::LessEqualUInt32:
        return "LessEqualUInt32";
    case OpCode::EqualUInt32:
        return "EqualUInt32";
    case OpCode::NotEqualUInt32:
        return "NotEqualUInt32";
    case OpCode::GreaterEqualUInt32:
        return "GreaterEqualUInt32";
    case OpCode::GreaterUInt32:
        return "GreaterUInt32";

    case OpCode::LessUInt64:
        return "LessUInt64";
    case OpCode::LessEqualUInt64:
        return "LessEqualUInt64";
    case OpCode::EqualUInt64:
        return "EqualUInt64";
    case OpCode::NotEqualUInt64:
        return "NotEqualUInt64";
    case OpCode::GreaterEqualUInt64:
        return "GreaterEqualUInt64";
    case OpCode::GreaterUInt64:
        return "GreaterUInt64";

    // Floating point.
    case OpCode::LessFloat32:
        return "LessFloat32";
    case OpCode::LessEqualFloat32:
        return "LessEqualFloat32";
    case OpCode::EqualFloat32:
        return "EqualFloat32";
    case OpCode::NotEqualFloat32:
        return "NotEqualFloat32";
    case OpCode::GreaterEqualFloat32:
        return "GreaterEqualFloat32";
    case OpCode::GreaterFloat32:
        return "GreaterFloat32";

    case OpCode::LessFloat64:
        return "LessFloat64";
    case OpCode::LessEqualFloat64:
        return "LessEqualFloat64";
    case OpCode::EqualFloat64:
        return "EqualFloat64";
    case OpCode::NotEqualFloat64:
        return "NotEqualFloat64";
    case OpCode::GreaterEqualFloat64:
        return "GreaterEqualFloat64";
    case OpCode::GreaterFloat64:
        return "GreaterFloat64";

    // IDs / handles.
    case OpCode::EqualStringId:
        return "EqualStringId";
    case OpCode::NotEqualStringId:
        return "NotEqualStringId";

    case OpCode::EqualObjectHandle:
        return "EqualObjectHandle";
    case OpCode::NotEqualObjectHandle:
        return "NotEqualObjectHandle";

    // Vectors.
    case OpCode::EqualVec2:
        return "EqualVec2";
    case OpCode::NotEqualVec2:
        return "NotEqualVec2";

    case OpCode::EqualVec3:
        return "EqualVec3";
    case OpCode::NotEqualVec3:
        return "NotEqualVec3";

    case OpCode::EqualVec4:
        return "EqualVec4";
    case OpCode::NotEqualVec4:
        return "NotEqualVec4";

    // Control flow.
    case OpCode::Jump:
        return "Jump";
    case OpCode::JumpIfFalse:
        return "JumpIfFalse";
    case OpCode::JumpIfTrue:
        return "JumpIfTrue";
    case OpCode::Return:
        return "Return";
    }

    return "Unknown";
}

template<typename T, typename CompareFn>
static void compareInline(VM& vm, const Instruction& ins, CompareFn compare)
{
    const auto a = readInlineValue<T>(vm.registers[ins.b]);
    const auto b = readInlineValue<T>(vm.registers[static_cast<uint8_t>(ins.c)]);

    vm.registers[ins.a] = Value{ValueKind::Bool, compare(a, b) ? 1ull : 0ull};
}

template<typename CompareFn>
static void comparePayload(VM& vm, const Instruction& ins, CompareFn compare)
{
    const uint64_t a = vm.registers[ins.b].payload;
    const uint64_t b = vm.registers[static_cast<uint8_t>(ins.c)].payload;

    vm.registers[ins.a] = Value{ValueKind::Bool, compare(a, b) ? 1ull : 0ull};
}

template<typename T, typename CompareFn>
    requires std::is_trivially_copyable_v<T>
static void compareConst(VM& vm, const Program& program, const Instruction& ins, CompareFn compare)
{
    const uint64_t payloadA = vm.registers[ins.b].payload;
    const uint64_t payloadB = vm.registers[static_cast<uint8_t>(ins.c)].payload;

    const auto a = program.readConstValue<T>(payloadA);
    const auto b = program.readConstValue<T>(payloadB);

    vm.registers[ins.a] = Value{ValueKind::Bool, compare(a, b) ? 1ull : 0ull};
}

uint16_t NativeRegistry::add(uint64_t id, NativeFn fn)
{
    if (entries.size() >= MaxEntryCount)
        throw std::runtime_error("too many native entries");

    entries.push_back({id, fn});
    return static_cast<uint16_t>(entries.size() - 1);
}

uint16_t NativeRegistry::resolve(uint64_t id) const
{
    for (uint16_t i = 0; i < entries.size(); ++i)
    {
        if (entries[i].id == id)
            return i;
    }

    throw std::runtime_error("native function not found");
}

const NativeEntry& NativeRegistry::get(uint16_t index) const
{
    if (index >= entries.size())
        throw std::runtime_error("native index out of range");

    return entries[index];
}


uint32_t Program::addConstData(const void* data, uint32_t size)
{
    const auto offset = static_cast<uint32_t>(constData.size());
    const auto* bytes = static_cast<const std::byte*>(data);

    constData.insert(constData.end(), bytes, bytes + size);

    return offset;
}

void VM::execute(const Program& program)
{
    if (!natives)
        throw std::runtime_error("VM missing native registry");

    size_t pc = 0;

    for (;;)
    {
        const auto& ins = program.code[pc++];

        switch (ins.op)
        {
#define KETTLE_CMP(OP, TYPE, EXPR) \
    case OpCode::OP: \
        compareInline<TYPE>( \
            *this, \
            ins, \
            [](TYPE a, TYPE b) \
            { \
                return EXPR; \
            } \
        ); \
        break

#define KETTLE_CMP_PAYLOAD(OP, EXPR) \
    case OpCode::OP: \
        comparePayload( \
            *this, \
            ins, \
            [](uint64_t a, uint64_t b) \
            { \
                return EXPR; \
            } \
        ); \
        break;



#define KETTLE_CMP_CONST(OP, TYPE, EXPR) \
    case OpCode::OP: \
        compareConst<TYPE>( \
            *this, \
            program, \
            ins, \
            [](const TYPE& a, const TYPE& b) \
            { \
                return EXPR; \
            } \
        ); \
        break

            // Bool.
            KETTLE_CMP(EqualBool, bool, a == b);
            KETTLE_CMP(NotEqualBool, bool, a != b);

            // Signed integers.
            KETTLE_CMP(LessInt8, int8_t, a < b);
            KETTLE_CMP(LessEqualInt8, int8_t, a <= b);
            KETTLE_CMP(EqualInt8, int8_t, a == b);
            KETTLE_CMP(NotEqualInt8, int8_t, a != b);
            KETTLE_CMP(GreaterEqualInt8, int8_t, a >= b);
            KETTLE_CMP(GreaterInt8, int8_t, a > b);

            KETTLE_CMP(LessInt16, int16_t, a < b);
            KETTLE_CMP(LessEqualInt16, int16_t, a <= b);
            KETTLE_CMP(EqualInt16, int16_t, a == b);
            KETTLE_CMP(NotEqualInt16, int16_t, a != b);
            KETTLE_CMP(GreaterEqualInt16, int16_t, a >= b);
            KETTLE_CMP(GreaterInt16, int16_t, a > b);

            KETTLE_CMP(LessInt32, int32_t, a < b);
            KETTLE_CMP(LessEqualInt32, int32_t, a <= b);
            KETTLE_CMP(EqualInt32, int32_t, a == b);
            KETTLE_CMP(NotEqualInt32, int32_t, a != b);
            KETTLE_CMP(GreaterEqualInt32, int32_t, a >= b);
            KETTLE_CMP(GreaterInt32, int32_t, a > b);

            KETTLE_CMP(LessInt64, int64_t, a < b);
            KETTLE_CMP(LessEqualInt64, int64_t, a <= b);
            KETTLE_CMP(EqualInt64, int64_t, a == b);
            KETTLE_CMP(NotEqualInt64, int64_t, a != b);
            KETTLE_CMP(GreaterEqualInt64, int64_t, a >= b);
            KETTLE_CMP(GreaterInt64, int64_t, a > b);

            // Unsigned integers.
            KETTLE_CMP(LessUInt8, uint8_t, a < b);
            KETTLE_CMP(LessEqualUInt8, uint8_t, a <= b);
            KETTLE_CMP(EqualUInt8, uint8_t, a == b);
            KETTLE_CMP(NotEqualUInt8, uint8_t, a != b);
            KETTLE_CMP(GreaterEqualUInt8, uint8_t, a >= b);
            KETTLE_CMP(GreaterUInt8, uint8_t, a > b);

            KETTLE_CMP(LessUInt16, uint16_t, a < b);
            KETTLE_CMP(LessEqualUInt16, uint16_t, a <= b);
            KETTLE_CMP(EqualUInt16, uint16_t, a == b);
            KETTLE_CMP(NotEqualUInt16, uint16_t, a != b);
            KETTLE_CMP(GreaterEqualUInt16, uint16_t, a >= b);
            KETTLE_CMP(GreaterUInt16, uint16_t, a > b);

            KETTLE_CMP(LessUInt32, uint32_t, a < b);
            KETTLE_CMP(LessEqualUInt32, uint32_t, a <= b);
            KETTLE_CMP(EqualUInt32, uint32_t, a == b);
            KETTLE_CMP(NotEqualUInt32, uint32_t, a != b);
            KETTLE_CMP(GreaterEqualUInt32, uint32_t, a >= b);
            KETTLE_CMP(GreaterUInt32, uint32_t, a > b);

            KETTLE_CMP(LessUInt64, uint64_t, a < b);
            KETTLE_CMP(LessEqualUInt64, uint64_t, a <= b);
            KETTLE_CMP(EqualUInt64, uint64_t, a == b);
            KETTLE_CMP(NotEqualUInt64, uint64_t, a != b);
            KETTLE_CMP(GreaterEqualUInt64, uint64_t, a >= b);
            KETTLE_CMP(GreaterUInt64, uint64_t, a > b);

            // Floats.
            // For now, an error margin is applied by default.
            KETTLE_CMP(LessFloat32, float, a < b);
            KETTLE_CMP(LessEqualFloat32, float, lessOrAlmostEqual(a, b));
            KETTLE_CMP(EqualFloat32, float, almostEqual(a, b));
            KETTLE_CMP(NotEqualFloat32, float, !almostEqual(a, b));
            KETTLE_CMP(GreaterEqualFloat32, float, greaterOrAlmostEqual(a, b));
            KETTLE_CMP(GreaterFloat32, float, a > b);

            KETTLE_CMP(LessFloat64, double, a < b);
            KETTLE_CMP(LessEqualFloat64, double, lessOrAlmostEqual(a, b));
            KETTLE_CMP(EqualFloat64, double, almostEqual(a, b));
            KETTLE_CMP(NotEqualFloat64, double, !almostEqual(a, b));
            KETTLE_CMP(GreaterEqualFloat64, double, greaterOrAlmostEqual(a, b));
            KETTLE_CMP(GreaterFloat64, double, a > b);

            // IDs / handles.
            KETTLE_CMP_PAYLOAD(EqualStringId, a == b);
            KETTLE_CMP_PAYLOAD(NotEqualStringId, a != b);

            KETTLE_CMP_PAYLOAD(EqualObjectHandle, a == b);
            KETTLE_CMP_PAYLOAD(NotEqualObjectHandle, a != b);

            // Vectors.
            // For now, an error margin is applied by default.
            KETTLE_CMP_CONST(EqualVec2, Vec2, almostEqual(a, b));
            KETTLE_CMP_CONST(NotEqualVec2, Vec2, !almostEqual(a, b));

            KETTLE_CMP_CONST(EqualVec3, Vec3, almostEqual(a, b));
            KETTLE_CMP_CONST(NotEqualVec3, Vec3, !almostEqual(a, b));

            KETTLE_CMP_CONST(EqualVec4, Vec4, almostEqual(a, b));
            KETTLE_CMP_CONST(NotEqualVec4, Vec4, !almostEqual(a, b));

        case OpCode::LoadValue:
            registers[ins.a] = Value{.kind = static_cast<ValueKind>(ins.b), .payload = ins.c};
            break;

        case OpCode::Move:
            registers[ins.a] = registers[ins.b];
            break;

        case OpCode::CallNative:
        {
            const auto nativeIndex = static_cast<uint16_t>(ins.c);
            const auto& entry = natives->get(nativeIndex);

            Value result{};
            entry.fn(registers, ins.a, ins.b, &result);

            if (result.kind != ValueKind::None)
                registers[ins.a] = result;

            break;
        }

        case OpCode::Jump:
            pc = static_cast<size_t>(ins.c);
            break;

        case OpCode::JumpIfFalse:
            if (registers[ins.a].kind != ValueKind::Bool || registers[ins.a].payload == 0)
                pc = static_cast<size_t>(ins.c);
            break;

        case OpCode::JumpIfTrue:
            if (registers[ins.a].kind == ValueKind::Bool && registers[ins.a].payload != 0)
                pc = static_cast<size_t>(ins.c);
            break;

        case OpCode::Return:
            return;

        default:
            throw std::runtime_error("unknown opcode");
        }

#undef KETTLE_CMP
#undef KETTLE_CMP_PAYLOAD
#undef KETTLE_CMP_CONST
    }
}

std::string formatProgram(const Program& program)
{
    std::ostringstream oss;

    oss << "maxRegisterCount=" << int(program.maxRegisterCount) << "\n";

    for (size_t pc = 0; pc < program.code.size(); ++pc)
    {
        const Instruction& ins = program.code[pc];

        oss << std::setw(4) << std::setfill('0') << pc << "  ";
        oss << toString(ins.op);

        switch (ins.op)
        {
        case OpCode::LoadValue:
            oss << " dst=R" << int(ins.a) << " kind=" << toString(static_cast<ValueKind>(ins.b)) << " payload=" << ins.c;
            break;

        case OpCode::Move:
            oss << " dst=R" << int(ins.a) << " src=R" << int(ins.b);
            break;

        case OpCode::CallNative:
            oss << " firstArg=R" << int(ins.a) << " argCount=" << ins.b << " nativeIndex=" << ins.c;
            break;

        case OpCode::Jump:
            oss << " target=" << ins.c;
            break;

        case OpCode::JumpIfFalse:
        case OpCode::JumpIfTrue:
            oss << " cond=R" << int(ins.a) << " target=" << ins.c;
            break;

        case OpCode::Return:
        case OpCode::Nop:
            break;

        default:
        {
            oss << " dst=R" << int(ins.a) << " lhs=R" << int(ins.b) << " rhs=R" << int(static_cast<uint8_t>(ins.c));
            break;
        }
        }

        oss << '\n';
    }

    return oss.str();
}

void dumpProgram(const Program& program)
{
#if KETTLE_DUMP_BYTECODE
    KETTLE_LOG_INFO("\n", formatProgram(program));
#else
    (void)program;
#endif
}
} // namespace kettle