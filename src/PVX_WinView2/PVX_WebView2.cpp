#include <windows.h>
#include <windowsx.h>

#include <wil/com.h>
#include <wrl.h>
#include <dcomp.h>
#include <WebView2.h>
#include <queue>
#include <functional>
#include <mutex>
#include "..\..\include\PVX_WebView2.h"

#include <shellscalingapi.h>   // SetProcessDpiAwareness
#pragma comment(lib, "Shcore.lib")

#define WM_RUN_TASKS (WM_APP)
#define WM_RUN_CREATE (WM_APP+1)

namespace PVX::WebView{
	namespace {
		void SetDpiAwarenessPerMonitorV2() {
			// Best effort, try newest first
			using SetCtxFn = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
			if (auto user32 = ::GetModuleHandleW(L"user32.dll")) {
				auto pSetCtx = reinterpret_cast<SetCtxFn>(::GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
				if (pSetCtx && pSetCtx(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) return;
			}
			// Fallbacks for older builds
			::SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE); // may fail pre-8.1
			::SetProcessDPIAware();                                   // last resort (Vista+)
		}

		//Microsoft::WRL::ComPtr<ICoreWebView2Environment> environment;
		Microsoft::WRL::ComPtr<ICoreWebView2Environment3> environment3;

		struct internalPanel {
			HWND hWnd;
			double scale = 1.0f;

			Microsoft::WRL::ComPtr<ICoreWebView2CompositionController> controller3;
			Microsoft::WRL::ComPtr<ICoreWebView2Controller2> controller2;
			Microsoft::WRL::ComPtr<ICoreWebView2Controller> controller;
			Microsoft::WRL::ComPtr<ICoreWebView2> view;

			Microsoft::WRL::ComPtr<IDCompositionDevice>		  dcompDevice;
			Microsoft::WRL::ComPtr<IDCompositionTarget>		  dcompTarget;
			Microsoft::WRL::ComPtr<IDCompositionVisual>		  rootVisual;
			Microsoft::WRL::ComPtr<IDCompositionVisual>		  webviewContainer;

			std::mutex lockQueue;
			std::queue<std::function<void()>> Messages;
		};

		std::unordered_map<HWND, std::unique_ptr<internalPanel>> panelStore;
		std::queue<std::pair<HWND, std::function<void()>>> CreationQueue;

		void PVX_PostMessage(internalPanel* p, std::function<void()> msg) {
			{
				std::lock_guard<std::mutex> lock{ p->lockQueue };
				p->Messages.push(std::move(msg));
			}
			PostMessage(p->hWnd, WM_RUN_TASKS, 0, 0);
		}
	};

	View::View(void* ptr):ptr{ptr} {}
	void View::NavigateTo(std::wstring url) {
		auto p = ((internalPanel*)ptr);
		PVX_PostMessage(p, [p, url]() {
			p->view->Navigate(url.c_str());
		});
	}
	void View::Reload() {
		auto p = ((internalPanel*)ptr);
		PVX_PostMessage(p, [p]() {
			p->view->Reload();
		});
	}

	HRESULT InitDComp(HWND hwnd, internalPanel& panel) {
		HRESULT hr = DCompositionCreateDevice(nullptr, __uuidof(IDCompositionDevice), reinterpret_cast<void**>(panel.dcompDevice.GetAddressOf()));

		if (FAILED(hr)) return hr;

		hr = panel.dcompDevice->CreateTargetForHwnd(hwnd, /*topmost*/ TRUE, &panel.dcompTarget);
		if (FAILED(hr)) return hr;

		hr = panel.dcompDevice->CreateVisual(&panel.rootVisual);
		if (FAILED(hr)) return hr;

		hr = panel.dcompTarget->SetRoot(panel.rootVisual.Get());
		if (FAILED(hr)) return hr;

		// Our container that WebView2 will render into
		hr = panel.dcompDevice->CreateVisual(&panel.webviewContainer);
		if (FAILED(hr)) return hr;

		// Put container under root (z-order relative to siblings here)
		hr = panel.rootVisual->AddVisual(panel.webviewContainer.Get(), FALSE, nullptr);
		if (FAILED(hr)) return hr;

		return panel.dcompDevice->Commit();
	}

// Helpers
	static COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS BuildMouseVirtualKeys() {
		auto vk = COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_NONE;

		if (GetKeyState(VK_SHIFT) & 0x8000) vk = (decltype(vk))(vk | COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_SHIFT);
		if (GetKeyState(VK_CONTROL) & 0x8000) vk = (decltype(vk))(vk | COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_CONTROL);

		if (GetKeyState(VK_LBUTTON) & 0x8000) vk = (decltype(vk))(vk | COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_LEFT_BUTTON);
		if (GetKeyState(VK_RBUTTON) & 0x8000) vk = (decltype(vk))(vk | COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_RIGHT_BUTTON);
		if (GetKeyState(VK_MBUTTON) & 0x8000) vk = (decltype(vk))(vk | COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_MIDDLE_BUTTON);
		if (GetKeyState(VK_XBUTTON1) & 0x8000) vk = (decltype(vk))(vk | COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_X_BUTTON1);
		if (GetKeyState(VK_XBUTTON2) & 0x8000) vk = (decltype(vk))(vk | COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_X_BUTTON2);

		return vk;
	}

