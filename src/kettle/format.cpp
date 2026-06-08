// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#include "format.h"

namespace kettle
{
const char* toString(ValueKind kind)
{
    switch (kind)
    {
    case ValueKind::None:
        return "None";
    case ValueKind::Bool:
        return "Bool";
    case ValueKind::Int8:
        return "Int8";
    case ValueKind::Int16:
        return "Int16";
    case ValueKind::Int32:
        return "Int32";
    case ValueKind::Int64:
        return "Int64";
    case ValueKind::UInt8:
        return "UInt8";
    case ValueKind::UInt16:
        return "UInt16";
    case ValueKind::UInt32:
        return "UInt32";
    case ValueKind::UInt64:
        return "UInt64";
    case ValueKind::Float32:
        return "Float32";
    case ValueKind::Float64:
        return "Float64";
    case ValueKind::StringId:
        return "StringId";
    case ValueKind::ObjectHandle:
        return "ObjectHandle";
    case ValueKind::Vec2:
        return "Vec2";
    case ValueKind::Vec3:
        return "Vec3";
    case ValueKind::Vec4:
        return "Vec4";
    case ValueKind::Blob:
        return "Blob";
    }

    return "Unknown";
}
} // namespace kettle