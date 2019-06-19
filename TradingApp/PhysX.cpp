#include "PreHeader.hpp"
#include "PhysX.hpp"
#include <Application.hpp>
#include <Logger.hpp>
#include <MathFunctions.hpp>
#include <Renderer.hpp>

//#define ENABLE_CONTACT_NOTIFICATIONS

#ifdef _WIN64
	#pragma comment(lib, "PhysXFoundation_64.lib")
	#pragma comment(lib, "PhysXCooking_64.lib")
	#pragma comment(lib, "PhysXExtensions_static_64.lib")
	#pragma comment(lib, "PhysXPvdSDK_static_64.lib")
	#pragma comment(lib, "PhysXCommon_64.lib")
	#pragma comment(lib, "PhysX_64.lib")
#else
	#pragma comment(lib, "PhysXFoundation_32.lib")
	#pragma comment(lib, "PhysXCooking_32.lib")
	#pragma comment(lib, "PhysXExtensions_static_32.lib")
	#pragma comment(lib, "PhysXPvdSDK_static_32.lib")
	#pragma comment(lib, "PhysXCommon_32.lib")
	#pragma comment(lib, "PhysX_32.lib")
#endif

using namespace EngineCore;
using namespace TradingApp;
using namespace physx;

#define PVD_HOST "127.0.0.1" // Set this to the IP address of the system running the PhysX Visual Debugger that you want to connect to.

enum class ProcessingOn { CPU, GPU };
enum class BroadPhaseType { SAP, MBP, GPU };
static bool SetSceneProcessing(PxSceneDesc &desc, ProcessingOn processingOn, BroadPhaseType broadPhaseType);

class SimulationCallback : public PxSimulationEventCallback
{
public:
    virtual void onConstraintBreak(PxConstraintInfo *constraints, PxU32 count) override;
    virtual void onWake(PxActor **actors, PxU32 count) override;
    virtual void onSleep(PxActor **actors, PxU32 count) override;
    virtual void onTrigger(PxTriggerPair* pairs, PxU32 count) override;
    virtual void onContact(const PxContactPairHeader &pairHeader, const PxContactPair *pairs, PxU32 numPairs) override;
    virtual void onAdvance(const PxRigidBody *const *bodyBuffer, const PxTransform *poseBuffer, const PxU32 count) override;
};

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
        bool isWoke = true;
        ui32 contactsCount = 0;
        f32 size{};
        PxRigidDynamic *actor{};
        PxShape *shape{};

        ~PhysXActorData()
        {
            if (PhysXScene && actor && shape)
            {
                PhysXScene->removeActor(*actor);
                actor->detachShape(*shape);
                //shape->release();
                actor->release();
            }
        }
    };
    vector<PhysXActorData> PhysXCubeDatas{}, PhysXSphereDatas{};
    PhysXActorData PhysXPlaneData{};

    unique_ptr<CubesInstanced> InstancedCubes{};
    unique_ptr<SpheresInstanced> InstancedSpheres{};

    ui32 SimulationMemorySize = 16384 * 64; // 1024 KB
    unique_ptr<ui8, void(*)(void *p)> SimulationMemory = {(ui8 *)_aligned_malloc(SimulationMemorySize, 16), [](void *p) { _aligned_free(p); }};

    SimulationCallback SimCallback{};

    vector<PhysX::ContactInfo> NewContactInfos{};
}

