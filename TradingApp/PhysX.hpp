#pragma once

#include "PhysicsCube.hpp"

namespace TradingApp::PhysX
{
    bool Create();
    void Destroy();
    void Update();
    void FinishUpdate();
    void SetObjects(vector<PhysicsCube> &objects);
}