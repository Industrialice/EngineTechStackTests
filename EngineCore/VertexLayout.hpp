#pragma once

namespace EngineCore
{
    class VertexLayout
    {
    public:
        class Attribute
        {
            friend VertexLayout;

        public:
            enum class Formatt
            {
                R32F, R32G32F, R32G32B32F, R32G32B32A32F
            };

            Attribute(string_view name, Formatt format, ui8 vertexArrayNumber, optional<ui32> vertexArrayOffsetInBytes = nullopt, optional<ui32> instanceStep = nullopt);
            string_view Name() const;
            Formatt Format() const;
            ui8 VertexArrayNumber() const;
            optional<ui32> VertexArrayOffset() const;
            optional<ui32> InstanceStep() const;
            ui32 SizeInBytes() const;

        private:
            string _name;
            Formatt _format;
            optional<ui32> _varrayOffsetInBytes;
            optional<ui32> _instanceStep;
            ui8 _varrayNumber;
        };

        VertexLayout() = default;
        VertexLayout(const Attribute *attributes, uiw attributesCount);
        VertexLayout(initializer_list<Attribute> initializer);
        const vector<Attribute> &Attributes() const;

    private:
        vector<Attribute> _attributes{};
    };
}