	static POINT ToWebViewPoint(HWND hwnd, LPARAM lParam, float dpiScale, int panelX, int panelY) {
		POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }; // client logical coords
		// Offset to panel origin, then to device pixels (WebView expects pixels)
		pt.x = LONG((pt.x - panelX) * dpiScale);
		pt.y = LONG((pt.y - panelY) * dpiScale);
		return pt;
	}

	static COREWEBVIEW2_PHYSICAL_KEY_STATUS MakeKeyStatus(LPARAM lParam) {
		COREWEBVIEW2_PHYSICAL_KEY_STATUS ks{};
		ks.RepeatCount = (UINT)(lParam & 0xFFFF);
		ks.ScanCode = (UINT)((lParam >> 16) & 0xFF);
		ks.IsExtendedKey = (lParam & (1 << 24)) != 0;
		ks.IsMenuKeyDown = (GetKeyState(VK_MENU) & 0x8000) != 0;
		ks.WasKeyDown = (lParam & (1 << 30)) != 0;
		ks.IsKeyReleased = (lParam & (1u << 31)) != 0;
		return ks;
	}

	static HRESULT ForwardMouseMessageToWebView(ICoreWebView2CompositionController* comp, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, float dpiScale, int panelX, int panelY) {
		if (!comp) return 1;

		COREWEBVIEW2_MOUSE_EVENT_KIND kind = COREWEBVIEW2_MOUSE_EVENT_KIND(msg);
		UINT mouseData = GET_WHEEL_DELTA_WPARAM(wParam);
		POINT pt = ToWebViewPoint(hwnd, lParam, dpiScale, panelX, panelY);
		return comp->SendMouseInput(kind, BuildMouseVirtualKeys(), mouseData, pt);
	}


//	static HRESULT ForwardMouseMessageToWebView(ICoreWebView2CompositionController* comp, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, float dpiScale, int panelX, int panelY) {
//		if (!comp) return 1;
//
//		COREWEBVIEW2_MOUSE_EVENT_KIND kind{};
//		UINT mouseData = 0;
//		POINT pt{};
//
//		switch (msg) {
//		case WM_MOUSEMOVE:
//			kind = COREWEBVIEW2_MOUSE_EVENT_KIND_MOVE;
//			pt = ToWebViewPoint(hwnd, lParam, dpiScale, panelX, panelY);
//			break;
//
//		case WM_LBUTTONDOWN: kind = COREWEBVIEW2_MOUSE_EVENT_KIND_LEFT_BUTTON_DOWN;  pt = ToWebViewPoint(hwnd, lParam, dpiScale, panelX, panelY); break;
//		case WM_LBUTTONUP:   kind = COREWEBVIEW2_MOUSE_EVENT_KIND_LEFT_BUTTON_UP;	pt = ToWebViewPoint(hwnd, lParam, dpiScale, panelX, panelY); break;
//		case WM_RBUTTONDOWN: kind = COREWEBVIEW2_MOUSE_EVENT_KIND_RIGHT_BUTTON_DOWN; pt = ToWebViewPoint(hwnd, lParam, dpiScale, panelX, panelY); break;
//		case WM_RBUTTONUP:   kind = COREWEBVIEW2_MOUSE_EVENT_KIND_RIGHT_BUTTON_UP;   pt = ToWebViewPoint(hwnd, lParam, dpiScale, panelX, panelY); break;
//		case WM_MBUTTONDOWN: kind = COREWEBVIEW2_MOUSE_EVENT_KIND_MIDDLE_BUTTON_DOWN; pt = ToWebViewPoint(hwnd, lParam, dpiScale, panelX, panelY); break;
//		case WM_MBUTTONUP:   kind = COREWEBVIEW2_MOUSE_EVENT_KIND_MIDDLE_BUTTON_UP;  pt = ToWebViewPoint(hwnd, lParam, dpiScale, panelX, panelY); break;
//
//		case WM_MOUSEWHEEL:
//		{
//			POINT spt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }; ScreenToClient(hwnd, &spt);
//			LPARAM lp = MAKELPARAM(spt.x, spt.y);
//			kind = COREWEBVIEW2_MOUSE_EVENT_KIND_WHEEL;
//			mouseData = GET_WHEEL_DELTA_WPARAM(wParam); // +/-120 steps
//			pt = ToWebViewPoint(hwnd, lp, dpiScale, panelX, panelY);
//			break;
//		}
//		case WM_MOUSEHWHEEL:
//		{
//			POINT spt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }; ScreenToClient(hwnd, &spt);
//			LPARAM lp = MAKELPARAM(spt.x, spt.y);
//			kind = COREWEBVIEW2_MOUSE_EVENT_KIND_HORIZONTAL_WHEEL;
//			mouseData = GET_WHEEL_DELTA_WPARAM(wParam);
//			pt = ToWebViewPoint(hwnd, lp, dpiScale, panelX, panelY);
//			break;
//		}
//
//		case WM_MOUSELEAVE:
//			kind = COREWEBVIEW2_MOUSE_EVENT_KIND_LEAVE;
//			pt = POINT{ 0, 0 }; // not used
//			break;
//
//		default:
//			return 1; // not a mouse message we forward
//		}
//
//		return comp->SendMouseInput(kind, BuildMouseVirtualKeys(), mouseData, pt);
//	}

