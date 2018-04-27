#pragma once

#include "System.hpp"
#include "VertexLayout.hpp"

namespace EngineCore
{
    // TODO: stencil, input layout, blending
    class RendererPipelineState : public RendererFrontendData
    {
    protected:
        RendererPipelineState(const shared_ptr<class Shader> &shader);
        RendererPipelineState(const RendererPipelineState &) = default;
        RendererPipelineState &operator = (const RendererPipelineState &) = default;
        RendererPipelineState(RendererPipelineState &&) = delete;
        RendererPipelineState &operator = (RendererPipelineState &&) = delete;

    public:
        static shared_ptr<RendererPipelineState> New(const shared_ptr<class Shader> &shader = nullptr);
        //static shared_ptr<RendererPipelineState> New(const RendererPipelineState *pipelineToCopy);

        enum class PolygonFillModet : ui8 { Solid, Wireframe };
        enum class PolygonCullModet : ui8 { Back, Front, None };
        enum class DepthComparisonFunct : ui8 { Never, Less, LessEqual, Equal, NotEqual, GreaterEqual, Greater, Always };
        enum class PolygonTypet : ui8 { Triangle, Point };
        enum class BlendFactort : ui8 { Zero, One, SourceColor, InvertSourceColor, SourceAlpha, InvertSourceAlpha, TargetColor, InvertTargetColor, TargetAlpha, InvertTargetAlpha };
        enum class BlendCombineModet : ui8 { Add, Subtract, ReverseSubtract, Min, Max };

        struct BlendSettingst
        {
            bool isEnabled = false;
            BlendFactort sourceColorFactor = BlendFactort::SourceAlpha;
            BlendFactort sourceAlphaFactor = BlendFactort::SourceAlpha;
            BlendFactort targetColorFactor = BlendFactort::InvertSourceAlpha;
            BlendFactort targetAlphaFactor = BlendFactort::InvertSourceAlpha;
            BlendCombineModet colorCombineMode = BlendCombineModet::Add;
            BlendCombineModet alphaCombineMode = BlendCombineModet::Add;
        };

        const shared_ptr<class Shader> &Shader() const;
        void Shader(const shared_ptr<class Shader> &shader);
        PolygonFillModet PolygonFillMode() const;
        void PolygonFillMode(PolygonFillModet mode);
        PolygonCullModet PolygonCullMode() const;
        void PolygonCullMode(PolygonCullModet mode);
        PolygonTypet PolygonType() const;
        void PolygonType(PolygonTypet type);
        bool PolygonFrontIsCounterClockwise() const;
        void PolygonFrontIsCounterClockwise(bool mode);
        DepthComparisonFunct DepthComparisonFunc() const;
        void DepthComparisonFunc(DepthComparisonFunct func);
        bool EnableDepthWrite() const;
        void EnableDepthWrite(bool enable);
        void BlendSettings(const BlendSettingst &settings, ui32 renderTargetIndex = 0);
        BlendSettingst BlendSettings(ui32 renderTargetIndex = 0) const;
        const VertexLayout &VertexDataLayout() const;
        void VertexDataLayout(const VertexLayout &layout);

    private:
        shared_ptr<class Shader> _shader{};
        PolygonFillModet _polygonFillMode = PolygonFillModet::Solid;
        PolygonCullModet _polygonCullMode = PolygonCullModet::Back;
        PolygonTypet _polygonType = PolygonTypet::Triangle;
        DepthComparisonFunct _depthComparisonFunc = DepthComparisonFunct::LessEqual;
        bool _polygonFrontIsCounterClockwise = false;
        bool _enableDepthWrite = true;
        array<BlendSettingst, RenderTargetsLimit> _blendSettings{};
        VertexLayout _vertexDataLayout{};
    };
}