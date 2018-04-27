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
        void Update();
        void Restart();
        void Draw(const EngineCore::Camera &camera);
    }
}