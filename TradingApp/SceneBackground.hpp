#pragma once

namespace EngineCore
{
    class Camera;
    struct Vector3;
}

namespace TradingApp
{
	namespace SceneBackground
	{
		bool Create(bool isDepthWriteEnabled);
        void Destroy();
		void Update();
		void Draw(const EngineCore::Vector3 &rotation, const EngineCore::Camera &camera);
	}
}