bool PhysX::Create()
{
    assert(!IsInitialized);

    Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, DefaultAllocator, DefaultErrorCallback);
    if (!Foundation)
    {
        SENDLOG(Error, "PhysX::Create -> PxCreateFoundation failed\n");
        return false;
    }

    //Pvd = PxCreatePvd(*Foundation);
    //PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
    //Pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

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

    PxSceneLimits sceneLimits;
    sceneLimits.maxNbActors = 10'000;
    sceneLimits.maxNbBodies = 10'000;
    sceneLimits.maxNbDynamicShapes = 10'000;

    if (!SetSceneProcessing(sceneDesc, ProcessingOn::GPU, BroadPhaseType::GPU))
    {
        SENDLOG(Error, "PhysX::Create -> SetSceneProcessing failed\n");
        return false;
    }

    auto filterShader = [](PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, PxPairFlags& pairFlags, const void *constantBlock, PxU32 constantBlockSize) -> PxFilterFlags
    {
		pairFlags = PxPairFlag::eCONTACT_DEFAULT;
		#ifdef ENABLE_CONTACT_NOTIFICATIONS
			pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_TOUCH_LOST | PxPairFlag::eNOTIFY_CONTACT_POINTS;
		#endif
        return PxFilterFlag::eDEFAULT;
    };
    
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = CpuDispatcher;
    //sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    sceneDesc.filterShader = filterShader;
    sceneDesc.simulationEventCallback = &SimCallback;
    sceneDesc.limits = sceneLimits;
    sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;
    sceneDesc.dynamicTreeRebuildRateHint = 100;
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
        PxBroadPhaseRegion bpregion;
        bpregion.bounds = bounds[i];
        bpregion.userData = reinterpret_cast<void *>(i);
        PhysXScene->addBroadPhaseRegion(bpregion);
    }

    PxPvdSceneClient* pvdClient = PhysXScene->getScenePvdClient();
    if (pvdClient)
    {
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
    }

    PhysXMaterial = Physics->createMaterial(0.75f, 0.5f, 0.25f);

    GroundPlane = PxCreatePlane(*Physics, PxPlane(0, 1, 0, 1), *PhysXMaterial);
    PhysXScene->addActor(*GroundPlane);
    GroundPlane->userData = &PhysXPlaneData;

    IsInitialized = true;

    return true;
}

void PhysX::Destroy()
{
    PhysXPlaneData = {};
    PhysXCubeDatas = {};
    PhysXSphereDatas = {};

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

    NewContactInfos.clear();

    PhysXScene->simulate(Application::GetEngineTime().secondSinceLastFrame, nullptr, SimulationMemory.get(), SimulationMemorySize);
}

void PhysX::Draw(const Camera &camera)
{
    if (!IsInitialized)
    {
        return;
    }

    auto genericDraw = [](const Camera &camera, const auto &source, const auto &instancedObject)
    {
        if (source.size() && instancedObject)
        {
            auto *lock = instancedObject->Lock(source.size());
            for (const auto &data : source)
            {
                const auto &phyPos = data.actor->getGlobalPoseWithoutActor();

                lock->position = {phyPos.p.x, phyPos.p.y, phyPos.p.z};
                lock->rotation = {phyPos.q.x, phyPos.q.y, phyPos.q.z, phyPos.q.w};
                f32 size = data.size;
                if (!data.isWoke)
                {
                    size = Funcs::SetBit(size, 31, 1);
                }
                //size = Funcs::SetBit(size, 0, data.contactsCount > 0);
                lock->size = size;

                ++lock;
            }
            instancedObject->Unlock();
            instancedObject->Draw(&camera, (ui32)source.size());
        }
    };

    genericDraw(camera, PhysXCubeDatas, InstancedCubes.get());
    genericDraw(camera, PhysXSphereDatas, InstancedSpheres.get());

    PhysXScene->fetchResults(true);
}

