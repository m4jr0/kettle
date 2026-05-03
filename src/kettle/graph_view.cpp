// Copyright 2026 m4jr0. All Rights Reserved.
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#include "graph_view.h"

#include <cstring>

namespace kettle
{
GraphView GraphView::load(const char* path)
{
    GraphView view;

    std::ifstream file(path, std::ios::binary | std::ios::ate);

    if (!file)
        throw std::runtime_error("failed to open file");

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    view.storage.resize(static_cast<size_t>(size));

    if (!file.read(reinterpret_cast<char*>(view.storage.data()), size))
        throw std::runtime_error("failed to read file");

    view.bind();
    return view;
}

void GraphView::bind()
{
    if (storage.size() < sizeof(Header))
        throw std::runtime_error("file too small");

    header = reinterpret_cast<const Header*>(storage.data());

    if (header->magic != KettleMagic)
        throw std::runtime_error("invalid magic");

    if (header->version != KettleVersion)
        throw std::runtime_error("unsupported version");

    if (header->endian != KettleEndian)
        throw std::runtime_error("unsupported endian");

    checkRange(header->stringTableOffset, header->stringTableSize);
    checkRange(header->nodesOffset, header->nodeCount * sizeof(NodeRecord));
    checkRange(header->pinsOffset, header->pinCount * sizeof(PinRecord));
    checkRange(header->linksOffset, header->linkCount * sizeof(LinkRecord));
    checkRange(header->propertiesOffset, header->propertyCount * sizeof(PropertyRecord));
    checkRange(header->propertyValuesOffset, header->propertyValuesSize);

    stringTable = storage.data() + header->stringTableOffset;
    nodes = reinterpret_cast<const NodeRecord*>(storage.data() + header->nodesOffset);
    pins = reinterpret_cast<const PinRecord*>(storage.data() + header->pinsOffset);
    links = reinterpret_cast<const LinkRecord*>(storage.data() + header->linksOffset);
    properties = reinterpret_cast<const PropertyRecord*>(storage.data() + header->propertiesOffset);
    propertyValues = storage.data() + header->propertyValuesOffset;
}

const NodeRecord* GraphView::findNode(uint32_t nodeId) const
{
    for (uint32_t i = 0; i < header->nodeCount; ++i)
    {
        if (nodes[i].id == nodeId)
            return &nodes[i];
    }

    return nullptr;
}

const PropertyRecord* GraphView::findProperty(const NodeRecord& node, uint64_t propertyId) const
{
    const auto* first = properties + node.firstProperty;

    for (uint32_t i = 0; i < node.propertyCount; ++i)
    {
        if (first[i].propertyId == propertyId)
            return &first[i];
    }

    return nullptr;
}

const LinkRecord* GraphView::findExecLink(uint32_t fromNode, uint64_t fromPinId) const
{
    for (uint32_t i = 0; i < header->linkCount; ++i)
    {
        if (links[i].fromNode == fromNode && links[i].fromPinId == fromPinId)
            return &links[i];
    }

    return nullptr;
}

const LinkRecord* GraphView::findInputLink(uint32_t toNode, uint64_t toPinId) const
{
    for (uint32_t i = 0; i < header->linkCount; ++i)
    {
        if (links[i].toNode == toNode && links[i].toPinId == toPinId)
            return &links[i];
    }

    return nullptr;
}

std::string_view GraphView::findDebugString(uint64_t stringId) const
{
    uint32_t offset = 0;

    while (offset < header->stringTableSize)
    {
        if (offset + 14 > header->stringTableSize)
            break;

        uint64_t id = 0;
        uint32_t textOffset = 0;
        uint16_t textLength = 0;

        std::memcpy(&id, stringTable + offset, sizeof(uint64_t));
        std::memcpy(&textOffset, stringTable + offset + 8, sizeof(uint32_t));
        std::memcpy(&textLength, stringTable + offset + 12, sizeof(uint16_t));

        if (textOffset + textLength > header->stringTableSize)
            break;

        if (id == stringId)
            return std::string_view(reinterpret_cast<const char*>(stringTable + textOffset), textLength);

        offset = textOffset + textLength + 1;
    }

    return {};
}

std::span<const uint8_t> GraphView::readBlobValue(const Value& value) const
{
    if (value.kind != ValueKind::Blob)
        throw std::runtime_error("expected blob");

    BlobHandle handle = unpackBlobHandle(value.payload);

    if (static_cast<size_t>(handle.offset) + handle.size > header->propertyValuesSize)
        throw std::runtime_error("blob out of bounds");

    return {propertyValues + handle.offset, handle.size};
}

void GraphView::checkRange(uint32_t offset, uint32_t size) const
{
    const size_t end = static_cast<size_t>(offset) + static_cast<size_t>(size);

    if (end > storage.size())
        throw std::runtime_error("section out of range");
}
} // namespace kettle