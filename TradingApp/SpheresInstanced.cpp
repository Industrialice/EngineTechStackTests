#include "PreHeader.hpp"
#include "SpheresInstanced.hpp"
#include <Application.hpp>
#include <Logger.hpp>
#include <RendererArray.hpp>
#include <Material.hpp>
#include <VertexLayout.hpp>
#include <RendererPipelineState.hpp>
#include <Renderer.hpp>
#include <MathFunctions.hpp>

using namespace EngineCore;
using namespace TradingApp;

SpheresInstanced::SpheresInstanced(ui32 slices, ui32 layerCuts, ui32 maxInstances)
{
    assert(slices >= 3 && layerCuts >= 1);

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
        Vector3 normal;
    };

    ui32 verticesCount = (slices * layerCuts) + 2;
    assert(verticesCount < 256);
    ui32 indexesCount = ((slices + 2) * 2) + ((layerCuts - 1) * (slices + 1) * 2);

    auto vertices = unique_ptr<ui8[], function<void(void *)>>(new ui8[verticesCount * sizeof(Vertex)], [](void *p) {delete[] p; });
    auto indexes = unique_ptr<ui8[], function<void(void *)>>(new ui8[indexesCount], [](void *p) {delete[] p; });

    auto *vertexPtr = (Vertex *)vertices.get();
    
    f32 phid = MathPi<f32>() / (layerCuts + 1);
    f32 thetad = 2.0f * MathPi<f32>() / slices;
    ui32 indexPerPointLayer = slices + 2;
    ui32 indexPerLayer = (slices * 2) + 2;

    vertexPtr[0] = {{0.0f, 0.5f, 0.0f}, {0.0f, 1.0f, 1.0f}};
    vertexPtr[verticesCount - 1] = {{0.0f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}};

    f32 phi = phid;
    for (ui32 i = 0; i < layerCuts; ++i)
    {
        f32 y = cos(phi);
        f32 d = sin(phi);
        f32 theta = 0.0f;
        for (ui32 j = 0; j < slices; ++j)
        {
            f32 x = sin(theta) * d;
            f32 z = cos(theta) * d;
            vertexPtr[i * slices + j + 1] = {{x * 0.5f, y * 0.5f, z * 0.5f}, Vector3{x, y, z} + Vector3{0.25f, 0.2f, 0.6f}};
            theta += thetad;
        }
        phi += phid;
    }

    auto vertexArray = RendererVertexArray::New(RendererArrayData<Vertex>(move(vertices), verticesCount));

    // indexes - cones
    indexes[0] = 0;
    indexes[indexPerPointLayer] = verticesCount - 1;
    for (ui32 i = 0; i < slices; ++i)
    {
        indexes[i + 1] = i + 1;
        indexes[i + 1 + indexPerPointLayer] = verticesCount - i - 2;
    }
    indexes[slices + 1] = 1;
    indexes[indexPerPointLayer + slices + 1] = verticesCount - 2;

    // indexes - quads
    ui32 layerStartIndex = indexPerPointLayer * 2;
    ui32 layerStartVertex = 1;
    for (ui32 i = 0; i < (layerCuts - 1); ++i)
    {
        for (ui32 j = 0; j < slices; ++j)
        {
            indexes[layerStartIndex + (j * 2)] = layerStartVertex + j;
            indexes[layerStartIndex + ((j * 2) + 1)] = layerStartVertex + j + slices;
        }
        indexes[layerStartIndex + (slices * 2)] = layerStartVertex;
        indexes[layerStartIndex + ((slices * 2) + 1)] = layerStartVertex + slices;
        layerStartIndex += indexPerLayer;
        layerStartVertex += slices;
    }

    auto indexArray = RendererIndexArray::New(RendererArrayData<ui8>(move(indexes), indexesCount));

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
    pipelineState->PolygonCullMode(RendererPipelineState::PolygonCullModet::None);
    //pipelineState->PolygonFillMode(RendererPipelineState::PolygonFillModet::Wireframe);

    _material = move(material);
    _pipelineState = move(pipelineState);
    _indexArray = move(indexArray);
    _vertexArray = move(vertexArray);

    RendererDataResource::AccessMode accessMode;
    accessMode.cpuMode.writeMode = RendererDataResource::CPUAccessMode::Mode::FrequentFull;
    _vertexInstanceArray = RendererVertexArray::New(RendererArrayData<InstanceData>(BufferOwnedData{new ui8[maxInstances * sizeof(InstanceData)], [](void *p) {delete[] p; }}, maxInstances), accessMode);
}

ui32 SpheresInstanced::MaxInstances()
{
	return _vertexInstanceArray->NumberOfElements();
}

auto SpheresInstanced::Lock(ui32 instancesCount) -> InstanceData *
{
    return (InstanceData *)_vertexInstanceArray->LockDataRegion(instancesCount, 0, RendererDataResource::LockMode::Write);
}

void SpheresInstanced::Unlock()
{
    _vertexInstanceArray->UnlockDataRegion();
}

void SpheresInstanced::Draw(const Camera *camera, ui32 instancesCount)
{
	ASSUME(instancesCount <= MaxInstances());

	Application::GetRenderer().BindIndexArray(_indexArray);
    Application::GetRenderer().BindVertexArray(_vertexArray, 0);
    Application::GetRenderer().BindVertexArray(_vertexInstanceArray, 1);

    Application::GetRenderer().DrawIndexedWithCamera(camera, nullptr, _pipelineState.get(), _material.get(), PrimitiveTopology::TriangleStrip, _indexArray->NumberOfElements(), instancesCount);
}
