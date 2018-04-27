#pragma once

namespace EngineCore
{
    class Camera;
}

namespace TradingApp
{
	namespace Scene
	{
		bool Create();
        void Destroy();
		void Update();
		void Draw(const EngineCore::Camera &camera);
	}
}