	//void EnsureEnvironment() {
	//	if (environment) return;
	//	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	//	if (auto ev = CreateEvent(nullptr, true, false, nullptr); ev) {
	//		CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr, Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>([ev](HRESULT hrEnv, ICoreWebView2Environment* env) {
	//			//env->QueryInterface<ICoreWebView2Environment>(environment.ReleaseAndGetAddressOf());
	//			environment = env;
	//			SetEvent(ev);
	//			return S_OK;
	//		}).Get());
	//		WaitForSingleObject(ev, INFINITE);
	//		CloseHandle(ev);
	//	}
	//}
	void View::OnWebMessageReceived(std::function<void(std::wstring msg)> clb) {
		Creation([this, clb] {
			EventRegistrationToken token{};
			(*(internalPanel*)ptr).view->add_WebMessageReceived(Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>([clb](ICoreWebView2* /*sender*/, ICoreWebView2WebMessageReceivedEventArgs* args) {
				wil::unique_cotaskmem_string json;
				if (SUCCEEDED(args->get_WebMessageAsJson(&json)) && json) {
					clb(json.get());
				}
				return S_OK;
			}).Get(), &token);
		});
	}

	void EnsureEnvironment3() {
		if (environment3) return;

		SetDpiAwarenessPerMonitorV2();

		HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		if (auto ev = CreateEvent(nullptr, true, false, nullptr); ev) {
			CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr, Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>([ev](HRESULT hrEnv, ICoreWebView2Environment* env) {
				env->QueryInterface<ICoreWebView2Environment3>(environment3.ReleaseAndGetAddressOf());
				SetEvent(ev);
				return S_OK;
			}).Get());
			WaitForSingleObject(ev, INFINITE);
			CloseHandle(ev);
		}
	}

	int DoCreation(HWND, UINT, WPARAM, LPARAM) {
		auto f = std::move(CreationQueue.front());
		CreationQueue.pop();
		f.second();
		return 0;
	}

	void View::Creation(std::function<void()> fnc) {
		auto p = (internalPanel*)ptr;
		auto sz = CreationQueue.size();
		CreationQueue.push({ p->hWnd, fnc });
		if(!sz) PostMessage(CreationQueue.front().first, WM_RUN_CREATE, 0, 0);
	}

	View CreateComposition(PVX::Windows::Window& win, std::wstring url) {
		if (panelStore.find(win.Handle()) != panelStore.end()) return nullptr;
		auto& panel = panelStore[win.Handle()] = std::make_unique<internalPanel>();
		panel->hWnd = win.Handle();
		win.Override(WM_RUN_CREATE, DoCreation);

		EnsureEnvironment3();
		auto ret = View{ panel.get() };
		ret.Creation([&panel, &win, url] {
			environment3->CreateCoreWebView2CompositionController(win.Handle(), Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2CompositionControllerCompletedHandler>([&panel, &win, url](HRESULT hrCtl, ICoreWebView2CompositionController* ctl) -> HRESULT {
				if (FAILED(hrCtl)) return hrCtl;
				panel->controller3 = ctl;
				panel->controller3.As(&panel->controller);
				panel->controller3.As(&panel->controller2);
				panel->controller->get_CoreWebView2(panel->view.ReleaseAndGetAddressOf());
				{
					win.OnCloseHWND([](HWND hWnd) {
						panelStore.erase(hWnd);
					});
					win.OnResizeClientRC([&panel](const RECT& rc) {
						panel->controller->put_Bounds(rc);
						panel->dcompDevice->Commit();
					});
					win.OnMove([&panel](int x, int y) {
						panel->controller->NotifyParentWindowPositionChanged();
					});
					win.OnMoving([&panel](RECT& rc) {
						panel->controller->NotifyParentWindowPositionChanged();
						return 0;
					});
					win.Override(WM_DPICHANGED, [&panel](HWND hWnd, UINT, WPARAM w, LPARAM) {
						Microsoft::WRL::ComPtr<ICoreWebView2Controller3> cc2;
						if (SUCCEEDED(panel->controller3.As(&cc2)))
							cc2->put_RasterizationScale(panel->scale = HIWORD(w) / 96.0);
						return 0;
					});

					win.Override({ WM_NCLBUTTONDOWN, WM_NCRBUTTONDOWN, WM_NCMBUTTONDOWN }, [&panel](HWND, UINT, WPARAM w, LPARAM) {
						if (w == HTCAPTION) {
							panel->controller->put_IsVisible(FALSE);
							panel->controller->put_IsVisible(TRUE);
							//panel->controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
						}
						return -1;
					});

					win.Override(WM_RUN_TASKS, [&panel](HWND, UINT, WPARAM, LPARAM) {
						for (;;) {
							std::function<void()> f;
							{
								std::lock_guard lk(panel->lockQueue);
								if (panel->Messages.empty()) break;
								f = std::move(panel->Messages.front()); panel->Messages.pop();
							}
							f();
						}
						return 0;
					});

					InitDComp(win.Handle(), *panel);
					panel->controller3->put_RootVisualTarget(panel->webviewContainer.Get());

					win.Override(WM_SETFOCUS, [&panel](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
						if (panel && panel->controller) panel->controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
						return 0;
					});

					//win.Override({ WM_LBUTTONDOWN, WM_RBUTTONDOWN, WM_MBUTTONDOWN }, [&panel](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
					//	auto y = GET_Y_LPARAM(lParam);
					//	return 0;
					//});

					win.Override({ WM_MOUSEMOVE, WM_LBUTTONDOWN, WM_LBUTTONUP, WM_RBUTTONDOWN, WM_RBUTTONUP, WM_MBUTTONDOWN, WM_MBUTTONUP, WM_MOUSEWHEEL, WM_MOUSEHWHEEL, WM_MOUSELEAVE }, [&panel](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
						ForwardMouseMessageToWebView(panel->controller3.Get(), hWnd, msg, wParam, lParam, panel->scale, 0, 0);
						return 0;
					});
				}

				panel->controller2->put_DefaultBackgroundColor({ 0, 0, 0, 0 });
				{
					Microsoft::WRL::ComPtr<ICoreWebView2Controller3> cc2;
					if (SUCCEEDED(panel->controller3.As(&cc2)))
						cc2->put_RasterizationScale(panel->scale = GetDpiForWindow(win.Handle()) / 96.0);
				}

				RECT rc{}; GetClientRect(win.Handle(), &rc);
				panel->controller->put_Bounds(rc);
				panel->dcompDevice->Commit();

				panel->view->Navigate(url.c_str());
				if (!CreationQueue.empty()) PostMessage(CreationQueue.front().first, WM_RUN_CREATE, 0, 0);
				return S_OK;
			}).Get());
		});
		return ret;
	};

	View Create(PVX::Windows::Window& win, std::wstring url) {
		if (panelStore.find(win.Handle()) != panelStore.end()) return nullptr;
		auto& panel = panelStore[win.Handle()] = std::make_unique<internalPanel>();
		panel->hWnd = win.Handle();

		win.Override(WM_RUN_CREATE, DoCreation);

		EnsureEnvironment3();
		auto ret = View{ panel.get() };
		ret.Creation([&panel, &win, url] {
			environment3->CreateCoreWebView2Controller(win.Handle(), Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>([&panel, &win, url](HRESULT hrCtl, ICoreWebView2Controller* ctl) -> HRESULT {
				//panel->controller3 = ctl;
				//panel->controller3.As(&panel->controller);
				panel->controller = ctl;
				panel->controller->get_CoreWebView2(panel->view.ReleaseAndGetAddressOf());

				win.OnCloseHWND([](HWND hWnd) { panelStore.erase(hWnd); });
				win.OnResizeClientRC([&panel](const RECT& rc) {
					panel->controller->put_Bounds(rc);
				});
				win.OnMove([&panel](int x, int y) {
					panel->controller->NotifyParentWindowPositionChanged();
				});

				win.Override(WM_RUN_TASKS, [&panel](HWND, UINT, WPARAM, LPARAM) {
					for (;;) {
						std::function<void()> f;
						{
							std::lock_guard lk(panel->lockQueue);
							if (panel->Messages.empty()) break;
							f = std::move(panel->Messages.front()); panel->Messages.pop();
						}
						f();
					}
					return 0;
				});

				//InitDComp(win.Handle(), *panel);

				RECT rc{}; GetClientRect(win.Handle(), &rc);
				panel->controller->put_Bounds(rc);

				panel->view->Navigate(url.c_str());
				//SetEvent(ev);
				if (!CreationQueue.empty()) PostMessage(CreationQueue.front().first, WM_RUN_CREATE, 0, 0);
				return S_OK;
			}).Get());
		});
		//CreationQueue.push({ win.Handle(), });
		//if(CreationQueue.front().first==win.Handle())
		//	PostMessage(CreationQueue.front().first, WM_RUN_CREATE, 0, 0);
		//while (WaitForSingleObject(ev, 10) != WAIT_OBJECT_0 && PVX::Windows::Eventer::DoEvents());
		//CloseHandle(ev);
		return panel.get();
	}
}

