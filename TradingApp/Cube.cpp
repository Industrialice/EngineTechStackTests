#include "PreHeader.hpp"
#include "Cube.hpp"
#include <Material.hpp>
#include <RendererPipelineState.hpp>
#include <Application.hpp>
#include <Logger.hpp>
#include <Renderer.hpp>
#include <MatrixMathTypes.hpp>
#include <RendererArray.hpp>

using namespace EngineCore;
using namespace TradingApp;

Cube::Cube()
{
    auto shader = Application::LoadResource<Shader>("Colored3DVertices");
    if (shader == nullptr)
    {
        SENDLOG(Error, "Cube failed to load shader Colored3DVertices\n");
        return;
    }

    auto material = Material::New(shader);
    if (!material->UniformF32("ColorMul", {1, 1, 1, 1}))
    {
        SENDLOG(Error, "Failed to set ColorMul uniform for Colored3DVertices shader\n");
    }

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
        VertexLayout::Attribute{"position", VertexLayout::Attribute::Formatt::R32G32B32F, 0},
        VertexLayout::Attribute{"color", VertexLayout::Attribute::Formatt::R32G32B32A32F, 0}
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
}

void Cube::Draw(const Camera *camera, const Vector3 &position, const Vector3 &rotation, f32 size)
{
    if (_material == nullptr || _pipelineState == nullptr || _vertexArray == nullptr || _indexArray == nullptr)
    {
        return;
    }

    auto modelMatrix = Matrix4x3::CreateRTS(rotation, position, Vector3{size, size, size});

    Application::GetRenderer().BindIndexArray(_indexArray);
    Application::GetRenderer().BindVertexArray(_vertexArray, 0);

    Application::GetRenderer().DrawIndexedWithCamera(camera, &modelMatrix, _pipelineState.get(), _material.get(), PrimitiveTopology::TriangleEnumeration, _indexArray->NumberOfElements());
}

void Cube::Draw(const Camera *camera, const Vector3 &position, const Quaternion &rotation, f32 size)
{
    if (_material == nullptr || _pipelineState == nullptr || _vertexArray == nullptr || _indexArray == nullptr)
    {
        return;
    }

    auto modelMatrix = Matrix4x3::CreateRTS(rotation, position, Vector3{size, size, size});

    Application::GetRenderer().BindIndexArray(_indexArray);
    Application::GetRenderer().BindVertexArray(_vertexArray, 0);

    Application::GetRenderer().DrawIndexedWithCamera(camera, &modelMatrix, _pipelineState.get(), _material.get(), PrimitiveTopology::TriangleEnumeration, _indexArray->NumberOfElements());
}