#include "PreHeader.hpp"
#include "PhysicsScene.hpp"
#include "SceneBackground.hpp"
#include <MatrixMathTypes.hpp>
#include <MathFunctions.hpp>
#include <Application.hpp>
#include <Logger.hpp>
#include "CubesInstanced.hpp"
#include "SpheresInstanced.hpp"
#include "PhysX.hpp"
#include "XAudio2.hpp"
#include "AudioWaveFormatParser.hpp"

using namespace EngineCore;
using namespace TradingApp;

static void PlaceSparse();
static void PlaceAsHugeCube();
static void PlaceAsTallTower();
static void PlaceHelicopter();
static void AddTestAudio();

namespace
{
    static constexpr uiw TowerWidth = 7;

    static constexpr uiw CubesCounts = (TowerWidth * TowerWidth) * 10;
    static constexpr uiw CubesSmallCount = 6;
    static constexpr uiw CubesMediumCount = 100;
    vector<CubesInstanced::InstanceData> Cubes{};
    vector<SpheresInstanced::InstanceData> Spheres{};
    unique_ptr<XAudioEngine> AudioEngine{};
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

    AudioEngine = XAudioEngine::New();
    if (!AudioEngine)
    {
        return false;
    }

    AddTestAudio();

    Restart();

    return true;
}

void PhysicsScene::Destroy()
{
    AudioEngine = {};
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
    Cubes.clear();
    Spheres.clear();

    //PlaceSparse();
    //PlaceAsHugeCube();
    PlaceAsTallTower();
    //PlaceHelicopter();

    PhysX::SetObjects(Cubes, Spheres);
}

void PhysicsScene::Draw(const Camera &camera)
{
    SceneBackground::Draw({MathPi<f32>() * 0.5f, 0, 0}, camera);

    PhysX::Draw(camera);
}

void PlaceSparse()
{
    Cubes.resize(CubesMediumCount * CubesMediumCount);

    for (uiw x = 0; x < CubesMediumCount; ++x)
    {
        for (uiw z = 0; z < CubesMediumCount; ++z)
        {
            Vector3 pos{x * 2.0f, 0.5f + (rand() / (f32)RAND_MAX) * 5.0f, z * 2.0f};
            Cubes[x * CubesMediumCount + z] = CubesInstanced::InstanceData{{}, pos, 1.0f};
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
                Cubes[computeIndex(x, y, z)] = CubesInstanced::InstanceData{{}, pos, 1.0f};
            }
        }
    }
}

void PlaceAsTallTower()
{
    assert(CubesCounts % (TowerWidth * TowerWidth) == 0);

    Cubes.resize(CubesCounts);

    f32 y = -0.5f;

    for (ui32 index = 0; index < CubesCounts / (TowerWidth * TowerWidth); ++index)
    {
        for (ui32 x = 0; x < TowerWidth; ++x)
        {
            for (ui32 z = 0; z < TowerWidth; ++z)
            {
                ui32 indexOffset = x * TowerWidth + z;
                Cubes[index * (TowerWidth * TowerWidth) + indexOffset] = CubesInstanced::InstanceData{{}, {(f32)x, y, (f32)z}, 1.0f};
            }
        }

        y += 1.0f;
    }

    for (ui32 x = 0; x < TowerWidth; ++x)
    {
        Cubes.push_back(CubesInstanced::InstanceData{{}, {(f32)x, y, 0.0f}, 1.0f});
        Cubes.push_back(CubesInstanced::InstanceData{{}, {(f32)x, y, (f32)(TowerWidth - 1)}, 1.0f});
    }

    for (ui32 z = 1; z < TowerWidth - 1; ++z)
    {
        Cubes.push_back(CubesInstanced::InstanceData{{}, {0.0f, y, (f32)z}, 1.0f});
        Cubes.push_back(CubesInstanced::InstanceData{{}, {(f32)(TowerWidth - 1), y, (f32)z}, 1.0f});
    }

    /*Spheres.resize(1);
    Spheres[0] = SpheresInstanced::InstanceData{{}, {TowerWidth / 2.0f, Cubes.back().position.y + 1.0f, TowerWidth / 2.0f}, 1.0f};*/

    /*Spheres.resize(CubesCounts);

    f32 y = 0.5f;

    for (ui32 index = 0; index < CubesCounts / (TowerWidth * TowerWidth); ++index)
    {
        for (ui32 x = 0; x < TowerWidth; ++x)
        {
            for (ui32 z = 0; z < TowerWidth; ++z)
            {
                ui32 indexOffset = x * TowerWidth + z;
                Spheres[index * (TowerWidth * TowerWidth) + indexOffset] = SpheresInstanced::InstanceData{{}, {(f32)x, y, (f32)z}, 1.0f};
            }
        }

        y += 1.0f;
    }

    for (ui32 x = 0; x < TowerWidth; ++x)
    {
        Spheres.push_back(SpheresInstanced::InstanceData{{}, {(f32)x, y, 0.0f}, 1.0f});
        Spheres.push_back(SpheresInstanced::InstanceData{{}, {(f32)x, y, (f32)(TowerWidth - 1)}, 1.0f});
    }

    for (ui32 z = 1; z < TowerWidth - 1; ++z)
    {
        Spheres.push_back(SpheresInstanced::InstanceData{{}, {0.0f, y, (f32)z}, 1.0f});
        Spheres.push_back(SpheresInstanced::InstanceData{{}, {(f32)(TowerWidth - 1), y, (f32)z}, 1.0f});
    }*/
}

void PlaceHelicopter()
{
    static constexpr uiw heliCubesCount = (TowerWidth * TowerWidth) * 10;

    Cubes.resize(heliCubesCount);

    f32 y = -0.5f;

    for (ui32 index = 0; index < heliCubesCount / (TowerWidth * TowerWidth); ++index)
    {
        for (ui32 x = 0; x < TowerWidth; ++x)
        {
            for (ui32 z = 0; z < TowerWidth; ++z)
            {
                ui32 indexOffset = x * TowerWidth + z;
                Cubes[index * (TowerWidth * TowerWidth) + indexOffset] = CubesInstanced::InstanceData{{}, {(f32)x, y, (f32)z}, 1.0f};
            }
        }

        y += 1.0f;
    }
}

void AddTestAudio()
{
    FILE *f = fopen("../Resources/Audio/[MLP] [PMV] [SFM] - Flutterwonder.wav", "rb");
    if (!f)
    {
        SENDLOG(Error, "Failed to open the test audio\n");
        return;
    }

    fseek(f, 0, SEEK_END);
    ui32 fileSize = (ui32)ftell(f);
    fseek(f, 0, SEEK_SET);

    auto data = make_unique<ui8[]>(fileSize);
    fread(data.get(), 1, fileSize, f);

    fclose(f);

    auto parsedFile = ParseWaveFormatHeader(data.get(), fileSize);
    if (!parsedFile)
    {
        SENDLOG(Error, "ParseWaveFormatHeader for the test audio failed\n");
        return;
    }

    auto audio = AudioEngine->AddAudio(move(data), parsedFile->dataStartOffset, parsedFile->dataChunkHeader.chunkDataSize);
    AudioEngine->StartAudio(audio);
}