void PhysX::SetObjects(vector<CubesInstanced::InstanceData> &cubes, vector<SpheresInstanced::InstanceData> &spheres)
{
    if (!IsInitialized)
    {
        return;
    }

    NewContactInfos.clear();
    PhysXCubeDatas.clear();
    PhysXSphereDatas.clear();

    if (!InstancedCubes)
    {
        InstancedCubes = make_unique<CubesInstanced>(cubes.size());
    }

    static constexpr f32 contactOffset = 0.0075f;
    static constexpr f32 restOffset = 0.0f;
    static constexpr f32 sleepThreshold = 0.01f;
    static constexpr f32 wakeCounter = 0.2f;

    PxShape *defaultCubeShape = Physics->createShape(PxBoxGeometry(0.5f, 0.5f, 0.5f), *PhysXMaterial, false, PxShapeFlag::eSIMULATION_SHAPE);
	defaultCubeShape->setContactOffset(contactOffset);
	defaultCubeShape->setRestOffset(restOffset);

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
        data.actor->setSleepThreshold(sleepThreshold);
        data.actor->setWakeCounter(wakeCounter);
        if (Distance(0.5f, halfSize) < DefaultF32Epsilon)
        {
            data.actor->attachShape(*defaultCubeShape);
            data.shape = defaultCubeShape;
        }
        else
        {
			data.shape = Physics->createShape(PxBoxGeometry(halfSize, halfSize, halfSize), *PhysXMaterial, PxShapeFlag::eSIMULATION_SHAPE);
			data.actor->attachShape(*data.shape);
			data.shape->setContactOffset(contactOffset);
			data.shape->setRestOffset(restOffset);
        }

        PxRigidBodyExt::updateMassAndInertia(*data.actor, 1.0f);

        data.actor->userData = &data;
        data.actor->setActorFlag(PxActorFlag::eSEND_SLEEP_NOTIFIES, true);
        PhysXScene->addActor(*data.actor);
        data.actor->wakeUp();
    }

    defaultCubeShape->release();

    if (!InstancedSpheres)
    {
        InstancedSpheres = make_unique<SpheresInstanced>(10, 7, spheres.size());
    }

    PxShape *defaultSphereShape = Physics->createShape(PxSphereGeometry(0.5f), *PhysXMaterial, false, PxShapeFlag::eSIMULATION_SHAPE);
	defaultSphereShape->setContactOffset(contactOffset);
	defaultSphereShape->setRestOffset(restOffset);

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
        if (Distance(0.5f, halfSize) < DefaultF32Epsilon)
        {
            data.actor->attachShape(*defaultSphereShape);
            data.shape = defaultSphereShape;
        }
        else
        {
			data.shape = Physics->createShape(PxSphereGeometry(halfSize), *PhysXMaterial, PxShapeFlag::eSIMULATION_SHAPE);
			data.actor->attachShape(*data.shape);
			data.shape->setContactOffset(contactOffset);
			data.shape->setRestOffset(restOffset);
        }

        PxRigidBodyExt::updateMassAndInertia(*data.actor, 1.0f);

        data.actor->userData = &data;
        data.actor->setActorFlag(PxActorFlag::eSEND_SLEEP_NOTIFIES, true);
        PhysXScene->addActor(*data.actor);
        data.actor->wakeUp();
    }

    defaultSphereShape->release();
}

auto PhysX::GetNewContacts() -> pair<const ContactInfo *, uiw>
{
    return {NewContactInfos.data(), NewContactInfos.size()};
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
        mc.constraintBufferCapacity *= 3;
        mc.contactBufferCapacity *= 3;
        mc.tempBufferCapacity *= 3;
        mc.contactStreamSize *= 3;
        mc.patchStreamSize *= 3;
        mc.forceStreamCapacity *= 8;
		mc.heapCapacity *= 3;
        mc.foundLostPairsCapacity *= 3;
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
        desc.cudaContextManager = CudaContexManager;
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
            desc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
        } break;
        case ProcessingOn::CPU:
        {
        } break;
    }

    return true;
}

void SimulationCallback::onConstraintBreak(PxConstraintInfo *constraints, PxU32 count)
{}

void SimulationCallback::onWake(PxActor **actors, PxU32 count)
{
    for (PxU32 index = 0; index < count; ++index)
    {
        PxActor *actor = actors[index];
        PhysXActorData &data = *(PhysXActorData *)actor->userData;
        data.isWoke = true;
    }
}

void SimulationCallback::onSleep(PxActor **actors, PxU32 count)
{
    for (PxU32 index = 0; index < count; ++index)
    {
        PxActor *actor = actors[index];
        PhysXActorData &data = *(PhysXActorData *)actor->userData;
        data.isWoke = false;
    }
}

