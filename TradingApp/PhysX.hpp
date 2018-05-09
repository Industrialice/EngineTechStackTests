#pragma once

#include "CubesInstanced.hpp"
#include "SpheresInstanced.hpp"
#include <MatrixMathTypes.hpp>

namespace TradingApp::PhysX
{
    struct ContactInfo
    {
        Vector3 position;
        f32 impulse;
    };

    bool Create();
    void Destroy();
    void Update();
    void Draw(const EngineCore::Camera &camera);
    void SetObjects(vector<CubesInstanced::InstanceData> &cubes, vector<SpheresInstanced::InstanceData> &spheres);
    pair<const ContactInfo *, uiw> GetNewContacts();
}