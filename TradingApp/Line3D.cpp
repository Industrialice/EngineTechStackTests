#include "PreHeader.hpp"
#include "Line3D.hpp"
#include <Material.hpp>
#include <RendererPipelineState.hpp>
#include <RendererArray.hpp>
#include <Application.hpp>
#include <Logger.hpp>
#include <Renderer.hpp>
#include <MatrixMathTypes.hpp>

using namespace TradingApp;
using namespace EngineCore;

Line3D::Line3D()
{
    constexpr f32 rls = 0.2f; // real line scale

    auto shader = Application::LoadResource<Shader>("Line3D");
    if (shader == nullptr)
    {
        SENDLOG(Error, "Line3D failed to load shader Line3D\n");
        return;
    }

    auto material = Material::New(shader);

    constexpr ui32 vertexCount = 16;

    static const Vector3 vertexPositions[vertexCount]
    {
        // front
        {-1, -1, -1},
        {-1, 1, -1},
        {1, 1, -1},
        {1, -1, -1},

        // up
        {-1, 1, -1},
        {-1, 1, 1},
        {1, 1, 1},
        {1, 1, -1},

        // back
        {1, -1, 1},
        {1, 1, 1},
        {-1, 1, 1},
        {-1, -1, 1},

        // down
        {-1, -1, 1},
        {-1, -1, -1},
        {1, -1, -1},
        {1, -1, 1}
    };

    static const Vector2 vertexTexcoords[vertexCount]
    {
        // front
        {0, 0},
        {0, 1},
        {1, 1},
        {1, 0},

        // up
        {0, 0},
        {0, 1},
        {1, 1},
        {1, 0},

        // back
        {0, 0},
        {0, 1},
        {1, 1},
        {1, 0},

        // down
        {0, 0},
        {0, 1},
        {1, 1},
        {1, 0}
    };

    struct Vertex
    {
        Vector3 position;
        Vector2 texcoord;
        Vector3 scale;
    };

    auto posTexcoordData = make_unique<Vertex[]>(vertexCount);
    for (ui32 index = 0; index < vertexCount; ++index)
    {
        auto &vertex = posTexcoordData[index];
        vertex = {vertexPositions[index], vertexTexcoords[index]};
        if (index & 0x4) // up or down
        {
            vertex.scale = {1, rls, 1};
        }
        else // front or back
        {
            vertex.scale = {1, 1, rls};
        }
    }

    RendererArrayData<Vertex> vertexArrayData{BufferOwnedData{(byte *)posTexcoordData.release(), [](void *p) {delete[] p; }}, vertexCount};

    auto vertexArray = RendererVertexArray::New(move(vertexArrayData));

    RendererArrayData<ui8> indexData
    {
        // front
        {0, 1, 2, 3, 0, 2,
        // up
        4, 5, 6, 7, 4, 6,
        // back
        8, 9, 10, 11, 8, 10,
        // down
        12, 13, 14, 15, 12, 14}
    };

    auto indexArray = RendererIndexArray::New(move(indexData));

    VertexLayout vertexLayout{
        VertexLayout::Attribute{"position", VertexLayout::Attribute::Formatt::R32G32B32F, 0},
        VertexLayout::Attribute{"texcoord", VertexLayout::Attribute::Formatt::R32G32F, 0},
        VertexLayout::Attribute{"scale", VertexLayout::Attribute::Formatt::R32G32B32F, 0}};

    auto pipelineState = RendererPipelineState::New(shader);
    RendererPipelineState::BlendSettingst blend;
    blend.isEnabled = true;
    blend.colorCombineMode = RendererPipelineState::BlendCombineModet::Add;
    pipelineState->BlendSettings(blend);
    pipelineState->VertexDataLayout(vertexLayout);
    pipelineState->EnableDepthWrite(false);

    _material = move(material);
    _pipelineState = move(pipelineState);
    _indexArray = move(indexArray);
    _vertexArray = move(vertexArray);

    auto shaderForDepthWrite = Application::LoadResource<Shader>("OnlyDepthWrite");

    _materialForDepthWrite = Material::New(shaderForDepthWrite);
    _materialForDepthWrite->UniformF32("_SidesScale", {1, rls, rls});

    _pipelineStateForDepthWrite = RendererPipelineState::New(shaderForDepthWrite);
    _pipelineStateForDepthWrite->VertexDataLayout(vertexLayout);
}

void Line3D::Draw(const Camera *camera, const Vector3 &point0, const Vector3 &point1, f32 thickness)
{
    if (_material == nullptr || _pipelineState == nullptr || _vertexArray == nullptr || _indexArray == nullptr)
    {
        return;
    }

    Vector3 direction = point1 - point0;
    Vector3 center = point0.GetLerped(point1, 0.5f);
    Vector3 scale{direction.Length() * 2, thickness * 2, thickness * 2};
    Vector3 directionNormal = direction.GetNormalized();

    auto modelMatrix = Matrix4x3::CreateRTS(nullopt, center, scale);

    Application::GetRenderer().BindIndexArray(_indexArray);
    Application::GetRenderer().BindVertexArray(_vertexArray, 0);

    // write only color
    Application::GetRenderer().DrawIndexedWithCamera(camera, &modelMatrix, _pipelineState.get(), _material.get(), PrimitiveTopology::TriangleEnumeration, _indexArray->NumberOfElements());

    // write only depth
    Application::GetRenderer().DrawIndexedWithCamera(camera, &modelMatrix, _pipelineStateForDepthWrite.get(), _materialForDepthWrite.get(), PrimitiveTopology::TriangleEnumeration, _indexArray->NumberOfElements());
}
