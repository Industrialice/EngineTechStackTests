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
#include <IKeyController.hpp>

using namespace EngineCore;
using namespace TradingApp;

static void PlaceSparse();
static void PlaceAsHugeCube();
static void PlaceAsTallTower(bool isShallow);
static void PlaceHelicopter();

static void ProcessControls(Vector3 mainCameraPosition, Vector3 mainCameraRotation);

#ifdef USE_XAUDIO
	static void AddTestAudio();
	static void AddAudioToNewCollisions();
#endif

namespace
{
    static constexpr uiw TowerWidth = 20;
	static constexpr uiw CubesCounts = (TowerWidth * TowerWidth) * 5;

	//static constexpr uiw TowerWidth = 10;
    //static constexpr uiw CubesCounts = (TowerWidth * TowerWidth) * 135; // 60 fps limit for GTX 970, if the app hangs, increase PxgDynamicsMemoryConfig values
    static constexpr uiw CubesSmallCount = 6;
    static constexpr uiw CubesMediumCount = 100;
    vector<PhysX::ObjectData> Cubes{};
    vector<PhysX::ObjectData> Spheres{};
	#ifdef USE_XAUDIO
		unique_ptr<XAudioEngine> AudioEngine{};
	#endif
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

	#ifdef USE_XAUDIO
		AudioEngine = XAudioEngine::New();
		if (!AudioEngine)
		{
			return false;
		}

		//AddTestAudio();
	#endif

    Restart();

    return true;
}

void PhysicsScene::Destroy()
{
	#ifdef USE_XAUDIO
		AudioEngine = {};
	#endif
    SceneBackground::Destroy();
    PhysX::Destroy();
}

void PhysicsScene::Update(Vector3 mainCameraPosition, Vector3 mainCameraRotation)
{
    //AddAudioToNewCollisions();

	ProcessControls(mainCameraPosition, mainCameraRotation);

    PhysX::Update();
    SceneBackground::Update();

	#ifdef USE_XAUDIO
		AudioEngine->Update();
	#endif
}

void PhysicsScene::Restart()
{
    Cubes.clear();
    Spheres.clear();

    //PlaceSparse();
    //PlaceAsHugeCube();
    PlaceAsTallTower(true);
    //PlaceHelicopter();

	PhysX::ClearObjects();
    PhysX::AddObjects(Cubes, Spheres);
}

void PhysicsScene::Draw(const Camera &camera)
{
    SceneBackground::Draw({MathPi<f32>() * 0.5f, 0, 0}, camera);

    PhysX::Draw(camera);

	#ifdef USE_XAUDIO
		XAudioEngine::PositioningInfo positioning;
		positioning.orientFront = camera.ForwardAxis();
		positioning.orientTop = camera.UpAxis();
		positioning.position = camera.Position();
		positioning.velocity = {};
		AudioEngine->SetListenerPositioning(positioning);
	#endif
}

void PlaceSparse()
{
    Cubes.resize(CubesMediumCount * CubesMediumCount);

    for (uiw x = 0; x < CubesMediumCount; ++x)
    {
        for (uiw z = 0; z < CubesMediumCount; ++z)
        {
            Vector3 pos{x * 2.0f, 0.5f + (rand() / (f32)RAND_MAX) * 5.0f, z * 2.0f};
			Cubes[x * CubesMediumCount + z] = PhysX::ObjectData{{}, pos, {}, 1.0f};
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
				Cubes[computeIndex(x, y, z)] = PhysX::ObjectData{{}, pos, {}, 1.0f};
            }
        }
    }
}

void PlaceAsTallTower(bool isShallow)
{
    assert(CubesCounts % (TowerWidth * TowerWidth) == 0);

	Cubes.clear();

    f32 y = -0.5f;

    for (ui32 index = 0; index < CubesCounts / (TowerWidth * TowerWidth); ++index)
    {
        for (ui32 x = 0; x < TowerWidth; ++x)
        {
            for (ui32 z = 0; z < TowerWidth; ++z)
            {
				if (isShallow)
				{
					if (x != 0 && x != (TowerWidth - 1) && z != 0 && z != (TowerWidth - 1))
					{
						continue;
					}
				}

				Cubes.push_back(PhysX::ObjectData{{}, {(f32)x, y, (f32)z}, {}, 1.0f});
            }
        }

        y += 1.0f;
    }

    for (ui32 x = 0; x < TowerWidth; ++x)
    {
		Cubes.push_back(PhysX::ObjectData{{}, {(f32)x, y, 0.0f}, {}, 1.0f});
		Cubes.push_back(PhysX::ObjectData{{}, {(f32)x, y, (f32)(TowerWidth - 1)}, {}, 1.0f});
    }

    for (ui32 z = 1; z < TowerWidth - 1; ++z)
    {
		Cubes.push_back(PhysX::ObjectData{{}, {0.0f, y, (f32)z}, {}, 1.0f});
		Cubes.push_back(PhysX::ObjectData{{}, {(f32)(TowerWidth - 1), y, (f32)z}, {}, 1.0f});
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
				Cubes[index * (TowerWidth * TowerWidth) + indexOffset] = PhysX::ObjectData{{}, {(f32)x, y, (f32)z}, {}, 1.0f};
            }
        }

        y += 1.0f;
    }
}

void ProcessControls(Vector3 mainCameraPosition, Vector3 mainCameraRotation)
{
	auto &keyController = Application::GetKeyController();

	if (keyController.GetKeyInfo(KeyCode::MButton0).IsPressed() == false)
	{
		return;
	}

	Cubes.clear();
	Spheres.clear();

	auto rotMatrix = Matrix3x3::CreateRS(mainCameraRotation);
	Vector3 objectDirection = Vector3(0, 0, 1) * rotMatrix;
	Vector3 objectPosition = mainCameraPosition + objectDirection * 2;

	Cubes.push_back(PhysX::ObjectData{{}, objectPosition, objectDirection * 25, 1.0f});

	PhysX::AddObjects(Cubes, Spheres);
}

#ifdef USE_XAUDIO
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
        f32 delta = (currentMoment - audio.startedAt).ToSec();
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
#endif