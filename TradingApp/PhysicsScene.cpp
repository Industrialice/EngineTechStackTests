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

struct BuildingShapes
{
	static constexpr struct BuildingShape : EnumCombinable<BuildingShape, ui32>
	{} LeftBorder = BuildingShape::Create(1 << 0),
		RightBorder = BuildingShape::Create(1 << 1),
		TopBorder = BuildingShape::Create(1 << 2),
		BottomBorder = BuildingShape::Create(1 << 3),
		Diagonal = BuildingShape::Create(1 << 4),
		ReverseDiagonal = BuildingShape::Create(1 << 5),
		Swastika = BuildingShape::Create(1 << 6),
		SwastikaInternal = BuildingShape::Create(1 << 7),
		Fill = BuildingShape::Create(1 << 8),
		UpperFloor = BuildingShape::Create(1 << 9),
		Borders = LeftBorder.Combined(RightBorder).Combined(TopBorder).Combined(BottomBorder);
};

static void PlaceSparse();
static void PlaceAsHugeCube();
static void PlaceAsTallTower(BuildingShapes::BuildingShape shape);
static void PlaceHelicopter();

static void ProcessControls(Vector3 mainCameraPosition, Vector3 mainCameraRotation);

#ifdef USE_XAUDIO
static void AddTestAudio();
static void AddAudioToNewCollisions();
#endif

namespace
{
	static constexpr uiw TowerWidth = 20;
	static constexpr uiw CubesCounts = (TowerWidth * TowerWidth) * 9;

	//static constexpr uiw TowerWidth = 10;
	//static constexpr uiw CubesCounts = (TowerWidth * TowerWidth) * 135; // 60 fps limit for GTX 970, if the app hangs, increase PxgDynamicsMemoryConfig values

	static constexpr uiw CubesSmallCount = 6;
	static constexpr uiw CubesMediumCount = 100;

	//static constexpr BuildingShapes::BuildingShape TowerShape = 
	//	BuildingShapes::LeftBorder.
	//	Combined(BuildingShapes::RightBorder).
	//	Combined(BuildingShapes::TopBorder).
	//	Combined(BuildingShapes::BottomBorder).
	//	Combined(BuildingShapes::Swastika);
	static constexpr BuildingShapes::BuildingShape TowerShape = BuildingShapes::SwastikaInternal.Combined(BuildingShapes::Borders);

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

	TimeDifference SpawnTimeout{};
	TimeMoment LastSpawnedTime = TimeMoment::Earliest();
}

bool PhysicsScene::Create()
{
	SpawnTimeout = 0.1_s;

	if (!PhysX::Create())
	{
		return false;
	}

	if (!SceneBackground::Create(true, true))
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
	PlaceAsTallTower(TowerShape);
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
			Cubes[x * CubesMediumCount + z] = PhysX::ObjectData{{}, pos, {}, {}, 1.0f};
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
				Cubes[computeIndex(x, y, z)] = PhysX::ObjectData{{}, pos, {}, {}, 1.0f};
			}
		}
	}
}

