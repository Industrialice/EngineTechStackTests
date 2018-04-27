#include "PreHeader.hpp"
#include "Scene.hpp"
#include <Application.hpp>
#include <Renderer.hpp>
#include <Camera.hpp>
#include "SceneBackground.hpp"
#include <Logger.hpp>
#include <Material.hpp>
#include <RendererPipelineState.hpp>
#include <RendererArray.hpp>
#include <MatrixMathTypes.hpp>
#include "Line3D.hpp"
#include "Cube.hpp"

using namespace EngineCore;
using namespace TradingApp;

static void DrawPyramid(const Camera &camera);

namespace
{
    shared_ptr<RendererVertexArray> PyramidVB;
    shared_ptr<RendererIndexArray> PyramidIB;
    shared_ptr<RendererPipelineState> PyramidPS;
    shared_ptr<Material> PyramidMaterial;

    unique_ptr<Line3D> TestLine;
    unique_ptr<Cube> TestCube;
}

bool Scene::Create()
{
    if (SceneBackground::Create(false) == false)
    {
        SENDLOG(Critical, "Failed to create scene background\n");
        return false;
    }

    auto shader = Application::LoadResource<Shader>("Colored3DVertices");
    if (shader == nullptr)
    {
        SENDLOG(Error, "Failed to load shader Colored3DVertices\n");
        return false;
    }

    f32 colorIntensity = 1.0f;

    PyramidMaterial = Material::New(shader);
    if (false == PyramidMaterial->UniformF32("ColorMul", {colorIntensity, colorIntensity, colorIntensity, 0.25f}))
    {
        SENDLOG(Error, "Failed to set ColorMul uniform for Colored3DVertices shader\n");
    }

    VertexLayout vertexLayout
    {
        VertexLayout::Attribute{"position", VertexLayout::Attribute::Formatt::R32G32B32F, 0},
        VertexLayout::Attribute{"color", VertexLayout::Attribute::Formatt::R32G32B32A32F, 0}
    };

    RendererPipelineState::BlendSettingst blendSettings;
    blendSettings.isEnabled = true;

    PyramidPS = RendererPipelineState::New(PyramidMaterial->Shader());
    PyramidPS->DepthComparisonFunc(RendererPipelineState::DepthComparisonFunct::LessEqual);
    PyramidPS->VertexDataLayout(vertexLayout);
    PyramidPS->PolygonCullMode(RendererPipelineState::PolygonCullModet::None);
    PyramidPS->PolygonFillMode(RendererPipelineState::PolygonFillModet::Wireframe);
    PyramidPS->BlendSettings(blendSettings);

    struct Vertex
    {
        f32 position[3];
        f32 color[4];
    };

    RendererArrayData<Vertex> vertexData
    {
        Vertex{{-1, -1, -1}, {1, 0, 0, 1}},
        Vertex{{1, -1, -1}, {0, 1, 0, 1}},
        Vertex{{-1, 1, -1}, {0, 0, 1, 1}},
        Vertex{{1, 1, -1}, {1, 0, 1, 1}},
        Vertex{{0, 0, 1}, {1, 1, 1, 1}}
    };
    PyramidVB = RendererVertexArray::New(move(vertexData), {RendererArray::CPUAccessMode::Mode::FrequentPartial});

    RendererArrayData<ui8> indexData{{
            2, 0, 1,
            3, 2, 1,
            0, 4, 1,
            1, 4, 3,
            3, 4, 2,
            2, 4, 0}};
    PyramidIB = RendererIndexArray::New(move(indexData));

    TestLine = make_unique<Line3D>();

    TestCube = make_unique<Cube>();

    SENDLOG(Info, "Scene initialization's completed\n");
    return true;
}

void Scene::Destroy()
{
    SceneBackground::Destroy();
    SENDLOG(Info, "Scene's been destroyed\n");
}

void Scene::Update()
{
    SceneBackground::Update();
}

void Scene::Draw(const Camera &camera)
{
    SceneBackground::Draw({0, 0, 0}, camera);

    //DrawPyramid(camera);

    for (f32 y = 1.0f; y >= -1.0f; y -= 0.3f)
    {
        TestLine->Draw(&camera, {-100, y, -1}, {100, y, -1}, 0.01f);
    }

    TestCube->Draw(&camera, {0, 0, 0}, {0, 0, 0}, 1);
}

void DrawPyramid(const Camera &camera)
{
    auto timeSinceStart = Application::GetEngineTime().secondsSinceStart;

    f32 randValue = rand() / (f32)RAND_MAX;

    struct Vertex
    {
        f32 position[3];
        f32 color[4];
    };

    byte *locked = PyramidVB->LockDataRegion(1, 1, RendererArray::LockMode::Write);
    *(Vertex *)locked = Vertex{{1, -1, -1}, {randValue, randValue, randValue, 1}};
    PyramidVB->UnlockDataRegion();

    Application::GetRenderer().BindVertexArray(PyramidVB, 0);
    Application::GetRenderer().BindIndexArray(PyramidIB);

    Matrix4x3 modelMatrix = Matrix4x3::CreateRTS(Vector3(timeSinceStart, timeSinceStart, timeSinceStart), Vector3());

    Application::GetRenderer().DrawIndexedWithCamera(&camera, nullptr, PyramidPS.get(), PyramidMaterial.get(), PrimitiveTopology::TriangleEnumeration, PyramidIB->NumberOfElements());
}