#include "PreHeader.hpp"
#include "CubesInstanced.hpp"
#include <Application.hpp>
#include <Logger.hpp>
#include <RendererArray.hpp>
#include <Material.hpp>
#include <VertexLayout.hpp>
#include <RendererPipelineState.hpp>
#include <Renderer.hpp>

using namespace EngineCore;
using namespace TradingApp;

CubesInstanced::CubesInstanced(ui32 maxInstances)
{
    auto shader = Application::LoadResource<Shader>("Colored3DVerticesInstanced");
    if (shader == nullptr)
    {
        SENDLOG(Error, "Cube failed to load shader Colored3DVerticesInstanced\n");
        return;
    }

    auto material = Material::New(shader);

    struct Vertex
    {
        Vector3 position;
        Vector4 color;
    };

    static constexpr f32 transparency = 0.5f;

    Vector4 frontColor{1, 1, 1, transparency};
    Vector4 upColor{0, 1, 1, transparency};
    Vector4 backColor{0, 0, 1, transparency};
    Vector4 downColor{1, 0, 0, transparency};
    Vector4 leftColor{1, 1, 0, transparency};
    Vector4 rightColor{1, 0, 1, transparency};

    RendererArrayData<Vertex> vertexArrayData
    {
        // front
        {{-0.5f, -0.5f, -0.5f}, frontColor},
    {{-0.5f, 0.5f, -0.5f}, frontColor},
    {{0.5f, -0.5f, -0.5f}, frontColor},
    {{0.5f, 0.5f, -0.5f}, frontColor},

    // up
    {{-0.5f, 0.5f, -0.5f}, upColor},
    {{-0.5f, 0.5f, 0.5f}, upColor},
    {{0.5f, 0.5f, -0.5f}, upColor},
    {{0.5f, 0.5f, 0.5f}, upColor},

    // back
    {{0.5f, -0.5f, 0.5f}, backColor},
    {{0.5f, 0.5f, 0.5f}, backColor},
    {{-0.5f, -0.5f, 0.5f}, backColor},
    {{-0.5f, 0.5f, 0.5f}, backColor},

    // down
    {{-0.5f, -0.5f, 0.5f}, downColor},
    {{-0.5f, -0.5f, -0.5f}, downColor},
    {{0.5f, -0.5f, 0.5f}, downColor},
    {{0.5f, -0.5f, -0.5f}, downColor},

    // left
    {{-0.5f, -0.5f, 0.5f}, leftColor},
    {{-0.5f, 0.5f, 0.5f}, leftColor},
    {{-0.5f, -0.5f, -0.5f}, leftColor},
    {{-0.5f, 0.5f, -0.5f}, leftColor},

    // right
    {{0.5f, -0.5f, -0.5f}, rightColor},
    {{0.5f, 0.5f, -0.5f}, rightColor},
    {{0.5f, -0.5f, 0.5f}, rightColor},
    {{0.5f, 0.5f, 0.5f}, rightColor},
    };

    auto vertexArray = RendererVertexArray::New(move(vertexArrayData));

    auto indexes = unique_ptr<ui8[], function<void(void *)>>(new ui8[36], [](void *p) {delete[] p; });
    for (ui32 index = 0; index < 6; ++index)
    {
        indexes[index * 6 + 0] = index * 4 + 0;
        indexes[index * 6 + 1] = index * 4 + 1;
        indexes[index * 6 + 2] = index * 4 + 3;

        indexes[index * 6 + 3] = index * 4 + 2;
        indexes[index * 6 + 4] = index * 4 + 0;
        indexes[index * 6 + 5] = index * 4 + 3;
    }

    RendererArrayData<ui8> indexData(move(indexes), 36);

    auto indexArray = RendererIndexArray::New(move(indexData));

    VertexLayout vertexLayout
    {
        VertexLayout::Attribute{"position", VertexLayout::Attribute::Formatt::R32G32B32F, 0, {}, {}},
        VertexLayout::Attribute{"color", VertexLayout::Attribute::Formatt::R32G32B32A32F, 0, {}, {}},
        VertexLayout::Attribute{"rotation", VertexLayout::Attribute::Formatt::R32G32B32A32F, 1, {}, 1},
        VertexLayout::Attribute{"wpos_scale", VertexLayout::Attribute::Formatt::R32G32B32A32F, 1, {}, 1},
    };

    auto pipelineState = RendererPipelineState::New(shader);
    RendererPipelineState::BlendSettingst blend;
    blend.isEnabled = false;
    blend.colorCombineMode = RendererPipelineState::BlendCombineModet::Add;
    pipelineState->BlendSettings(blend);
    pipelineState->VertexDataLayout(vertexLayout);
    pipelineState->EnableDepthWrite(true);
    //pipelineState->PolygonCullMode(RendererPipelineState::PolygonCullModet::None);
    //pipelineState->PolygonFillMode(RendererPipelineState::PolygonFillModet::Wireframe);

    _material = move(material);
    _pipelineState = move(pipelineState);
    _indexArray = move(indexArray);
    _vertexArray = move(vertexArray);

    RendererDataResource::AccessMode accessMode;
    accessMode.cpuMode.writeMode = RendererDataResource::CPUAccessMode::Mode::FrequentFull;
    _vertexInstanceArray = RendererVertexArray::New(RendererArrayData<InstanceData>(BufferOwnedData{new byte[maxInstances * sizeof(InstanceData)], [](void *p) {delete[] p; }}, maxInstances), accessMode);
}

auto CubesInstanced::Lock(ui32 instancesCount) -> InstanceData *
{
    return (InstanceData *)_vertexInstanceArray->LockDataRegion(instancesCount, 0, RendererDataResource::LockMode::Write);
}

void CubesInstanced::Unlock()
{
    _vertexInstanceArray->UnlockDataRegion();
}

void CubesInstanced::Draw(const Camera *camera, ui32 instancesCount)
{
    Application::GetRenderer().BindIndexArray(_indexArray);
    Application::GetRenderer().BindVertexArray(_vertexArray, 0);
    Application::GetRenderer().BindVertexArray(_vertexInstanceArray, 1);

    Application::GetRenderer().DrawIndexedWithCamera(camera, nullptr, _pipelineState.get(), _material.get(), PrimitiveTopology::TriangleEnumeration, _indexArray->NumberOfElements(), instancesCount);
}
