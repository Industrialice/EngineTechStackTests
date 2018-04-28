#include "PreHeader.hpp"
#include "PhysicsCube.hpp"
#include "Cube.hpp"

using namespace EngineCore;
using namespace TradingApp;

namespace
{
    unique_ptr<Cube> TestCube;
}

PhysicsCube::PhysicsCube(const Vector3 &position, const EngineCore::Quaternion &rotation, f32 size) : _position(position), _rotation(rotation), _size(size)
{
    if (!TestCube)
    {
        TestCube = make_unique<Cube>();
    }
}

void PhysicsCube::Update(const EngineCore::Vector3 &position, const EngineCore::Quaternion &rotation)
{
    _position = position;
    _rotation = rotation;
}

void PhysicsCube::Draw(const Camera &camera)
{
    TestCube->Draw(&camera, _position, _rotation, _size);
}
