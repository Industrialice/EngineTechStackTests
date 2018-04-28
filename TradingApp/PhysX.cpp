#include "PreHeader.hpp"
#include "PhysX.hpp"
#include <PxPhysicsAPI.h>
#include <Application.hpp>
#include <Logger.hpp>
#include <MathFunctions.hpp>

#ifdef DEBUG
    #pragma comment(lib, "PxFoundationDEBUG_x64.lib")
    #pragma comment(lib, "PhysX3CookingDEBUG_x64.lib")
    #pragma comment(lib, "PhysX3ExtensionsDEBUG.lib")
    #pragma comment(lib, "PxPvdSDKDEBUG_x64.lib")
    #pragma comment(lib, "PhysX3CommonDEBUG_x64.lib")
    #pragma comment(lib, "PhysX3DEBUG_x64.lib")
#else
    #pragma comment(lib, "PxFoundation_x64.lib")
    #pragma comment(lib, "PhysX3Cooking_x64.lib")
    #pragma comment(lib, "PhysX3Extensions.lib")
    #pragma comment(lib, "PxPvdSDK_x64.lib")
    #pragma comment(lib, "PhysX3Common_x64.lib")
    #pragma comment(lib, "PhysX3_x64.lib")
#endif

using namespace EngineCore;
using namespace TradingApp;
using namespace physx;

#define PVD_HOST "127.0.0.1" // Set this to the IP address of the system running the PhysX Visual Debugger that you want to connect to.

namespace
{
    PxDefaultAllocator DefaultAllocator{};
    PxDefaultErrorCallback DefaultErrorCallback{};
    PxFoundation *Foundation{};
    PxPvd *Pvd{};
    PxPhysics *Physics{};
    PxCooking *Cooking{};
    PxDefaultCpuDispatcher *CpuDispatcher{};
    PxScene *PhysXScene{};
    PxMaterial *PhysXMaterial{};
    PxRigidStatic *GroundPlane{};

    bool IsInitialized = false;
    vector<CubesInstanced::InstanceData> *Objects{};

    struct PhysXCubeData
    {
        PxRigidDynamic *actor{};
        PxShape *shape{};

        ~PhysXCubeData()
        {
            if (PhysXScene)
            {
                PhysXScene->removeActor(*actor);
                actor->detachShape(*shape);
                //shape->release();
                actor->release();
            }
        }
    };
    vector<PhysXCubeData> PhysXCubeDatas{};
}

bool PhysX::Create()
{
    assert(!IsInitialized);

    Foundation = PxCreateFoundation(PX_FOUNDATION_VERSION, DefaultAllocator, DefaultErrorCallback);
    if (!Foundation)
    {
        SENDLOG(Error, "PhysX::Create -> PxCreateFoundation failed\n");
        return false;
    }

    Pvd = PxCreatePvd(*Foundation);
    PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
    Pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

    Physics = PxCreatePhysics(PX_PHYSICS_VERSION, *Foundation, PxTolerancesScale(), false, Pvd);
    if (!Physics)
    {
        SENDLOG(Error, "PhysX::Create -> PxCreatePhysics failed\n");
        return false;
    }

    Cooking = PxCreateCooking(PX_PHYSICS_VERSION, *Foundation, PxCookingParams(Physics->getTolerancesScale()));
    if (!Cooking)
    {
        SENDLOG(Error, "PhysX::Create -> PxCreateCooking failed\n");
        return false;
    }

    if (!PxInitExtensions(*Physics, Pvd))
    {
        SENDLOG(Error, "PhysX::Create -> PxInitExtensions failed\n");
        return false;
    }

    CpuDispatcher = PxDefaultCpuDispatcherCreate(3);
    if (!CpuDispatcher)
    {
        SENDLOG(Error, "PhysX::Create -> PxDefaultCpuDispatcherCreate failed\n");
        return false;
    }

    PxSceneDesc sceneDesc(Physics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = CpuDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    PhysXScene = Physics->createScene(sceneDesc);
    if (!PhysXScene)
    {
        SENDLOG(Error, "PhysX::Create -> Physics->createScene failed\n");
        return false;
    }

    PxPvdSceneClient* pvdClient = PhysXScene->getScenePvdClient();
    if (pvdClient)
    {
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
    }

    PhysXMaterial = Physics->createMaterial(0.5f, 0.5f, 0.6f);

    GroundPlane = PxCreatePlane(*Physics, PxPlane(0, 1, 0, 0), *PhysXMaterial);
    PhysXScene->addActor(*GroundPlane);

    IsInitialized = true;

    return true;
}

void PhysX::Destroy()
{
    if (GroundPlane)
    {
        GroundPlane->release();
        GroundPlane = nullptr;
    }
    if (PhysXMaterial)
    {
        PhysXMaterial->release();
        PhysXMaterial = nullptr;
    }
    if (PhysXScene)
    {
        PhysXScene->release();
        PhysXScene = nullptr;
    }
    if (CpuDispatcher)
    {
        CpuDispatcher->release();
        CpuDispatcher = nullptr;
    }
    PxCloseExtensions();
    if (Cooking)
    {
        Cooking->release();
        Cooking = nullptr;
    }
    if (Physics)
    {
        Physics->release();
        Physics = nullptr;
    }
    if (Pvd)
    {
        Pvd->release();
        Pvd = nullptr;
    }
    if (Foundation)
    {
        Foundation->release();
        Foundation = nullptr;
    }
}

void PhysX::Update()
{
    if (!IsInitialized || !Objects)
    {
        return;
    }

    PhysXScene->simulate(Application::GetEngineTime().secondSinceLastFrame);
}

void PhysX::FinishUpdate()
{
    if (!IsInitialized || !Objects)
    {
        return;
    }

    PhysXScene->fetchResults(true);

    for (uiw index = 0, size = Objects->size(); index < size; ++index)
    {
        auto &data = PhysXCubeDatas[index];
        auto &object = Objects->operator[](index);

        auto position = data.actor->getGlobalPose().p;
        auto rotation = data.actor->getGlobalPose().q;

        object.position = {position.x, position.y, position.z};
        object.rotation = {rotation.x, rotation.y, rotation.z, rotation.w};
    }
}

void PhysX::SetObjects(vector<CubesInstanced::InstanceData> &objects)
{
    if (!IsInitialized)
    {
        return;
    }

    PhysXCubeDatas.clear();

    Objects = &objects;

    PhysXCubeDatas.resize(Objects->size());
    for (uiw index = 0, size = Objects->size(); index < size; ++index)
    {
        auto &data = PhysXCubeDatas[index];
        auto &object = Objects->operator[](index);

        auto position = object.position;
        auto rotation = object.rotation;
        auto halfSize = object.size * 0.5f;

        data.actor = Physics->createRigidDynamic(PxTransform(position.x, position.y, position.z, PxQuat{rotation.x, rotation.y, rotation.z, rotation.w}));
        data.shape = data.actor->createShape(PxBoxGeometry(halfSize, halfSize, halfSize), *PhysXMaterial);
        PxRigidBodyExt::updateMassAndInertia(*data.actor, 1.0f);

        PhysXScene->addActor(*data.actor);
    }
}