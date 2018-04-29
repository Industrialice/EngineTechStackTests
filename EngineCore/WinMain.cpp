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
static void LogRecipient(LogLevel logLevel, string_view nullTerminatedText);
static void FileLogRecipient(FILE *file, LogLevel logLevel, string_view nullTerminatedText);
static bool CreateApplicationSubsystems();

namespace
{
	KeyController::ListenerHandle KeysListener;
	Logger::ListenerHandle LogListener;

	class FileAndCallback
	{
        unique_ptr<FILE, void(*)(FILE *)> file{nullptr, [](FILE *) {}};
		Logger::ListenerHandle handle;

    public:
        FileAndCallback() = default;
		FileAndCallback(decltype(file) &&file, decltype(handle) &&callback) : file(move(file)), handle(move(callback))
		{}
        FileAndCallback(FileAndCallback &&) = default;
        FileAndCallback &operator = (FileAndCallback &&) = default;
	};

	FileAndCallback FileLogListener;

    ui32 SceneRestartedCounter;
}

template <typename _Ty> struct MyCoroutineReturn
{
	struct promise_type
	{
		_Ty value;

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
			this->value = value;
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

	MyCoroutineReturn &operator = (MyCoroutineReturn &&_Right)
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
		return _Coro.promise().value;
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
			if (key->key == vkeyt::Escape)
			{
				PostQuitMessage(0);
			}

			//SENDLOG(Info, "%i key action %i state %i\n", number, key->key, key->keyState);
		}
		else if (auto mouseMove = std::get_if<ControlAction::MouseMove>(&action.action))
		{
            if (mouseMove->deltaY)
            {
                Application::GetMainCamera()->RotateAroundRightAxis(mouseMove->deltaY * 0.001f);
            }
            if (mouseMove->deltaX)
            {
                Application::GetMainCamera()->RotateAroundUpAxis(mouseMove->deltaX * 0.001f);
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
	Application::Create();

    LogListener = Application::GetLogger().AddListener(LogRecipient);

	FILE *logFile = fopen("log.txt", "w");
	if (logFile)
	{
		auto closeFile = [](FILE *file)
		{
			if (file)
			{
				fclose(file);
			}
		};

		auto uniqueFile = std::remove_const_t<decltype(FileAndCallback::file)>(logFile, closeFile);

		FileLogListener = FileAndCallback{
            move(uniqueFile),
            Application::GetLogger().AddListener(std::bind(FileLogRecipient, logFile, std::placeholders::_1, std::placeholders::_2))};
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
	AppWindow appWindow;
	appWindow.fullscreen = false;
	appWindow.height = 1000;
	appWindow.width = 1000;
	appWindow.x = 1000;
	appWindow.y = 1000;
	appWindow.title = "EngineCore";
	appWindow.hideBorders = false;

    if (appWindow.height <= 0) appWindow.height = GetSystemMetrics(SM_CYSCREEN);
    if (appWindow.width <= 0) appWindow.width = GetSystemMetrics(SM_CXSCREEN);

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

    KeysListener = Application::GetKeyController().AddListener(listenerLambda);

    auto oglRenderer = OGLRenderer::OpenGLRenderer::New(appWindow.fullscreen, TextureDataFormat::R8G8B8X8, 24, 8, {GetDC(*systemWindow)});
	if (oglRenderer == nullptr)
	{
		SENDLOG(Critical, "Failed to initialize OpenGL\n");
		return false;
	}
    Application::SetRenderer(oglRenderer);

    auto renderTarget = RenderTarget::New();
	Application::GetMainCamera()->RenderTarget(renderTarget);
    Application::GetMainCamera()->Position(Vector3(0, 5, -10));

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
	auto mainWindow = Application::GetMainWindow();

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
    chrono::time_point<chrono::steady_clock> lastUpdate = chrono::steady_clock::now();
    chrono::time_point<chrono::steady_clock> firstUpdate = chrono::steady_clock::now();

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

            auto currentMemont = chrono::steady_clock::now();

            chrono::duration<f64> durationSinceStart = currentMemont - firstUpdate;
            chrono::duration<f32> durationSinceLastUpdate = currentMemont - lastUpdate;

            EngineTime engineTime;
            engineTime.timeScale = 1.0f;
            engineTime.secondsSinceStart = durationSinceStart.count();
            engineTime.secondSinceLastFrame = durationSinceLastUpdate.count();
            engineTime.unscaledSecondSinceLastFrame = durationSinceLastUpdate.count();

            lastUpdate = currentMemont;

            Application::SetEngineTime(engineTime);

            Application::GetRenderer().BeginFrame();

			const auto &camera = Application::GetMainCamera();
			camera->ClearColorRGBA(array<f32, 4>{0, 0, 0, 1});
            camera->ClearDepthValue(1.0f);

            float camMovementScale = engineTime.secondSinceLastFrame * 5;
            if (Application::GetKeyController().GetKeyInfo(vkeyt::LShift).keyState != ControlAction::Key::KeyStateType::Released)
            {
                camMovementScale *= 3;
            }
            if (Application::GetKeyController().GetKeyInfo(vkeyt::LControl).keyState != ControlAction::Key::KeyStateType::Released)
            {
                camMovementScale *= 0.33f;
            }

            if (Application::GetKeyController().GetKeyInfo(vkeyt::S).keyState != ControlAction::Key::KeyStateType::Released)
            {
                camera->MoveAlongForwardAxis(-camMovementScale);
            }
            if (Application::GetKeyController().GetKeyInfo(vkeyt::W).keyState != ControlAction::Key::KeyStateType::Released)
            {
                camera->MoveAlongForwardAxis(camMovementScale);
            }
            if (Application::GetKeyController().GetKeyInfo(vkeyt::A).keyState != ControlAction::Key::KeyStateType::Released)
            {
                camera->MoveAlongRightAxis(-camMovementScale);
            }
            if (Application::GetKeyController().GetKeyInfo(vkeyt::D).keyState != ControlAction::Key::KeyStateType::Released)
            {
                camera->MoveAlongRightAxis(camMovementScale);
            }
            if (Application::GetKeyController().GetKeyInfo(vkeyt::R).keyState != ControlAction::Key::KeyStateType::Released)
            {
                camera->MoveAlongUpAxis(camMovementScale);
            }
            if (Application::GetKeyController().GetKeyInfo(vkeyt::F).keyState != ControlAction::Key::KeyStateType::Released)
            {
                camera->MoveAlongUpAxis(-camMovementScale);
            }

            if (ui32 newCounter = Application::GetKeyController().GetKeyInfo(vkeyt::Space).timesKeyStateChanged; newCounter != SceneRestartedCounter)
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
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;  //  means that we need to redraw the whole window if its size changes, not just a new portion, CS_OWNDC is required by OpenGL
	wc.lpfnWndProc = MsgProc;  //  note that the message procedure runs in the same thread that created the window( it's a requirement )
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
		windowData = (WindowData *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);;
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

static const char *LogLevelToTag(LogLevel logLevel)
{
	switch (logLevel)
	{
	case LogLevel::Critical:
		return "[crt] ";
	case LogLevel::Debug:
		return "[dbg] ";
	case LogLevel::Error:
		return "[err] ";
	case LogLevel::ImportantInfo:
		return "[imp] ";
	case LogLevel::Info:
		return "[inf] ";
	case LogLevel::Other:
		return "[oth] ";
	case LogLevel::Warning:
		return "[wrn] ";
	}

	UNREACHABLE;
	return nullptr;
}

void LogRecipient(LogLevel logLevel, string_view nullTerminatedText)
{
    if (logLevel == LogLevel::Critical || logLevel == LogLevel::Debug || logLevel == LogLevel::Error) // TODO: cancel breaking
    {
        SOFTBREAK;
    }

	if (logLevel == LogLevel::Critical/* || logLevel == LogLevel::Debug || logLevel == LogLevel::Error*/)
	{
		const char *tag = nullptr;
		switch (logLevel) // fill out all the cases just in case
		{
		case LogLevel::Critical:
			tag = "CRITICAL";
			break;
		case LogLevel::Debug:
			tag = "DEBUG";
			break;
		case LogLevel::Error:
			tag = "ERROR";
			break;
		case LogLevel::ImportantInfo:
			tag = "IMPORTANT INFO";
			break;
		case LogLevel::Info:
			tag = "INFO";
			break;
		case LogLevel::Other:
			tag = "OTHER";
			break;
		case LogLevel::Warning:
			tag = "WARNING";
			break;
		}

		MessageBoxA(0, nullTerminatedText.data(), tag, 0);
		return;
	}

	OutputDebugStringA(LogLevelToTag(logLevel));
	OutputDebugStringA(nullTerminatedText.data());
}

void FileLogRecipient(FILE *file, LogLevel logLevel, string_view nullTerminatedText)
{
	const char *tag = LogLevelToTag(logLevel);
	fwrite(tag, 1, strlen(tag), file);
	fwrite(nullTerminatedText.data(), 1, nullTerminatedText.size(), file);
}