void PlaceAsTallTower(BuildingShapes::BuildingShape shape)
{
	assert(CubesCounts % (TowerWidth * TowerWidth) == 0);

	Cubes.clear();

	auto checkShape = [shape](ui32 x, ui32 z)
	{
		if (shape.Contains(BuildingShapes::LeftBorder))
		{
			if (x == 0)
			{
				return true;
			}
		}
		if (shape.Contains(BuildingShapes::RightBorder))
		{
			if (x == TowerWidth - 1)
			{
				return true;
			}
		}
		if (shape.Contains(BuildingShapes::TopBorder))
		{
			if (z == 0)
			{
				return true;
			}
		}
		if (shape.Contains(BuildingShapes::BottomBorder))
		{
			if (z == TowerWidth - 1)
			{
				return true;
			}
		}
		if (shape.Contains(BuildingShapes::Diagonal))
		{
			if (x == z)
			{
				return true;
			}
		}
		if (shape.Contains(BuildingShapes::ReverseDiagonal))
		{
			if (TowerWidth - 1 - x == z)
			{
				return true;
			}
		}
		if (shape.Contains(BuildingShapes::Swastika))
		{
			if (x == TowerWidth / 2 || z == TowerWidth / 2)
			{
				return true;
			}
			if (z == 0 && x < TowerWidth / 2)
			{
				return true;
			}
			if (x == 0 && z > TowerWidth / 2)
			{
				return true;
			}
			if (x == TowerWidth - 1 && z < TowerWidth / 2)
			{
				return true;
			}
			if (z == TowerWidth - 1 && x > TowerWidth / 2)
			{
				return true;
			}
		}
		if (shape.Contains(BuildingShapes::SwastikaInternal))
		{
			auto towerWidth = TowerWidth - 1;
			if (x > 0 && z > 0 && x < towerWidth && z < towerWidth)
			{
				if (x == towerWidth / 2 || z == towerWidth / 2)
				{
					return true;
				}
				if (z == 1 && x < towerWidth / 2)
				{
					return true;
				}
				if (x == 1 && z > towerWidth / 2)
				{
					return true;
				}
				if (x == towerWidth - 1 && z < towerWidth / 2)
				{
					return true;
				}
				if (z == towerWidth - 1 && x > towerWidth / 2)
				{
					return true;
				}
			}
		}
		if (shape.Contains(BuildingShapes::Fill))
		{
			return true;
		}
		return false;
	};

	f32 y = -0.5f;

	for (ui32 index = 0; index < CubesCounts / (TowerWidth * TowerWidth); ++index)
	{
		for (ui32 x = 0; x < TowerWidth; ++x)
		{
			for (ui32 z = 0; z < TowerWidth; ++z)
			{
				if (checkShape(x, z))
				{
					Cubes.push_back(PhysX::ObjectData{{}, {(f32)x, y, (f32)z}, {}, {}, 1.0f});
				}
			}
		}

		y += 1.0f;
	}

	if (shape.Contains(BuildingShapes::UpperFloor))
	{
		for (ui32 x = 0; x < TowerWidth; ++x)
		{
			Cubes.push_back(PhysX::ObjectData{{}, {(f32)x, y, 0.0f}, {}, {}, 1.0f});
			Cubes.push_back(PhysX::ObjectData{{}, {(f32)x, y, (f32)(TowerWidth - 1)}, {}, {}, 1.0f});
		}

		for (ui32 z = 1; z < TowerWidth - 1; ++z)
		{
			Cubes.push_back(PhysX::ObjectData{{}, {0.0f, y, (f32)z}, {}, {}, 1.0f});
			Cubes.push_back(PhysX::ObjectData{{}, {(f32)(TowerWidth - 1), y, (f32)z}, {}, {}, 1.0f});
		}
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
				Cubes[index * (TowerWidth * TowerWidth) + indexOffset] = PhysX::ObjectData{{}, {(f32)x, y, (f32)z}, {}, {}, 1.0f};
			}
		}

		y += 1.0f;
	}
}

void ProcessControls(Vector3 mainCameraPosition, Vector3 mainCameraRotation)
{
	auto &keyController = Application::GetKeyController();

	bool isLPressed = keyController.GetKeyInfo(KeyCode::MButton0).IsPressed();
	bool isRPressed = keyController.GetKeyInfo(KeyCode::MButton1).IsPressed();
	bool isMPressed = keyController.GetKeyInfo(KeyCode::MButton2).IsPressed();

	if (isLPressed == false && isRPressed == false && isMPressed == false)
	{
		return;
	}

	TimeMoment current = TimeMoment::Now();
	if (current < LastSpawnedTime + SpawnTimeout)
	{
		return;
	}

	LastSpawnedTime = current + SpawnTimeout;

	Cubes.clear();
	Spheres.clear();

	auto rotMatrix = Matrix3x3::CreateRS(mainCameraRotation);
	Vector3 objectDirection = Vector3(0, 0, 1) * rotMatrix;
	Vector3 objectPosition = mainCameraPosition + objectDirection * 5;

	Vector3 torque;
	if (isRPressed)
	{
		f32 x = (static_cast<f32>(rand()) / RAND_MAX * 100.f) - 50.0f;
		f32 y = (static_cast<f32>(rand()) / RAND_MAX * 100.f) - 50.0f;
		f32 z = (static_cast<f32>(rand()) / RAND_MAX * 100.f) - 50.0f;
		torque = {x, y, z};
	}
	
	f32 speed = 5000;
	if (isMPressed)
	{
		speed *= 0.1f;
	}

	Cubes.push_back(PhysX::ObjectData{{}, objectPosition, objectDirection * speed, torque, 1.0f});

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