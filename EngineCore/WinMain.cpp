#include "BasicHeader.hpp"
#include "Application.hpp"
#include "Logger.hpp"
#include <..\TradingApp\Scene.hpp>
#include <..\TradingApp\PhysicsScene.hpp>
#include "WinHIDInput.hpp"
#include "WinVKInput.hpp"
#include "Camera.hpp"
#include "RenderTarget.hpp"
#include <OpenGLRenderer.hpp>
#include "KeyController.hpp"
#include "RecordingKeyController.hpp"

using namespace EngineCore;

namespace SceneToDraw = TradingApp::PhysicsScene;

namespace EngineCore::Application
{
	void Create();
	void Destroy();
    void SetEngineTime(EngineTime time);
}

struct WindowData
{
	HWND hwnd;
	ControlsQueue controlsQueue;
};

static optional<HWND> CreateSystemWindow(bool isFullscreen, const string &title, HINSTANCE instance, bool hideBorders, RECT &dimensions);
static LRESULT WINAPI MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void MessageLoop();
static void ShutdownApp();
static void LogRecipient(LogLevels::LogLevel logLevel, string_view nullTerminatedText);
static void FileLogRecipient(File &file, LogLevels::LogLevel logLevel, string_view nullTerminatedText);
static bool CreateApplicationSubsystems();

namespace
{
    IKeyController::ListenerHandle KeysListener{};
    StdLib::Logger<void, false>::ListenerHandle LogListener{};
    StdLib::Logger<void, false>::ListenerHandle FileLogListener{};
    File LogFile{};
    vector<ControlAction> AllReceivedControlActions{};
    IKeyController::ListenerHandle ControlActionRecordingListener{};

    ui32 SceneRestartedCounter{};
    ui32 UpTimeDeltaCounter{}, DownTimeDeltaCounter{};
}

template <typename _Ty> struct MyCoroutineReturn
{
	struct promise_type
	{
		_Ty _value;

		promise_type &get_return_object()
		{
			return *this;
		}

		bool initial_suspend()
		{
			return true;
		}

		bool final_suspend()
		{
			return false;
		}

		void yield_value(_Ty const &value)
		{
            _value = value;
		}
	};

	explicit MyCoroutineReturn(promise_type &_Prom) : _Coro(std::experimental::coroutine_handle<promise_type>::from_promise(_Prom))
	{
	}

	MyCoroutineReturn() = default;

	MyCoroutineReturn(MyCoroutineReturn &&_Right) : _Coro(_Right._Coro)
	{
		_Right._Coro = nullptr;
	}

	MyCoroutineReturn &operator = (MyCoroutineReturn &&_Right) noexcept
	{
		assert(this != std::addressof(_Right));
		_Coro = _Right._Coro;
		_Right._Coro = nullptr;
		return *this;
	}

	~MyCoroutineReturn()
	{
		if (_Coro)
		{
			_Coro.destroy();
		}
	}

	const _Ty &get() const
	{
		return _Coro.promise()._value;
	}

	auto &Handle()
	{
		return _Coro;
	}

private:
	std::experimental::coroutine_handle<promise_type> _Coro = nullptr;
};

static MyCoroutineReturn<bool> ListenKeys(const ControlAction &action)
{
	int number = 0;

	for (;;)
	{
		if (auto key = std::get_if<ControlAction::Key>(&action.action))
		{
			if (key->key == KeyCode::Escape)
			{
				PostQuitMessage(0);
			}

			//SENDLOG(Info, "%i key action %i state %i\n", number, key->key, key->keyState);
		}
		else if (auto mouseMove = std::get_if<ControlAction::MouseMove>(&action.action))
		{
            if (mouseMove->delta.y)
            {
                Application::GetMainCamera()->RotateAroundRightAxis(mouseMove->delta.y * 0.001f);
            }
            if (mouseMove->delta.x)
            {
                Application::GetMainCamera()->RotateAroundUpAxis(mouseMove->delta.x * 0.001f);
            }

			//SENDLOG(Info, "%i mouse move action %i %i\n", number, mouseMove->deltaX, mouseMove->deltaY);
		}
		else if (auto mouseWheel = std::get_if<ControlAction::MouseWheel>(&action.action))
		{
            Application::GetMainCamera()->MoveAlongUpAxis(-mouseWheel->delta * 0.5f);
			//SENDLOG(Info, "%i mouse wheel action %i\n", number, mouseWheel->delta);
		}
		else
		{
			SENDLOG(Info, "%i unknown action\n", number);
		}

		number += 1;

		co_yield false;
	}
}

