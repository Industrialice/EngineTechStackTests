#pragma once

#include "CubesInstanced.hpp"
#include "SpheresInstanced.hpp"

namespace TradingApp::PhysX
{
    bool Create();
    void Destroy();
    void Update();
    void Draw(const EngineCore::Camera &camera);
    void SetObjects(vector<CubesInstanced::InstanceData> &cubes, vector<SpheresInstanced::InstanceData> &spheres);
}