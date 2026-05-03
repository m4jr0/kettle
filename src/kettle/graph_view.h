// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#pragma once

#include "format.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string_view>
#include <span>
#include <type_traits>
#include <vector>

namespace kettle
{
struct GraphView
{
    std::vector<uint8_t> storage;

    const Header* header = nullptr;
    const uint8_t* stringTable = nullptr;
    const NodeRecord* nodes = nullptr;
    const PinRecord* pins = nullptr;
    const LinkRecord* links = nullptr;
    const PropertyRecord* properties = nullptr;
    const uint8_t* propertyValues = nullptr;

    static GraphView load(const char* path);

    void bind();

    const NodeRecord* findNode(uint32_t nodeId) const;
    const PropertyRecord* findProperty(const NodeRecord& node, uint64_t propertyId) const;
    const LinkRecord* findExecLink(uint32_t fromNode, uint64_t fromPinId) const;
    const LinkRecord* findInputLink(uint32_t toNode, uint64_t toPinId) const;

    std::string_view findDebugString(uint64_t stringId) const;

    template<typename T>
        requires std::is_trivially_copyable_v<T>
    T readPropertyValue(const PropertyRecord& property) const
    {
        if (property.valueSize != sizeof(T))
            throw std::runtime_error("property size mismatch");

        if (static_cast<size_t>(property.valueOffset) + property.valueSize > header->propertyValuesSize)
            throw std::runtime_error("property read out of bounds");

        T value{};
        std::memcpy(&value, propertyValues + property.valueOffset, sizeof(T));
        return value;
    }

    template<typename T>
        requires std::is_trivially_copyable_v<T>
    T readValueObject(const Value& value) const
    {
        if (value.payload + sizeof(T) > header->propertyValuesSize)
            throw std::runtime_error("value object out of bounds");

        T out{};
        std::memcpy(&out, propertyValues + value.payload, sizeof(T));
        return out;
    }

    std::span<const uint8_t> readBlobValue(const Value& value) const;

private:
    void checkRange(uint32_t offset, uint32_t size) const;
};
} // namespace kettle