int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    StdLib::Initialization::Initialize({});

	Application::Create();

    LogListener = Application::GetLogger().OnMessage(LogRecipient);

    auto recordingController = RecordingKeyController::New(KeyController::New());
    ControlActionRecordingListener = recordingController->OnRecordingControlAction(
        [](const ControlAction &action) -> bool
        {
            AllReceivedControlActions.push_back(action);
            return false;
        });
    Application::SetKeyController(recordingController);

    LogFile = File(FilePath::FromChar("log.txt"), FileOpenMode::CreateAlways, FileProcModes::Write);
	if (LogFile.IsOpen())
	{
		FileLogListener = Application::GetLogger().OnMessage(std::bind(FileLogRecipient, std::ref(LogFile), std::placeholders::_1, std::placeholders::_2));
	}

	SENDLOG(Info, "Log started\n");

	if (CreateApplicationSubsystems() == false)
	{
		ShutdownApp();
		return 1;
	}

	MessageLoop();

	ShutdownApp();

	return 0;
}

bool CreateApplicationSubsystems()
{
    static constexpr bool isFullScreened = false;

	AppWindow appWindow;
	appWindow.fullscreen = isFullScreened;
	appWindow.height = isFullScreened ? 0 : 1000;
	appWindow.width = isFullScreened ? 0 : 1000;
	appWindow.x = isFullScreened ? 0 : 1000;
	appWindow.y = isFullScreened ? 0 : 1000;
	appWindow.title = "EngineCore";
	appWindow.hideBorders = isFullScreened;

	if (appWindow.width <= 0) appWindow.width = 1280;
    if (appWindow.height <= 0) appWindow.height = 800;

	RECT windowRect{ appWindow.x, appWindow.y, appWindow.x + appWindow.width, appWindow.y + appWindow.height };

	auto systemWindow = CreateSystemWindow(appWindow.fullscreen, appWindow.title, GetModuleHandleW(NULL), appWindow.hideBorders, windowRect);
	if (systemWindow == nullopt)
	{
		SENDLOG(Critical, "Failed to create a systemWindow\n");
		return false;
	}

	appWindow.x = windowRect.left;
	appWindow.y = windowRect.top;
	appWindow.width = windowRect.right - windowRect.left;
	appWindow.height = windowRect.bottom - windowRect.top;
	appWindow.hwnd = *systemWindow;

	Application::SetMainWindow(appWindow);

	static WindowData windowData;
	windowData.hwnd = *systemWindow;
	SetWindowLongPtrA(*systemWindow, GWLP_USERDATA, (LONG_PTR)&windowData);

	auto hid = make_unique<HIDInput>();
	if (hid->Register(appWindow.hwnd) == true)
	{
        SENDLOG(Info, "Using HID input\n");
		Application::SetHIDInput(move(hid));
	}
	else
	{
		SENDLOG(Error, "Failed to register HID input, using VK input\n");
		Application::SetVKInput(make_unique<VKInput>());
	}

	auto coroutine = make_shared<MyCoroutineReturn<bool>>();
	ControlAction action;
	auto listenerLambda = [action, coroutine](const ControlAction &newAction) mutable -> bool // TODO: fix it, it could be rewritten with at least std::bind
	{
		if (!coroutine->Handle())
		{
			*coroutine = ListenKeys(action);
		}
		action = newAction;
		coroutine->Handle().resume();
		return coroutine->get();
	};

    KeysListener = Application::GetKeyController().OnControlAction(listenerLambda, DeviceTypes::_AllDevices);

    auto oglRenderer = OGLRenderer::OpenGLRenderer::New(appWindow.fullscreen, TextureDataFormat::R8G8B8X8, 24, 8, {GetDC(*systemWindow)});
	if (oglRenderer == nullptr)
	{
		SENDLOG(Critical, "Failed to initialize OpenGL\n");
		return false;
	}
    Application::SetRenderer(oglRenderer);

    EngineTime engineTime;
    //engineTime.timeScale = 0.5f;
    Application::SetEngineTime(engineTime);

    auto renderTarget = RenderTarget::New();
	Application::GetMainCamera()->RenderTarget(renderTarget);
    Application::GetMainCamera()->Position(Vector3(0, 25, -25));

    if (SceneToDraw::Create() == false)
	{
		SENDLOG(Critical, "Failed to create the scene\n");
		return false;
	}

	SENDLOG(Info, "Application subsystems're created\n");
	return true;
}

