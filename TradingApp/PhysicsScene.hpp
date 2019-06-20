#pragma once

namespace EngineCore
{
    class Camera;
}

namespace TradingApp
{
    namespace PhysicsScene
    {
        bool Create();
        void Destroy();
        void Update(Vector3 mainCameraPosition, Vector3 mainCameraRotation);
        void Restart();
        void Draw(const EngineCore::Camera &camera);
    }
}