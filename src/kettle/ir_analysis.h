// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#pragma once

#include <unordered_set>
#include <vector>

#include "ir.h"

namespace kettle
{
using IrValueSet = std::unordered_set<IrValueId>;

struct BlockLiveness
{
    IrValueSet use;
    IrValueSet def;
    IrValueSet liveIn;
    IrValueSet liveOut;
};

std::vector<BlockLiveness> computeCfgLiveness(const IrProgram& ir);
IrValueSet computeLiveAfterInstruction(const IrBlock& block, size_t instructionIndex, const std::vector<BlockLiveness>& liveness);
} // namespace kettle