void ShutdownApp()
{
    SceneToDraw::Destroy();

	auto mainWindow = Application::GetMainWindow();

	if (mainWindow.fullscreen)
	{
		if (ChangeDisplaySettingsA(nullptr, 0) != DISP_CHANGE_SUCCESSFUL)
		{
			SENDLOG(Error, "Failed to exit fullscreen mode\n");
		}
	}

    Application::SetRenderer(nullptr);

	if (mainWindow.hwnd != NULL)
	{
		DestroyWindow(mainWindow.hwnd);
		mainWindow.hwnd = NULL;
	}
	Application::SetMainWindow(mainWindow);

	Application::Destroy();
}

void MessageLoop()
{
	MSG o_msg;
    auto lastUpdate = TimeMoment::Now();
    auto firstUpdate = TimeMoment::Now();

	do
	{
		if (PeekMessageA(&o_msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&o_msg);
			DispatchMessageA(&o_msg);
		}
		else
		{
			WindowData *windowData = (WindowData *)GetWindowLongPtrA(Application::GetMainWindow().hwnd, GWLP_USERDATA);
			if (windowData != nullptr)
			{
				Application::GetKeyController().Dispatch(windowData->controlsQueue.Enumerate());
				windowData->controlsQueue.clear();
			}
			else
			{
				HARDBREAK;
			}

            auto currentMemont = TimeMoment::Now();

            TimeDifference durationSinceStart = currentMemont - firstUpdate;
            TimeDifference durationSinceLastUpdate = currentMemont - lastUpdate;

            EngineTime engineTime = Application::GetEngineTime();
            engineTime.secondsSinceStart = durationSinceStart.ToSec_f64() * engineTime.timeScale;
            engineTime.secondSinceLastFrame = durationSinceLastUpdate.ToSec() * engineTime.timeScale;
            engineTime.unscaledSecondSinceLastFrame = durationSinceLastUpdate.ToSec();

            lastUpdate = currentMemont;

            Application::SetEngineTime(engineTime);

            Application::GetRenderer().BeginFrame();

			const auto &camera = Application::GetMainCamera();
			camera->ClearColorRGBA(array<f32, 4>{0, 0, 0, 1});
            camera->ClearDepthValue(1.0f);

            const auto &keyController = Application::GetKeyController();

            float camMovementScale = engineTime.unscaledSecondSinceLastFrame * 5;
            if (keyController.GetKeyInfo(KeyCode::LShift).IsPressed())
            {
                camMovementScale *= 3;
            }
            if (keyController.GetKeyInfo(KeyCode::LControl).IsPressed())
            {
                camMovementScale *= 0.33f;
            }

            if (keyController.GetKeyInfo(KeyCode::S).IsPressed())
            {
                camera->MoveAlongForwardAxis(-camMovementScale);
            }
            if (keyController.GetKeyInfo(KeyCode::W).IsPressed())
            {
                camera->MoveAlongForwardAxis(camMovementScale);
            }
            if (keyController.GetKeyInfo(KeyCode::A).IsPressed())
            {
                camera->MoveAlongRightAxis(-camMovementScale);
            }
            if (keyController.GetKeyInfo(KeyCode::D).IsPressed())
            {
                camera->MoveAlongRightAxis(camMovementScale);
            }
            if (keyController.GetKeyInfo(KeyCode::R).IsPressed())
            {
                camera->MoveAlongUpAxis(camMovementScale);
            }
            if (keyController.GetKeyInfo(KeyCode::F).IsPressed())
            {
                camera->MoveAlongUpAxis(-camMovementScale);
            }

            if (keyController.GetKeyInfo(KeyCode::LAlt).IsPressed())
            {
                while (UpTimeDeltaCounter != keyController.GetKeyInfo(KeyCode::UpArrow).timesKeyStateChanged)
                {
                    SENDLOG(Info, "SENT\n");
                    ++UpTimeDeltaCounter;
                    engineTime.timeScale += 0.05f;
                    Application::SetEngineTime(engineTime);
                }
                while (DownTimeDeltaCounter != keyController.GetKeyInfo(KeyCode::DownArrow).timesKeyStateChanged)
                {
                    SENDLOG(Info, "SENT\n");
                    ++DownTimeDeltaCounter;
                    engineTime.timeScale -= 0.05f;
                    engineTime.timeScale = std::max(engineTime.timeScale, 0.1f);
                    Application::SetEngineTime(engineTime);
                }
            }

            if (ui32 newCounter = keyController.GetKeyInfo(KeyCode::Space).timesKeyStateChanged; newCounter != SceneRestartedCounter)
            {
                SceneRestartedCounter = newCounter;
                SceneToDraw::Restart();
            }
            
            Application::GetRenderer().ClearCameraTargets(camera.get());

            SceneToDraw::Update();
            SceneToDraw::Draw(*camera);

            Application::GetRenderer().SwapBuffers();

            Application::GetRenderer().EndFrame();
		}
	} while (o_msg.message != WM_QUIT);
}

