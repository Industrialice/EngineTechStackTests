#include "BasicHeader.hpp"
#include "RendererPipelineState.hpp"
#include "Application.hpp"
#include "Logger.hpp"

using namespace EngineCore;

// TODO: check that the old value isn't equal to the new value

RendererPipelineState::RendererPipelineState(const shared_ptr<class Shader> &shader) : _shader(shader)
{}

shared_ptr<RendererPipelineState> RendererPipelineState::New(const shared_ptr<class Shader> &shader)
{
    struct Proxy : public RendererPipelineState
    {
        Proxy(const shared_ptr<class Shader> &shader) : RendererPipelineState(shader) {}
    };
    return make_shared<Proxy>(shader);
}

/*shared_ptr<RendererPipelineState> RendererPipelineState::New(const RendererPipelineState *pipelineToCopy)
{
    if (pipelineToCopy == nullptr)
    {
        return nullptr;
    }
    struct Proxy : public RendererPipelineState
    {
        Proxy(const RendererPipelineState &pipelineToCopy) : RendererPipelineState(pipelineToCopy) {}
    };
    return make_shared<Proxy>(*pipelineToCopy);
}*/

auto EngineCore::RendererPipelineState::Shader() const -> const shared_ptr<class Shader> &
{
    return _shader;
}

void RendererPipelineState::Shader(const shared_ptr<class Shader> &shader)
{
    _shader = shader;
    BackendDataMayBeDirty();
}

auto RendererPipelineState::PolygonFillMode() const -> PolygonFillModet
{
    return _polygonFillMode;
}

void RendererPipelineState::PolygonFillMode(PolygonFillModet mode)
{
    _polygonFillMode = mode;
    BackendDataMayBeDirty();
}

auto RendererPipelineState::PolygonCullMode() const -> PolygonCullModet
{
    return _polygonCullMode;
}

void RendererPipelineState::PolygonCullMode(PolygonCullModet mode)
{
    _polygonCullMode = mode;
    BackendDataMayBeDirty();
}

auto RendererPipelineState::PolygonType() const -> PolygonTypet
{
    return _polygonType;
}

void RendererPipelineState::PolygonType(PolygonTypet type)
{
    _polygonType = type;
    BackendDataMayBeDirty();
}

bool RendererPipelineState::PolygonFrontIsCounterClockwise() const
{
    return _polygonFrontIsCounterClockwise;
}

void RendererPipelineState::PolygonFrontIsCounterClockwise(bool mode)
{
    _polygonFrontIsCounterClockwise = mode;
    BackendDataMayBeDirty();
}

auto RendererPipelineState::DepthComparisonFunc() const -> DepthComparisonFunct
{
    return _depthComparisonFunc;
}

void RendererPipelineState::DepthComparisonFunc(DepthComparisonFunct func)
{
    _depthComparisonFunc = func;
    BackendDataMayBeDirty();
}

bool RendererPipelineState::EnableDepthWrite() const
{
    return _enableDepthWrite;
}

void RendererPipelineState::EnableDepthWrite(bool enable)
{
    _enableDepthWrite = enable;
    BackendDataMayBeDirty();
}

void RendererPipelineState::BlendSettings(const BlendSettingst &settings, ui32 renderTargetIndex)
{
    if (renderTargetIndex >= RenderTargetsLimit)
    {
        SENDLOG(Error, "BlendSettings trying to access render target %u, but only %u render target are allowed\n", renderTargetIndex, RenderTargetsLimit);
        return;
    }
    _blendSettings[renderTargetIndex] = settings;

    auto checkAlphaFactor = [](auto &alphaFactor)
    {
        if (alphaFactor == BlendFactort::SourceColor)
        {
            SENDLOG(Error, "alpha factor cannot be SourceColor, replacing with SourceAlpha");
            alphaFactor = BlendFactort::SourceAlpha;
        }
        else if (alphaFactor == BlendFactort::InvertSourceColor)
        {
            SENDLOG(Error, "alpha factor cannot be InvertSourceColor, replacing with InvertSourceAlpha");
            alphaFactor = BlendFactort::InvertSourceAlpha;
        }
        else if (alphaFactor == BlendFactort::TargetColor)
        {
            SENDLOG(Error, "alpha factor cannot be TargetColor, replacing with TargetAlpha");
            alphaFactor = BlendFactort::TargetAlpha;
        }
        else if (alphaFactor == BlendFactort::InvertTargetColor)
        {
            SENDLOG(Error, "alpha factor cannot be InvertTargetColor, replacing with InvertTargetAlpha");
            alphaFactor = BlendFactort::InvertTargetAlpha;
        }
    };

    checkAlphaFactor(_blendSettings[renderTargetIndex].sourceAlphaFactor);
    checkAlphaFactor(_blendSettings[renderTargetIndex].targetAlphaFactor);

    BackendDataMayBeDirty();
}

auto RendererPipelineState::BlendSettings(ui32 renderTargetIndex) const -> BlendSettingst
{
    if (renderTargetIndex >= RenderTargetsLimit)
    {
        SENDLOG(Error, "IsBlendingEnabled trying to access render target %u, but only %u render target are allowed\n", renderTargetIndex, RenderTargetsLimit);
        return {};
    }
    return _blendSettings[renderTargetIndex];
}

auto RendererPipelineState::VertexDataLayout() const -> const VertexLayout &
{
    return _vertexDataLayout;
}

void RendererPipelineState::VertexDataLayout(const VertexLayout &layout)
{
    _vertexDataLayout = layout;
    BackendDataMayBeDirty();
}