//inline HRESULT InitWebView2ForHwnd(HWND hwnd,
//	Microsoft::WRL::ComPtr<ICoreWebView2Controller>& outController,
//	Microsoft::WRL::ComPtr<ICoreWebView2>& outWebView) {
//// Ensure COM for the thread (simple STA is fine for WebView2)
//	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
//	if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return hr;
//
//	// Create the WebView2 runtime environment (Evergreen)
//	return CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,  Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>([hwnd, &outController, &outWebView](HRESULT hrEnv, ICoreWebView2Environment* env) -> HRESULT {
//		if (FAILED(hrEnv)) return hrEnv;
//		// Create a controller bound to our parent window (this gives a child HWND “panel”)
//		return env->CreateCoreWebView2Controller(hwnd, Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>([&outController, &outWebView, hwnd](HRESULT hrCtl, ICoreWebView2Controller* ctl) -> HRESULT {
//			if (FAILED(hrCtl)) return hrCtl;
//			outController = ctl;
//			outController->get_CoreWebView2(outWebView.ReleaseAndGetAddressOf());
//
//			// Size it to the client area
//			RECT rc{}; GetClientRect(hwnd, &rc);
//			outController->put_Bounds(rc);
//
//			// First pixel
//			outWebView->Navigate(L"about:blank");
//			return S_OK;
//		}).Get());
//	}).Get());
//}