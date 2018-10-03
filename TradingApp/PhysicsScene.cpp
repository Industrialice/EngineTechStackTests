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
#include <Camera.hpp>
#include "SoundCache.hpp"

using namespace EngineCore;
using namespace TradingApp;

static void PlaceSparse();
static void PlaceAsHugeCube();
static void PlaceAsTallTower();
static void PlaceHelicopter();
static void AddTestAudio();
static void AddAudioToNewCollisions();

namespace
{
    static constexpr uiw TowerWidth = 7;

    static constexpr uiw CubesCounts = (TowerWidth * TowerWidth) * 10;
    static constexpr uiw CubesSmallCount = 6;
    static constexpr uiw CubesMediumCount = 100;
    vector<CubesInstanced::InstanceData> Cubes{};
    vector<SpheresInstanced::InstanceData> Spheres{};
    unique_ptr<XAudioEngine> AudioEngine{};
    unique_ptr<SoundCache> SoundCacheInstance = SoundCache::New();

    struct AudioSourceInRun
    {
        XAudioEngine::AudioSource audio;
        TimeMoment startedAt;
    };
    AudioSourceInRun CollisionAudioSources[16]{};
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

    //AddTestAudio();

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
    //AddAudioToNewCollisions();

    PhysX::Update();
    SceneBackground::Update();

    AudioEngine->Update();
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

    XAudioEngine::PositioningInfo positioning;
    positioning.orientFront = camera.ForwardAxis();
    positioning.orientTop = camera.UpAxis();
    positioning.position = camera.Position();
    positioning.velocity = {};
    AudioEngine->SetListenerPositioning(positioning);
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
    auto stream = SoundCacheInstance->GetAudioStream(FilePath::FromChar("../Resources/Audio/bullet_impact_concrete.wav"));
    if (!stream)
    {
        SENDLOG(Error, "Failed to load test audio file\n");
        return;
    }

    XAudioEngine::PositioningInfo positioning;
    positioning.orientFront = {0, 0, 1};
    positioning.orientTop = {0, 1, 0};
    positioning.position = {0, 0, 0};
    positioning.velocity = {};

    auto audio = AudioEngine->AddAudio(move(*stream), true, positioning);
}

void AddAudioToNewCollisions()
{
    auto currentMoment = TimeMoment::Now();

    for (auto &audio : CollisionAudioSources)
    {
        if (!audio.audio.IsValid())
        {
            continue;
        }
        f32 delta = (currentMoment - audio.startedAt).ToSeconds();
        if (delta - 0.05f >= AudioEngine->AudioLength(audio.audio))
        {
            AudioEngine->RemoveAudio(audio.audio);
            audio.audio = {};
        }
    }

    auto findFreeSlot = []() -> optional<uiw>
    {
        for (uiw index = 0; index < CountOf(CollisionAudioSources); ++index)
        {
            if (!CollisionAudioSources[index].audio.IsValid())
            {
                return index;
            }
        }
        return {};
    };

    auto findFreeableAudio = []() -> optional<uiw>
    {
        TimeMoment oldestAudioMoment;
        uiw oldestAudioIndex = 0;

        for (uiw index = 0; index < CountOf(CollisionAudioSources); ++index)
        {
            if (CollisionAudioSources[index].startedAt < oldestAudioMoment)
            {
                oldestAudioMoment = CollisionAudioSources[index].startedAt;
                oldestAudioIndex = index;
            }
        }

        if (AudioEngine->AudioPositionPercent(CollisionAudioSources[oldestAudioIndex].audio) < 75)
        {
            return {};
        }

        return oldestAudioIndex;
    };

    auto newContacts = PhysX::GetNewContacts();
    for (uiw index = 0; index < newContacts.second; ++index)
    {
        const auto &contact = newContacts.first[index];
        if (contact.impulse < 0.25f)
        {
            continue;
        }

        uiw addToIndex;
        auto freeSlot = findFreeSlot();
        if (freeSlot)
        {
            addToIndex = *freeSlot;
        }
        else
        {
            auto freeableIndex = findFreeableAudio();
            if (!freeableIndex)
            {
                return;
            }
            addToIndex = *freeableIndex;
            AudioEngine->RemoveAudio(CollisionAudioSources[addToIndex].audio);
        }

        auto audioStream = SoundCacheInstance->GetAudioStream(FilePath::FromChar("../Resources/Audio/bullet_impact_concrete.wav"));
        if (!audioStream)
        {
            SOFTBREAK;
            return;
        }

        XAudioEngine::PositioningInfo positioning;
        positioning.position = contact.position;

        CollisionAudioSources[addToIndex].audio = AudioEngine->AddAudio(move(*audioStream), true, positioning);
        CollisionAudioSources[addToIndex].startedAt = TimeMoment::Now();
    }
}