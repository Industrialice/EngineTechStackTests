#pragma once

namespace EngineCore
{
	class KeyController;
	class Renderer;
	class Logger;
	class HIDInput;
	class VKInput;
	class Camera;
	class Shader;

	struct AppWindow
	{
		#ifdef WINPLATFORM
		HWND hwnd = 0;
		#endif

		i32 width = 0, height = 0;
		i32 x = 0, y = 0;
        string title{};
		bool fullscreen = false;
		bool hideBorders = true;
	};

    struct EngineTime
    {
        f64 secondsSinceStart{};
        f32 secondSinceLastFrame{};
        f32 unscaledSecondSinceLastFrame{};
        f32 timeScale{};
    };

	// this is a god object, it supposed to contain all highest level systems
	namespace Application
	{
		KeyController &GetKeyController();
		const AppWindow &GetMainWindow();
		void SetMainWindow(const AppWindow &window);
		Renderer &GetRenderer();
        void SetRenderer(const shared_ptr<Renderer> &renderer);
		Logger &GetLogger();
		const shared_ptr<Camera> &GetMainCamera();
        EngineTime GetEngineTime();

		shared_ptr<Shader> LoadResourceShader(string_view name);

		template <typename Resource, typename = enable_if_t<std::is_same_v<Resource, Shader>>> shared_ptr<Shader> LoadResource(string_view name) { return LoadResourceShader(name); }

		#ifdef WINPLATFORM
		HIDInput *GetHIDInput();
		void SetHIDInput(unique_ptr<HIDInput> hid);
		VKInput *GetVKInput();
		void SetVKInput(unique_ptr<VKInput> input);
		#endif
	};
}

#ifdef DEBUG
    // this snprintf will trigger compiler warnings if incorrect args were passed in
    #define SENDLOG(level, ...) snprintf(nullptr, 0, __VA_ARGS__); EngineCore::Application::GetLogger().Message(EngineCore::LogLevel:: level, __VA_ARGS__)
#else
    #define SENDLOG(level, ...) EngineCore::Application::GetLogger().Message(EngineCore::LogLevel:: level, __VA_ARGS__)
#endif