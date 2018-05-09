#pragma once

namespace EngineCore
{
    class Camera;
}

namespace TradingApp
{
	namespace SceneBackground
	{
		bool Create(bool isDepthWriteEnabled);
        void Destroy();
		void Update();
		void Draw(const Vector3 &rotation, const EngineCore::Camera &camera);
	}
}