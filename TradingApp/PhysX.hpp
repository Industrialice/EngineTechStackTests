#pragma once

#include "CubesInstanced.hpp"

namespace TradingApp::PhysX
{
    bool Create();
    void Destroy();
    void Update();
    void FinishUpdate();
    void SetObjects(vector<CubesInstanced::InstanceData> &objects);
}