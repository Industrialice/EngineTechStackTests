#include "BasicHeader.hpp"
#include "VertexLayout.hpp"
#include <map>

using namespace EngineCore;

VertexLayout::Attribute::Attribute(string_view name, Formatt format, ui8 vertexArrayNumber, optional<ui32> vertexArrayOffsetInBytes, optional<ui32> instanceStep) : _name(name), _format(format), _varrayNumber(vertexArrayNumber), _varrayOffsetInBytes(vertexArrayOffsetInBytes), _instanceStep(instanceStep)
{
    assert(instanceStep == nullopt || *instanceStep != 0);
}

string_view VertexLayout::Attribute::Name() const
{
    return _name;
}

auto VertexLayout::Attribute::Format() const -> Formatt
{
    return _format;
}

ui8 VertexLayout::Attribute::VertexArrayNumber() const
{
    return _varrayNumber;
}

optional<ui32> VertexLayout::Attribute::VertexArrayOffset() const
{
    return _varrayOffsetInBytes;
}

optional<ui32> VertexLayout::Attribute::InstanceStep() const
{
    return _instanceStep;
}

ui32 VertexLayout::Attribute::SizeInBytes() const
{
    switch (_format)
    {
    case VertexLayout::Attribute::Formatt::R32F:
        return sizeof(f32);
    case VertexLayout::Attribute::Formatt::R32G32F:
        return sizeof(f32) * 2;
    case VertexLayout::Attribute::Formatt::R32G32B32F:
        return sizeof(f32) * 3;
    case VertexLayout::Attribute::Formatt::R32G32B32A32F:
        return sizeof(f32) * 4;
    }

    UNREACHABLE;
    return 0;
}

// TODO: resolve optional offsets
VertexLayout::VertexLayout(const Attribute *attributes, uiw attributesCount) : _attributes(attributes, attributes + attributesCount)
{
    assert(attributes != nullptr || attributesCount == 0);
    std::map<ui32 /* buffer number */, ui32 /* current offset */> offsets;
    for (uiw index = 0; index < attributesCount; ++index)
    {
        const auto &attribute = attributes[index];
        if (attribute.VertexArrayOffset().has_value() == false)
        {
            auto &currentOffset = offsets[attribute.VertexArrayNumber()];
            _attributes[index]._varrayOffsetInBytes = currentOffset;
            currentOffset += attribute.SizeInBytes();
        }
        else
        {
            offsets[attribute.VertexArrayNumber()] = *attribute.VertexArrayOffset() + attribute.SizeInBytes();
        }
    }
}

VertexLayout::VertexLayout(initializer_list<Attribute> initializer) : VertexLayout(initializer.begin(), initializer.size())
{}

auto VertexLayout::Attributes() const -> const vector<Attribute> &
{
    return _attributes;
}