void SimulationCallback::onTrigger(PxTriggerPair *pairs, PxU32 count)
{}

void SimulationCallback::onContact(const PxContactPairHeader &pairHeader, const PxContactPair *pairs, PxU32 numPairs)
{
    if (pairHeader.flags & (PxContactPairHeaderFlag::eREMOVED_ACTOR_0 | PxContactPairHeaderFlag::eREMOVED_ACTOR_1))
    {
        return;
    }

    PxRigidActor *actor0 = pairHeader.actors[0];
    PxRigidActor *actor1 = pairHeader.actors[1];

    auto &actor0data = *(PhysXActorData *)actor0->userData;
    auto &actor1data = *(PhysXActorData *)actor1->userData;

    auto getNextContactPairPoint = [](const PxContactPair &pair, PxContactStreamIterator &iter, uiw &count) -> optional<PxContactPairPoint>
    {
        const PxU32 flippedContacts = (pair.flags & PxContactPairFlag::eINTERNAL_CONTACTS_ARE_FLIPPED);
        const PxU32 hasImpulses = (pair.flags & PxContactPairFlag::eINTERNAL_HAS_IMPULSES);

        if (iter.hasNextPatch())
        {
            iter.nextPatch();
            if (iter.hasNextContact())
            {
                iter.nextContact();
                PxContactPairPoint dst;
                dst.position = iter.getContactPoint();
                dst.separation = iter.getSeparation();
                dst.normal = iter.getContactNormal();
                if (!flippedContacts)
                {
                    dst.internalFaceIndex0 = iter.getFaceIndex0();
                    dst.internalFaceIndex1 = iter.getFaceIndex1();
                }
                else
                {
                    dst.internalFaceIndex0 = iter.getFaceIndex1();
                    dst.internalFaceIndex1 = iter.getFaceIndex0();
                }

                if (hasImpulses)
                {
                    const PxReal impulse = pair.contactImpulses[count];
                    dst.impulse = dst.normal * impulse;
                }
                else
                {
                    dst.impulse = PxVec3(0.0f);
                }

                ++count;

                return dst;
            }
        }

        return {};
    };

    for (PxU32 pairIndex = 0; pairIndex < numPairs; ++pairIndex)
    {
        const auto &pair = pairs[pairIndex];

        if (pair.events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
        {
            if (actor0data.actor && actor1data.actor)
            {
                uiw count = 0;
                PxContactStreamIterator iter(pair.contactPatches, pair.contactPoints, pair.getInternalFaceIndices(), pair.patchCount, pair.contactCount);
                for (auto contactPoint = getNextContactPairPoint(pair, iter, count); contactPoint; contactPoint = getNextContactPairPoint(pair, iter, count))
                {
                    ++actor0data.contactsCount;
                    ++actor1data.contactsCount;

					#ifdef USE_XAUDIO
						NewContactInfos.push_back({Vector3{contactPoint->position.x, contactPoint->position.y, contactPoint->position.z}, contactPoint->impulse.magnitude()});
					#endif
                }
            }
        }
        else if (pair.events & PxPairFlag::eNOTIFY_TOUCH_LOST)
        {
            if (actor0data.actor && actor1data.actor)
            {
                uiw count = 0;
                PxContactStreamIterator iter(pair.contactPatches, pair.contactPoints, pair.getInternalFaceIndices(), pair.patchCount, pair.contactCount);
                for (auto contactPoint = getNextContactPairPoint(pair, iter, count); contactPoint; contactPoint = getNextContactPairPoint(pair, iter, count))
                {
                    assert(actor0data.contactsCount && actor1data.contactsCount);
                    --actor0data.contactsCount;
                    --actor1data.contactsCount;
                }
            }
        }
    }
}

void SimulationCallback::onAdvance(const PxRigidBody *const *bodyBuffer, const PxTransform *poseBuffer, const PxU32 count)
{}
