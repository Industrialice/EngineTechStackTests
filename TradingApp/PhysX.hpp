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

	struct ObjectData
	{
		Quaternion rotation;
		Vector3 position;
		Vector3 impulse;
		f32 size;
	};

    bool Create();
    void Destroy();
    void Update();
    void Draw(const EngineCore::Camera &camera);
	void ClearObjects();
    void AddObjects(vector<ObjectData> &cubes, vector<ObjectData> &spheres);
    pair<const ContactInfo *, uiw> GetNewContacts();
}