#include "BasicHeader.hpp"
#include "Application.hpp"
#include "Renderer.hpp"
#include "Logger.hpp"
#include "Camera.hpp"
#include "ShadersManager.hpp"

#ifdef WINPLATFORM
#include "WinHIDInput.hpp"
#include "WinVKInput.hpp"
#endif

using namespace EngineCore;

struct ApplicationData
{
    Logger logger{};
    shared_ptr<KeyController> keyController{};
    AppWindow mainWindow{};
    shared_ptr<Renderer> renderer{};
    shared_ptr<Camera> mainCamera{};
    EngineTime CurrentEngineTime{};

#ifdef WINPLATFORM
    unique_ptr<HIDInput> hid{};
    unique_ptr<VKInput> vkInput{};
#endif
};

namespace
{
    // don't use unique_ptr because it seems to perform
    // auto temp = ptr;
    // ptr = nullptr;
    // delete temp;
    // which isn't gonna work because during delete
    // you might get log messages for example
    // which would call Instance->logger and still
    // must acquire a valid pointer (because Logger
    // hasn't been deleted yet)
    ApplicationData *Instance{};
}

namespace EngineCore::Application
{
    void Create()
    {
        assert(Instance == nullptr);
        Instance = new ApplicationData;
        Instance->mainCamera = Camera::New();
        Instance->keyController = KeyController::New();
    }

    void Destroy()
    {
        assert(Instance != nullptr);

    #ifdef WINPLATFORM
        assert(Instance->mainWindow.hwnd == 0); // make sure you destroyed the current window
    #endif

        Instance->renderer.reset();

        delete Instance;
        Instance = nullptr;
    }

    void SetEngineTime(EngineTime time)
    {
        Instance->CurrentEngineTime = time;
    }
}

auto Application::GetKeyController() -> KeyController &
{
    return *Instance->keyController;
}

auto Application::GetMainWindow() -> const AppWindow &
{
    return Instance->mainWindow;
}

void Application::SetMainWindow(const AppWindow &window)
{
    Instance->mainWindow = window;
}

auto Application::GetRenderer() -> Renderer &
{
    return *Instance->renderer;
}

void Application::SetRenderer(const shared_ptr<Renderer> &renderer)
{
    Instance->renderer = renderer;
}

auto Application::GetLogger() -> Logger &
{
    return Instance->logger;
}

const shared_ptr<Camera> &Application::GetMainCamera()
{
    return Instance->mainCamera;
}

EngineTime Application::GetEngineTime()
{
    return Instance->CurrentEngineTime;
}

auto Application::LoadResourceShader(string_view name) -> shared_ptr<Shader>
{
    return ShadersManager::FindShaderByName(name);
}

#ifdef WINPLATFORM
auto Application::GetHIDInput() -> HIDInput *
{
    return Instance->hid.get();
}

void Application::SetHIDInput(unique_ptr<HIDInput> hid)
{
    Instance->hid = move(hid);
}

auto Application::GetVKInput() -> VKInput *
{
    return Instance->vkInput.get();
}

void Application::SetVKInput(unique_ptr<VKInput> input)
{
    Instance->vkInput = move(input);
}
#endif