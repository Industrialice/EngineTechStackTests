#include "PreHeader.hpp"
#include "PhysX.hpp"
#include <PxPhysicsAPI.h>
#include <Application.hpp>
#include <Logger.hpp>
#include <MathFunctions.hpp>
#include <Renderer.hpp>

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

enum class ProcessingOn { CPU, GPU };
enum class BroadPhaseType { SAP, MBP, GPU };
static bool SetSceneProcessing(PxSceneDesc &desc, ProcessingOn processingOn, BroadPhaseType broadPhaseType);

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
    PxCudaContextManager *CudaContexManager{};

    bool IsInitialized = false;

    struct PhysXActorData
    {
        f32 size;
        PxRigidDynamic *actor{};
        PxShape *shape{};

        ~PhysXActorData()
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
    vector<PhysXActorData> PhysXCubeDatas{}, PhysXSphereDatas{};

    unique_ptr<CubesInstanced> InstancedCubes{};
    unique_ptr<SpheresInstanced> InstancedSpheres{};
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

    if (!SetSceneProcessing(sceneDesc, ProcessingOn::CPU, BroadPhaseType::GPU))
    {
        SENDLOG(Error, "PhysX::Create -> SetSceneProcessing failed\n");
        return false;
    }

    auto filterShader = [](PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, PxPairFlags& pairFlags, const void *constantBlock, PxU32 constantBlockSize) -> PxFilterFlags
    {
        pairFlags = PxPairFlag::eCONTACT_DEFAULT;
        return PxFilterFlag::eDEFAULT;
    };

    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = CpuDispatcher;
    //sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    sceneDesc.filterShader = filterShader;
    PhysXScene = Physics->createScene(sceneDesc);
    if (!PhysXScene)
    {
        SENDLOG(Error, "PhysX::Create -> Physics->createScene failed\n");
        return false;
    }

    // required by MBP
    PxBounds3 region;
    region.minimum = {-100.0f, 0.0f, -100.0f};
    region.maximum = {100.0f, 200.0f, 100.0f};
    PxBounds3 bounds[256];
    const PxU32 nbRegions = PxBroadPhaseExt::createRegionsFromWorldBounds(bounds, region, 4);
    for (PxU32 i = 0; i < nbRegions; ++i)
    {
        PxBroadPhaseRegion region;
        region.bounds = bounds[i];
        region.userData = (void *)i;
        PhysXScene->addBroadPhaseRegion(region);
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
    if (CudaContexManager)
    {
        CudaContexManager->release();
        CudaContexManager = nullptr;
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
    if (!IsInitialized)
    {
        return;
    }

    PhysXScene->simulate(Application::GetEngineTime().secondSinceLastFrame);
}

void PhysX::Draw(const EngineCore::Camera &camera)
{
    if (!IsInitialized)
    {
        return;
    }

    if (PhysXCubeDatas.size())
    {
        CubesInstanced::InstanceData *cubeLock = InstancedCubes->Lock(PhysXCubeDatas.size());
        for (const auto &data : PhysXCubeDatas)
        {
            const auto &phyPos = data.actor->getGlobalPose();

            cubeLock->position = {phyPos.p.x, phyPos.p.y, phyPos.p.z};
            cubeLock->rotation = {phyPos.q.x, phyPos.q.y, phyPos.q.z, phyPos.q.w};
            cubeLock->size = data.size;

            ++cubeLock;
        }
        InstancedCubes->Unlock();
        InstancedCubes->Draw(&camera, (ui32)PhysXCubeDatas.size());
    }

    if (PhysXSphereDatas.size())
    {
        SpheresInstanced::InstanceData *sphereLock = InstancedSpheres->Lock(PhysXSphereDatas.size());
        for (const auto &data : PhysXSphereDatas)
        {
            const auto &phyPos = data.actor->getGlobalPose();

            sphereLock->position = {phyPos.p.x, phyPos.p.y, phyPos.p.z};
            sphereLock->rotation = {phyPos.q.x, phyPos.q.y, phyPos.q.z, phyPos.q.w};
            sphereLock->size = data.size;

            ++sphereLock;
        }
        InstancedSpheres->Unlock();
        InstancedSpheres->Draw(&camera, (ui32)PhysXSphereDatas.size());
    }

    PhysXScene->fetchResults(true);
}

void PhysX::SetObjects(vector<CubesInstanced::InstanceData> &cubes, vector<SpheresInstanced::InstanceData> &spheres)
{
    if (!IsInitialized)
    {
        return;
    }

    PhysXCubeDatas.clear();
    PhysXSphereDatas.clear();

    if (!InstancedCubes)
    {
        InstancedCubes = make_unique<CubesInstanced>(cubes.size());
    }

    PxShape *defaultCubeShape = Physics->createShape(PxBoxGeometry(0.5f, 0.5f, 0.5f), *PhysXMaterial, false, PxShapeFlag::eSIMULATION_SHAPE);

    PhysXCubeDatas.resize(cubes.size());
    for (uiw index = 0, size = cubes.size(); index < size; ++index)
    {
        auto &data = PhysXCubeDatas[index];
        auto &object = cubes[index];

        auto position = object.position;
        auto rotation = object.rotation;
        auto halfSize = object.size * 0.5f;

        data.size = object.size;
        data.actor = Physics->createRigidDynamic(PxTransform(position.x, position.y, position.z, PxQuat{rotation.x, rotation.y, rotation.z, rotation.w}));
        if (Distance(0.5f, halfSize) < DefaultEpsilon)
        {
            data.actor->attachShape(*defaultCubeShape);
            data.shape = defaultCubeShape;
        }
        else
        {
            data.shape = data.actor->createShape(PxBoxGeometry(halfSize, halfSize, halfSize), *PhysXMaterial, PxShapeFlag::eSIMULATION_SHAPE);
        }
        PxRigidBodyExt::updateMassAndInertia(*data.actor, 1.0f);

        PhysXScene->addActor(*data.actor);
    }

    defaultCubeShape->release();

    if (!InstancedSpheres)
    {
        InstancedSpheres = make_unique<SpheresInstanced>(10, 7, spheres.size());
    }

    PxShape *defaultSphereShape = Physics->createShape(PxSphereGeometry(0.5f), *PhysXMaterial, false, PxShapeFlag::eSIMULATION_SHAPE);

    PhysXSphereDatas.resize(spheres.size());
    for (uiw index = 0, size = spheres.size(); index < size; ++index)
    {
        auto &data = PhysXSphereDatas[index];
        auto &object = spheres[index];

        auto position = object.position;
        auto rotation = object.rotation;
        auto halfSize = object.size * 0.5f;

        data.size = object.size;
        data.actor = Physics->createRigidDynamic(PxTransform(position.x, position.y, position.z, PxQuat{rotation.x, rotation.y, rotation.z, rotation.w}));
        if (Distance(0.5f, halfSize) < DefaultEpsilon)
        {
            data.actor->attachShape(*defaultSphereShape);
            data.shape = defaultSphereShape;
        }
        else
        {
            data.shape = data.actor->createShape(PxSphereGeometry(halfSize), *PhysXMaterial, PxShapeFlag::eSIMULATION_SHAPE);
        }

        PxRigidBodyExt::updateMassAndInertia(*data.actor, 1.0f);

        PhysXScene->addActor(*data.actor);
    }

    defaultSphereShape->release();
}

bool SetSceneProcessing(PxSceneDesc &desc, ProcessingOn processingOn, BroadPhaseType broadPhaseType)
{
    if (processingOn == ProcessingOn::GPU || broadPhaseType == BroadPhaseType::GPU)
    {
        /*PxU32 constraintBufferCapacity;	//!< Capacity of constraint buffer allocated in GPU global memory
        PxU32 contactBufferCapacity;	//!< Capacity of contact buffer allocated in GPU global memory
        PxU32 tempBufferCapacity;		//!< Capacity of temp buffer allocated in pinned host memory.
        PxU32 contactStreamSize;		//!< Size of contact stream buffer allocated in pinned host memory. This is double-buffered so total allocation size = 2* contactStreamCapacity * sizeof(PxContact).
        PxU32 patchStreamSize;			//!< Size of the contact patch stream buffer allocated in pinned host memory. This is double-buffered so total allocation size = 2 * patchStreamCapacity * sizeof(PxContactPatch).
        PxU32 forceStreamCapacity;		//!< Capacity of force buffer allocated in pinned host memory.
        PxU32 heapCapacity;				//!< Initial capacity of the GPU and pinned host memory heaps. Additional memory will be allocated if more memory is required.
        PxU32 foundLostPairsCapacity;	//!< Capacity of found and lost buffers allocated in GPU global memory. This is used for the found/lost pair reports in the BP. */

        PxgDynamicsMemoryConfig mc;
        /*mc.constraintBufferCapacity *= 4;
        mc.contactBufferCapacity *= 4;
        mc.tempBufferCapacity *= 4;
        mc.contactStreamSize *= 4;
        mc.patchStreamSize *= 4;
        mc.forceStreamCapacity *= 4;
        mc.heapCapacity *= 4;
        mc.foundLostPairsCapacity *= 4;*/
        desc.gpuDynamicsConfig = mc;

        HGLRC hg = wglGetCurrentContext();

        PxCudaContextManagerDesc cudaContextManagerDesc;
        //cudaContextManagerDesc.interopMode = PxCudaInteropMode::OGL_INTEROP;
        //cudaContextManagerDesc.graphicsDevice = Application::GetRenderer().RendererContext();
        //cudaContextManagerDesc.graphicsDevice = hg;
        CudaContexManager = PxCreateCudaContextManager(*Foundation, cudaContextManagerDesc);
        if (!CudaContexManager || !CudaContexManager->contextIsValid())
        {
            SENDLOG(Error, "PhysX::Create -> PxCreateCudaContextManager failed\n");
            return false;
        }
        desc.gpuDispatcher = CudaContexManager->getGpuDispatcher();
    }

    switch (broadPhaseType)
    {
        case BroadPhaseType::MBP:
        {
            desc.broadPhaseType = PxBroadPhaseType::eMBP;
        } break;
        case BroadPhaseType::SAP:
        {
            desc.broadPhaseType = PxBroadPhaseType::eSAP;
        } break;
        case BroadPhaseType::GPU:
        {
            desc.broadPhaseType = PxBroadPhaseType::eGPU;
        } break;
    }
    
    switch (processingOn)
    {
        case ProcessingOn::GPU:
        {
            desc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS | PxSceneFlag::eENABLE_PCM;
        } break;
        case ProcessingOn::CPU:
        {
        } break;
    }

    return true;
}