#pragma once

#include <MatrixMathTypes.hpp>

namespace EngineCore
{
    class Camera;
}

namespace TradingApp
{
    class PhysicsCube
    {
    protected:
        EngineCore::Vector3 _position{};
        EngineCore::Vector3 _rotation{};
        f32 _size{};

    public:
        PhysicsCube() = default;
        PhysicsCube(const EngineCore::Vector3 &position, const EngineCore::Vector3 &rotation, f32 size);
        void Update(const EngineCore::Vector3 &position, const EngineCore::Vector3 &rotation);
        void Draw(const EngineCore::Camera &camera);
    };
}