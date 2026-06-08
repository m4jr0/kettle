// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#pragma once

#include <optional>
#include <vector>

#include "ir.h"

namespace kettle
{
bool isValidIrValue(IrValueId value);

void collectIrUses(const IrInstruction& instruction, std::vector<IrValueId>& out);
std::optional<IrValueId> getIrDef(const IrInstruction& instruction);

bool isIrTerminator(IrOp op);
bool irInstructionHasSideEffects(const IrInstruction& instruction);
} // namespace kettle