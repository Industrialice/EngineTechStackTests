#include "PreHeader.hpp"
#include "PhysicsScene.hpp"
#include "SceneBackground.hpp"
#include <MatrixMathTypes.hpp>
#include <MathFunctions.hpp>
#include "CubesInstanced.hpp"
#include "PhysX.hpp"

using namespace EngineCore;
using namespace TradingApp;

static void PlaceSparse();
static void PlaceAsHugeCube();
static void PlaceAsTallTower();
//static void PlaceRandomly();
//static void PlaceCubeRandomly(CubesInstanced &cube);

namespace
{
    static constexpr uiw CubesCounts = 36 * 185;
    static constexpr uiw CubesSmallCount = 6;
    static constexpr uiw CubesMediumCount = 100;
    vector<CubesInstanced::InstanceData> Cubes{};
    unique_ptr<CubesInstanced> InstancedCubes{};
}

bool PhysicsScene::Create()
{
    if (!PhysX::Create())
    {
        return false;
    }

    if (!SceneBackground::Create(true))
    {
        return false;
    }

    Restart();

    InstancedCubes = make_unique<CubesInstanced>(Cubes.size());

    return true;
}

void PhysicsScene::Destroy()
{
    SceneBackground::Destroy();
    PhysX::Destroy();
}

void PhysicsScene::Update()
{
    PhysX::Update();
    SceneBackground::Update();
}

void PhysicsScene::Restart()
{
    //PlaceSparse();
    //PlaceAsHugeCube();
    PlaceAsTallTower();

    PhysX::SetObjects(Cubes);
}

void PhysicsScene::Draw(const Camera &camera)
{
    SceneBackground::Draw({MathPi<f32>() * 0.5f, 0, 0}, camera);

    InstancedCubes->Draw(&camera, Cubes.data(), (ui32)Cubes.size());

    PhysX::FinishUpdate();
}

void PlaceSparse()
{
    Cubes.resize(CubesMediumCount * CubesMediumCount);

    for (uiw x = 0; x < CubesMediumCount; ++x)
    {
        for (uiw z = 0; z < CubesMediumCount; ++z)
        {
            Vector3 pos{x * 2.0f, 0.5f + (rand() / (f32)RAND_MAX) * 5.0f, z * 2.0f};
            Cubes[x * CubesMediumCount + z] = CubesInstanced::InstanceData{pos, {}, 1.0f};
        }
    }
}

void PlaceAsHugeCube()
{
    Cubes.resize(CubesSmallCount * CubesSmallCount * CubesSmallCount);

    f32 yInitial = 5;
    f32 xInitial = -5;
    f32 zInitial = -5;

    auto computeIndex = [](uiw x, uiw y, uiw z)
    {
        return z + x * CubesSmallCount + y * CubesSmallCount * x;
    };

    for (uiw y = 0; y < CubesSmallCount; ++y)
    {
        for (uiw x = 0; x < CubesSmallCount; ++x)
        {
            for (uiw z = 0; z < CubesSmallCount; ++z)
            {
                Vector3 pos{xInitial + x, yInitial + y, zInitial + z};
                Cubes[computeIndex(x, y, z)] = CubesInstanced::InstanceData{pos, {}, 1.0f};
            }
        }
    }
}

void PlaceAsTallTower()
{
    assert(CubesCounts % 36 == 0);

    Cubes.resize(CubesCounts);

    for (ui32 index = 0; index < CubesCounts / 36; ++index)
    {
        for (ui32 x = 0; x < 6; ++x)
        {
            for (ui32 z = 0; z < 6; ++z)
            {
                ui32 indexOffset = x * 6 + z;
                Cubes[index * 36 + indexOffset] = CubesInstanced::InstanceData{{(f32)x, 0.5f + index, (f32)z}, {}, 1.0f};
            }
        }
    }
}

/*void PlaceRandomly()
{
    Cubes.resize(CubesCounts);

    for (ui32 index = 0; index < CubesCounts; ++index)
    {
        PlaceCubeRandomly(Cubes[index]);
    }
}

void PlaceCubeRandomly(PhysicsCube &cube)
{
    f32 x = ((rand() - RAND_MAX / 2) / (f32)RAND_MAX) * 35;
    f32 y = (rand() / (f32)RAND_MAX) * 90 + 0.5f + 5;
    f32 z = ((rand() - RAND_MAX / 2) / (f32)RAND_MAX) * 35;

    f32 rx = (rand() / (f32)RAND_MAX) * MathPi<f32>() * 2;
    f32 ry = (rand() / (f32)RAND_MAX) * MathPi<f32>() * 2;
    f32 rz = (rand() / (f32)RAND_MAX) * MathPi<f32>() * 2;

    cube = PhysicsCube({x, y, z}, Quaternion::FromEuler({rx, ry, rz}), 1.0f);
}*/