optional<HWND> CreateSystemWindow(bool isFullscreen, const string &title, HINSTANCE instance, bool hideBorders, RECT &dimensions)
{
	auto className = "window_" + title;

	WNDCLASSA wc;
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // means that we need to redraw the whole window if its size changes, not just a new portion, CS_OWNDC is required by OpenGL
	wc.lpfnWndProc = MsgProc; // note that the message procedure runs in the same thread that created the window (it's a requirement)
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = instance;
	wc.hIcon = LoadIconA(0, IDI_APPLICATION);
	wc.hCursor = LoadCursorA(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = className.c_str();

	if (!RegisterClassA(&wc))
	{
		SENDLOG(Critical, "failed to register class\n");
		return nullopt;
	}

    ui32 width = dimensions.right - dimensions.left;
    ui32 height = dimensions.bottom - dimensions.top;

    if (isFullscreen)
    {
        DEVMODE dmScreenSettings = {};
        dmScreenSettings.dmSize = sizeof(dmScreenSettings);
        dmScreenSettings.dmPelsWidth = width;
        dmScreenSettings.dmPelsHeight = height;
        dmScreenSettings.dmBitsPerPel = 32;
        dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

        if (ChangeDisplaySettingsA(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
        {
            SENDLOG(Error, "Failed to set fullscreen mode\n");
        }
        else
        {
            SENDLOG(Info, "Display mode has been changed to fullscreen\n");
            ShowCursor(FALSE);
        }
    }

	DWORD style = isFullscreen || hideBorders ? WS_POPUP : WS_SYSMENU;

	AdjustWindowRect(&dimensions, style, false);

	HWND hwnd = CreateWindowA(className.c_str(), title.c_str(), style, dimensions.left, dimensions.top, width, height, 0, 0, instance, 0);
	if (!hwnd)
	{
		SENDLOG(Critical, "failed to create requested window\n");
		return nullopt;
	}

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	return hwnd;
}

LRESULT WINAPI MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WindowData *windowData = nullptr;
	if (hwnd == Application::GetMainWindow().hwnd)
	{
		windowData = (WindowData *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
		if (windowData == nullptr || windowData->hwnd != hwnd)
		{
			SOFTBREAK;
			windowData = nullptr;
		}
	}

	switch (msg)
	{
	case WM_INPUT:
		if (windowData != nullptr && Application::GetHIDInput() != nullptr)
		{
			Application::GetHIDInput()->Dispatch(windowData->controlsQueue, hwnd, wParam, lParam);
			return 0;
		}
		break;
	case WM_SETCURSOR:
	case WM_MOUSEMOVE:
	case WM_KEYDOWN:
    case WM_KEYUP:
		if (windowData != nullptr && Application::GetVKInput() != nullptr)
		{
			Application::GetVKInput()->Dispatch(windowData->controlsQueue, hwnd, msg, wParam, lParam);
			return 0;
		}
		break;
	case WM_ACTIVATE:
		//  switching window's active state
		break;
	case WM_SIZE:
	{
		//window->width = LOWORD( lParam );
		//window->height = HIWORD( lParam );
	} break;
	// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		break;
		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
	case WM_EXITSIZEMOVE:
		break;
		// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
		// somebody has asked our window to close
	case WM_CLOSE:
		break;
		// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		//((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		//((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200; 
		break;
	}

	return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static const char *LogLevelToTag(LogLevels::LogLevel logLevel)
{
	switch (logLevel.AsInteger())
	{
	case LogLevels::Critical.AsInteger():
		return "[crt] ";
	case LogLevels::Debug.AsInteger():
		return "[dbg] ";
	case LogLevels::Error.AsInteger():
		return "[err] ";
	case LogLevels::Attention.AsInteger():
		return "[imp] ";
	case LogLevels::Info.AsInteger():
		return "[inf] ";
	case LogLevels::Other.AsInteger():
		return "[oth] ";
	case LogLevels::Warning.AsInteger():
		return "[wrn] ";
    case LogLevels::_None.AsInteger():
    case LogLevels::_All.AsInteger():
        HARDBREAK;
        return "";
	}

	UNREACHABLE;
	return nullptr;
}

void LogRecipient(LogLevels::LogLevel logLevel, string_view nullTerminatedText)
{
    if (logLevel == LogLevels::Critical || logLevel == LogLevels::Debug || logLevel == LogLevels::Error) // TODO: cancel breaking
    {
        SOFTBREAK;
    }

	if (logLevel == LogLevels::Critical/* || logLevel == LogLevel::Debug || logLevel == LogLevel::Error*/)
	{
		const char *tag = nullptr;
		switch (logLevel.AsInteger()) // fill out all the cases just in case
		{
		case LogLevels::Critical.AsInteger():
			tag = "CRITICAL";
			break;
		case LogLevels::Debug.AsInteger():
			tag = "DEBUG";
			break;
		case LogLevels::Error.AsInteger():
			tag = "ERROR";
			break;
		case LogLevels::Attention.AsInteger():
			tag = "IMPORTANT INFO";
			break;
		case LogLevels::Info.AsInteger():
			tag = "INFO";
			break;
		case LogLevels::Other.AsInteger():
			tag = "OTHER";
			break;
		case LogLevels::Warning.AsInteger():
			tag = "WARNING";
			break;
        case LogLevels::_None.AsInteger():
        case LogLevels::_All.AsInteger():
            HARDBREAK;
            return;
		}

		MessageBoxA(0, nullTerminatedText.data(), tag, 0);
		return;
	}

	OutputDebugStringA(LogLevelToTag(logLevel));
	OutputDebugStringA(nullTerminatedText.data());
}

void FileLogRecipient(File &file, LogLevels::LogLevel logLevel, string_view nullTerminatedText)
{
	const char *tag = LogLevelToTag(logLevel);
    file.Write(tag, (ui32)strlen(tag));
    file.Write(nullTerminatedText.data(), (ui32)nullTerminatedText.size());
}