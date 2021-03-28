#include <PVX_Window.h>
#include <map>
#include <functional>
#include <Windows.h>
#include <windowsx.h>
#include <WinUser.h>
#include <thread>
#include <chrono>
#include "..\..\include\PVX_Window.h"

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC         ((USHORT) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE        ((USHORT) 0x02)
#endif
#ifndef HID_USAGE_GENERIC_KEYBOARD
#define HID_USAGE_GENERIC_KEYBOARD        ((USHORT) 0x06)
#endif


namespace PVX::Windows {
	namespace {
		static int WindowCount = 0;
		std::thread EventThread;

		struct WindowPrivate {
			HWND hWnd = 0;
			unsigned int Flags = 0;
			std::map<unsigned int, std::vector<std::function<LRESULT(HWND, WPARAM, LPARAM)>>> Events;
			std::map<WPARAM, std::vector<std::function<void(LPARAM)>>> Command;
		};
		INT_PTR PVX_DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
			WindowPrivate& ov = *(WindowPrivate*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (&ov) {
				if (message==WM_COMMAND && !ov.Command.empty()) {
					if (auto ev = ov.Command.find(wParam); ev != ov.Command.end()) {
						for(auto& evl : ev->second)
							evl(lParam);
						if (!(ov.Flags && 1)) return 0;
					}
				}
				if (!ov.Events.empty()) {
					if (auto ev = ov.Events.find(message); ev != ov.Events.end()) {
						for (auto& evl : ev->second)
							if (auto res = evl(hWnd, wParam, lParam))
								return res;
						return 0;
					}
				}
			}
			switch (message) {
				case WM_DESTROY:
					WindowCount--;
					break;
			}
			return 0;
		}
		LRESULT CALLBACK PVX_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
			WindowPrivate& ov = *(WindowPrivate*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

			if (&ov) {
				if (message==WM_COMMAND && !ov.Command.empty()) {
					if (auto ev = ov.Command.find(wParam); ev != ov.Command.end()) {
						for (auto& evl : ev->second)
							evl(lParam);
						if (!(ov.Flags && 1)) return 0;
					}
				}
				if (!ov.Events.empty()) {
					if (auto ev = ov.Events.find(message); ev != ov.Events.end()) {
						for (auto& evl : ev->second)
							if (auto res = evl(hWnd, wParam, lParam))
								return res;
						return 0;
					}
				}
			}

			switch (message) {
				case WM_DESTROY:
					WindowCount--;
					//PostQuitMessage(0);
					break;
				default:
					return DefWindowProc(hWnd, message, wParam, lParam);
			}
			return 0;
		}
	}

	static int Inited;

	Eventer::Eventer(HWND hWnd) {
		WindowPrivate* win = new WindowPrivate();
		WindowData = (void*)win;
		WindowCount++;
		win->hWnd = hWnd;
		SetWindowLongPtr(win->hWnd, GWLP_USERDATA, (LONG_PTR)win);
		SetWindowLongPtr(win->hWnd, GWLP_WNDPROC, (LONG_PTR)PVX_WndProc);
	}
	Eventer::~Eventer() {
		delete ((WindowPrivate*)WindowData);
	}


	Window::Window(int DialogResource) {
		WindowPrivate* win = new WindowPrivate();
		WindowData = (void*)win;
		WindowCount++;
		win->hWnd = CreateDialogParamW(GetModuleHandle(NULL), MAKEINTRESOURCEW(DialogResource), GetDesktopWindow(), PVX_DlgProc, 0);
		SetWindowLongPtr(win->hWnd, GWLP_USERDATA, (LONG_PTR)win);
	}

	void Window::Init(int w, int h, const wchar_t* className) {
		WindowCount++;
		WindowPrivate* win = new WindowPrivate();
		WindowData = (void*)win;
		if (!Inited) {
			WNDCLASSEXW wcex = { 0 };
			wcex.cbSize = sizeof(WNDCLASSEX);
			wcex.lpfnWndProc = PVX_WndProc;
			wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
			wcex.lpszClassName = L"PVX::Windows::Window";
			RegisterClassExW(&wcex);
			Inited = 1;
		}
		RECT rc = { 0, 0, w, h };
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
		win->hWnd = CreateWindowW(L"PVX::Windows::Window", className, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
			rc.right - rc.left, rc.bottom - rc.top, GetDesktopWindow(), NULL,
			GetModuleHandle(NULL), NULL);

		SetWindowLongPtr(win->hWnd, GWLP_USERDATA, (LONG_PTR)win);
	}

	Window::Window(int w, int h, const wchar_t* className) {
		Init(w, h, className);
	}


	Window::~Window() {
		for (auto e : Eventers)
			delete e;
	}

	void Window::Show() {
		ShowWindow(((WindowPrivate*)WindowData)->hWnd, SW_SHOW);
	}

	void Window::Hide() {
		ShowWindow(((WindowPrivate*)WindowData)->hWnd, SW_HIDE);
	}

	void Window::Resize(int Width, int Height) {
		RECT rc;
		auto h = ((WindowPrivate*)WindowData)->hWnd;
		GetWindowRect(h, &rc);
		MoveWindow(h, rc.left, rc.top, Width, Height, 1);
	}

	void Window::ResizeClient(int Width, int Height) {
		RECT rc, rc2 = { 0, 0, Width, Height };
		AdjustWindowRect(&rc2, WS_OVERLAPPEDWINDOW, FALSE);
		auto h = ((WindowPrivate*)WindowData)->hWnd;
		GetWindowRect(h, &rc);
		MoveWindow(h, rc.left, rc.top, rc2.right - rc2.left, rc2.bottom - rc2.top, 1);
	}
	RECT Window::ClientRect() {
		RECT cl;
		GetClientRect(((WindowPrivate*)WindowData)->hWnd, &cl);
		return cl;
	}
	RECT Window::GetWindowRectangle() {
		RECT rc;
		GetWindowRect(((WindowPrivate*)WindowData)->hWnd, &rc);
		return rc;
	}

	Eventer& Window::MakeEventer(int DialogItem) {
		return MakeEventer(GetDlgItem(((WindowPrivate*)WindowData)->hWnd, DialogItem));
	}

	Eventer& Window::MakeEventer(HWND hWnd) {
		Eventers.push_back(new Eventer(hWnd));
		return *Eventers.back();
	}

	void Window::LockCursor() {
		lockCursor.locked++;
		if (lockCursor.locked == 1) {
			POINT cur;
			GetCursorPos(&cur);
			auto rc = GetWindowRectangle();
			lockCursor.x = cur.x;
			lockCursor.y = cur.y;
			lockCursor.locked = true;
			lockCursor.CenterX = (rc.right + rc.left) / 2;
			lockCursor.CenterY = (rc.top + rc.bottom) / 2;
			SetCursorPos(lockCursor.CenterX, lockCursor.CenterY);
			ClipCursor(&rc);
			ShowCursor(0);
		}
	}

	void Window::UnlockCursor() {
		lockCursor.locked--;
		if (!lockCursor.locked) {
			ClipCursor(nullptr);
			ShowCursor(1);
			SetCursorPos(lockCursor.x, lockCursor.y);
		}
	}

	std::pair<int, int> Window::GetLockedRelative() {
		if (lockCursor.locked) {
			POINT cur;
			GetCursorPos(&cur);
			SetCursorPos(lockCursor.CenterX, lockCursor.CenterY);
			return { cur.x - lockCursor.CenterX, cur.y - lockCursor.CenterY };
		}
		return { 0, 0 };
	}

	int Eventer::DoEvents() {
		MSG msg = { 0 };
		if (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//return WindowCount;
		return WM_QUIT != msg.message;
	}

	int Eventer::DoEvents_NoRawInput() {
		using namespace std::chrono_literals;
		MSG msg = { 0 };
		for (;;) {
			if (PeekMessage(&msg, NULL, 0, WM_INPUT-1, PM_REMOVE)||PeekMessage(&msg, NULL, WM_INPUT+1, 0, PM_REMOVE)) {
				if (msg.message == WM_INPUT)
					return 0;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				return WindowCount;
			} else {
				std::this_thread::sleep_for(100ms);
			}
		}
	}

	int Eventer::DoEventsAsync() {
		MSG msg = { 0 };
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return WM_QUIT != msg.message;
	}

	void Eventer::EventLoop() {
		while (DoEvents());
	}
	void Eventer::DoEventThread(bool& Running) {
		using namespace std::chrono_literals;
		Running = true;
		EventThread = std::thread([&] {
			while (Eventer::DoEventsAsync() && Running)
				std::this_thread::sleep_for(1ms);
			Running = false;
		});
	}

	void Eventer::JoinEventThread() {
		EventThread.join();
	}

	bool Eventer::PostMsg(UINT Msg, WPARAM wParam, LPARAM lParam) {
		return PostMessage(((WindowPrivate*)WindowData)->hWnd, Msg, wParam, lParam);
	}

	HWND Eventer::Handle() {
		return ((WindowPrivate*)WindowData)->hWnd;
	}
	HWND Eventer::Handle(int DialogResource) {
		return GetDlgItem(((WindowPrivate*)WindowData)->hWnd, DialogResource);
	}
	void Eventer::Override(unsigned int Message, std::function<LRESULT(HWND, WPARAM, LPARAM)> Event) {
		(*(WindowPrivate*)WindowData).Events[Message].push_back(Event);
		(*(WindowPrivate*)WindowData).Flags |= (Message == WM_COMMAND);
	}

	void Eventer::OnCloseHWND(std::function<void(HWND)> lmd) {
		Override(WM_CLOSE, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd(hWnd);
			return 0;
		});
	}

	void Eventer::OnLeftButtonDownHWND(std::function<void(HWND, int, int)> lmd) {
		Override(WM_LBUTTONDOWN, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd(hWnd, GET_X_LPARAM(l), GET_Y_LPARAM(l));
			return 0;
		});
	}
	void Eventer::OnLeftButtonUpHWND(std::function<void(HWND, int, int)> lmd) {
		Override(WM_LBUTTONUP, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd(hWnd, GET_X_LPARAM(l), GET_Y_LPARAM(l));
			return 0;
		});
	}
	void Eventer::OnRightButtonDownHWND(std::function<void(HWND, int, int)> lmd) {
		Override(WM_RBUTTONDOWN, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd(hWnd, GET_X_LPARAM(l), GET_Y_LPARAM(l));
			return 0;
		});
	}
	void Eventer::OnRightButtonUpHWND(std::function<void(HWND, int, int)> lmd) {
		Override(WM_RBUTTONUP, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd(hWnd, GET_X_LPARAM(l), GET_Y_LPARAM(l));
			return 0;
		});
	}
	void Eventer::OnMiddleButtonDownHWND(std::function<void(HWND, int, int)> lmd) {
		Override(WM_MBUTTONDOWN, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd(hWnd, GET_X_LPARAM(l), GET_Y_LPARAM(l));
			return 0;
		});
	}
	void Eventer::OnMiddleButtonUpHWND(std::function<void(HWND, int, int)> lmd) {
		Override(WM_MBUTTONUP, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd(hWnd, GET_X_LPARAM(l), GET_Y_LPARAM(l));
			return 0;
		});
	}
	void Eventer::OnMouseMoveHWND(std::function<void(HWND, unsigned int, int, int)> lmd) {
		Override(WM_MOUSEMOVE, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd(hWnd, GET_KEYSTATE_WPARAM(w), GET_X_LPARAM(l), GET_Y_LPARAM(l));
			return 0;
		});
	}

	void Eventer::OnMouseWheelHWND(std::function<void(HWND, int x, int y, int delta)> lmd) {
		Override(WM_MOUSEWHEEL, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd(hWnd, GET_X_LPARAM(l), GET_Y_LPARAM(l), GET_WHEEL_DELTA_WPARAM(w));
			return 0;
		});
	}
	void Eventer::OnWindowActiveHWND(std::function<void(HWND, int Active)> lmd) {
		Override(WM_ACTIVATE, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd(hWnd, (int)w);
			return 0;
		});
	}
	void Eventer::OnResizeHWND(std::function<void(HWND, int x, int y)> lmd) {
		Override(WM_SIZE, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd(hWnd, GET_X_LPARAM(l), GET_Y_LPARAM(l));
			return 0;
		});
	}
	void Eventer::OnResizeClientHWND(std::function<void(HWND, int x, int y)> lmd) {
		Override(WM_SIZE, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			RECT cl;
			GetClientRect(hWnd, &cl);
			lmd(hWnd, cl.right - cl.left, cl.bottom - cl.top);
			return 0;
		});
	}
	void Eventer::OnResizingHWND(std::function<void(HWND, int Type, RECT * Rectangle)> lmd) {
		Override(WM_SIZING, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			int tp[] = { 0,
				Anchor::Left,
				Anchor::Right,
				Anchor::Top,
				Anchor::TopLeft,
				Anchor::TopRight,
				Anchor::Bottom,
				Anchor::BottomLeft,
				Anchor::BottomRight
			};
			lmd(hWnd, tp[w], (RECT*)l);
			return 0;
		});
	}
	void Eventer::OnPaint(std::function<void()> lmd) {
		Override(WM_PAINT, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd();
			ValidateRect(hWnd, 0);
			return 0;
		});
	}

	void Eventer::OnButton(unsigned int ControlId, std::function<void()> lmd) {
		WindowPrivate* ov = (WindowPrivate*)WindowData;
		ov->Command[ControlId | (BN_CLICKED << 16)].push_back([lmd](LPARAM l) {
			lmd();
		});
	}

	void Eventer::OnNotification(unsigned int ControlId, unsigned int Notification, std::function<void(HWND control)> lmd) {
		WindowPrivate* ov = (WindowPrivate*)WindowData;
		ov->Command[ControlId | (Notification << 16)].push_back([lmd](LPARAM l) {
			lmd((HWND)l);
		});
	}

	void Eventer::OnVirtualKeyDown(std::function<void(UCHAR VirtualKey)> lmd) {
		Override(WM_KEYDOWN, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd((UCHAR)w);
			return 0;
		});
	}

	void Eventer::OnVirtualKeyUp(std::function<void(UCHAR)> lmd) {
		Override(WM_KEYUP, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd((UCHAR)w);
			return 0;
		});
	}

	void Eventer::GetRawInput(std::function<void(const RAWINPUT&)> lmd) {
		UINT dwSize = 1024;
		using QWORD = unsigned long long;
		std::vector<unsigned char> RawInput(1024);
		auto res = GetRawInputBuffer(0, &dwSize, sizeof(RAWINPUTHEADER));
		if (!res && dwSize) {
			if (dwSize>RawInput.size()) RawInput.resize(dwSize);
			RAWINPUT* Input = (RAWINPUT*)&RawInput[0];
			GetRawInputBuffer(Input, &dwSize, sizeof(RAWINPUTHEADER));
			while (Input) {
				lmd(*Input);
				Input = NEXTRAWINPUTBLOCK(Input);
			}
		}
	}

	void Eventer::OnRawInput(std::function<void(const RAWINPUT&)> lmd) {
		Override(WM_INPUT, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			UINT dwSize = 0;
			std::vector<unsigned char> RawInput(1024);

			if (!GetRawInputData((HRAWINPUT)l, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER))&&dwSize) {
				if (dwSize>1024) RawInput.resize(dwSize);
				GetRawInputData((HRAWINPUT)l, RID_INPUT, &RawInput[0], &dwSize, sizeof(RAWINPUTHEADER));
				lmd(*(RAWINPUT*)RawInput.data());
			}
			return 0;
		});
	}

	void Eventer::RegisterRawInput(bool Mouse, bool Keyboard) {
		int Count = 0;
		RAWINPUTDEVICE Rid[2];
		if (Mouse) {
			auto& rid = Rid[Count++];
			rid.usUsagePage = HID_USAGE_PAGE_GENERIC;
			rid.usUsage = HID_USAGE_GENERIC_MOUSE;
			rid.dwFlags = RIDEV_INPUTSINK;
			rid.hwndTarget = ((WindowPrivate*)WindowData)->hWnd;
		}
		if (Keyboard) {
			auto& rid = Rid[Count++];
			rid.usUsagePage = HID_USAGE_PAGE_GENERIC;
			rid.usUsage = HID_USAGE_GENERIC_KEYBOARD;
			rid.dwFlags = RIDEV_INPUTSINK;
			rid.hwndTarget = ((WindowPrivate*)WindowData)->hWnd;
		}
		//if (Count) {
		RegisterRawInputDevices(Rid, Count, sizeof(RAWINPUTDEVICE));
	//}
	}

	void Eventer::OnMouseRelative(std::function<void(int, int, int, unsigned int)> clb) {
		static POINT Start{ -1,-1 }, Screen;
		static unsigned int btns = 0;

		HWND w = Handle();
		auto down = [w](int x, int y) {
			if (Start.x==-1) {
				Start.x = x; Start.y = y;
				ShowCursor(false);
				Screen = Start;
				ClientToScreen(w, &Screen);
			}
		};
		auto up = [](int x, int y) { if (Start.x!=-1) ShowCursor(true); Start.x = -1; };
		OnRightButtonDown(down);
		OnRightButtonUp(up);
		OnLeftButtonDown(down);
		OnLeftButtonUp(up);
		OnMiddleButtonDown(down);
		OnMiddleButtonUp(up);
		OnMouseWheel([clb](int, int, int d) { clb(0, 0, d, btns); });

		OnMouseMove([&, w, clb](unsigned int btn, int x, int y) {
			btns = btn;
			if (Start.x != -1 && btn) {
				clb(x-Start.x, y-Start.y, 0, btn);
				SetCursorPos(Screen.x, Screen.y);
			}
		});
	}

	///////////////////////////////////////////////////////////////////////////////////////////////


	void Eventer::OnClose(std::function<int()> lmd) {
		Override(WM_CLOSE, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			if (lmd())
				DestroyWindow(hWnd);
			return 0;
		});
	}

	void Eventer::OnLeftButtonDown(std::function<void(int, int)> lmd) {
		Override(WM_LBUTTONDOWN, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd(GET_X_LPARAM(l), GET_Y_LPARAM(l));
			return 0;
		});
	}
	void Eventer::OnLeftButtonUp(std::function<void(int, int)> lmd) {
		Override(WM_LBUTTONUP, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd(GET_X_LPARAM(l), GET_Y_LPARAM(l));
			return 0;
		});
	}
	void Eventer::OnRightButtonDown(std::function<void(int, int)> lmd) {
		Override(WM_RBUTTONDOWN, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd(GET_X_LPARAM(l), GET_Y_LPARAM(l));
			return 0;
		});
	}
	void Eventer::OnRightButtonUp(std::function<void(int, int)> lmd) {
		Override(WM_RBUTTONUP, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd(GET_X_LPARAM(l), GET_Y_LPARAM(l));
			return 0;
		});
	}
	void Eventer::OnMiddleButtonDown(std::function<void(int, int)> lmd) {
		Override(WM_MBUTTONDOWN, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd(GET_X_LPARAM(l), GET_Y_LPARAM(l));
			return 0;
		});
	}
	void Eventer::OnMiddleButtonUp(std::function<void(int, int)> lmd) {
		Override(WM_MBUTTONUP, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd(GET_X_LPARAM(l), GET_Y_LPARAM(l));
			return 0;
		});
	}
	void Eventer::OnMouseMove(std::function<void(unsigned int, int, int)> lmd) {
		Override(WM_MOUSEMOVE, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd((unsigned int)w, GET_X_LPARAM(l), GET_Y_LPARAM(l));
			return 0;
		});
	}
	void Eventer::OnMouseWheel(std::function<void(int x, int y, int delta)> lmd) {
		Override(WM_MOUSEWHEEL, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd(GET_X_LPARAM(l), GET_Y_LPARAM(l), GET_WHEEL_DELTA_WPARAM(w));
			return 0;
		});
	}
	void Eventer::OnWindowActive(std::function<void(int Active)> lmd) {
		Override(WM_ACTIVATE, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd((int)w);
			return 0;
		});
	}
	void Eventer::OnResize(std::function<void(int x, int y)> lmd) {
		Override(WM_SIZE, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd(GET_X_LPARAM(l), GET_Y_LPARAM(l));
			return 0;
		});
	}
	void Eventer::OnResizing(std::function<void(int Type, RECT * Rectangle)> lmd) {
		Override(WM_SIZING, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			int tp[] = { 0,
				Anchor::Left,
				Anchor::Right,
				Anchor::Top,
				Anchor::TopLeft,
				Anchor::TopRight,
				Anchor::Bottom,
				Anchor::BottomLeft,
				Anchor::BottomRight
			};
			lmd(tp[w], (RECT*)l);
			return 0;
		});
	}
	void Eventer::OnResizeClient(std::function<void(int x, int y)> lmd) {
		Override(WM_SIZE, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			RECT cl;
			GetClientRect(hWnd, &cl);
			lmd(cl.right - cl.left, cl.bottom - cl.top);
			return 0;
		});
	}
	void Eventer::OnPaintHWND(std::function<void(HWND)> lmd) {
		Override(WM_PAINT, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd(hWnd);
			return 0;
		});
	}

	void Eventer::OnVirtualKeyDownHWND(std::function<void(HWND, UCHAR)> lmd) {
		Override(WM_KEYDOWN, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd(hWnd, (UCHAR)w);
			return 0;
		});
	}

	void Eventer::OnVirtualKeyUpHWND(std::function<void(HWND, UCHAR)> lmd) {
		Override(WM_KEYUP, [lmd](HWND hWnd, WPARAM w, LPARAM l)->LRESULT {
			lmd(hWnd, (UCHAR)w);
			return 0;
		});
	}

	ComboBox::ComboBox(HWND hWnd, int Id) : Parent{ hWnd }, Id{ Id }, hWnd{ GetDlgItem(hWnd, Id) } {}
	void ComboBox::ResizeHeight() {
		int h1 = int(SendDlgItemMessageW(Parent, Id, CB_GETITEMHEIGHT, 0, 0));
		WINDOWPLACEMENT wp{ sizeof(WINDOWPLACEMENT) };
		GetWindowPlacement(hWnd, &wp);
		MoveWindow(hWnd,
			wp.rcNormalPosition.left,
			wp.rcNormalPosition.top,
			wp.rcNormalPosition.right - wp.rcNormalPosition.left,
			wp.rcNormalPosition.bottom - wp.rcNormalPosition.top + h1*10, true);
	}

	int ComboBox::AddItem(const std::wstring& Text, void* Data) {
		int cnt = Count();
		int index = int(SendDlgItemMessageW(Parent, Id, CB_ADDSTRING, 0, (LPARAM)Text.c_str()));
		if(Data)
			SendDlgItemMessageW(Parent, Id, CB_SETITEMDATA, index, (LPARAM)Data);
		ResizeHeight();
		return index;
	}

	void ComboBox::Clear() {
		SendDlgItemMessageW(Parent, Id, CB_RESETCONTENT, 0, 0);
		ResizeHeight();
	}

	int ComboBox::Count() const {
		return int(SendDlgItemMessageW(Parent, Id, CB_GETCOUNT, 0, 0));
	}

	int ComboBox::SelectedIndex() const {
		return int(SendDlgItemMessageW(Parent, Id, CB_GETCURSEL, 0, 0));
	}

	int ComboBox::Select(int i) {
		return int(SendDlgItemMessageW(Parent, Id, CB_SETCURSEL, i, 0));
	}

	std::wstring ComboBox::Text() const {
		int Index = SelectedIndex();
		int len = int(SendDlgItemMessageW(Parent, Id, CB_GETLBTEXTLEN, Index, 0));
		std::wstring ret(len, 0);
		if(len)
			SendDlgItemMessageW(Parent, Id, CB_GETLBTEXT, Index, (LPARAM)&ret[0]);
		return ret;
	}

	void* ComboBox::getData() {
		return (void*)SendDlgItemMessageW(Parent, Id, CB_GETLBTEXTLEN, SelectedIndex(), 0);
	}

	//void PVX::Windows::Resizing_MinWidth(int type, int Width, RECT * rc) {
	//	if ((rc->right - rc->left) < Width) {
	//		if (type & PVX_Anchor::Left) {
	//			rc->left = rc->right - Width;
	//		} else {
	//			rc->right = rc->left + Width;
	//		}
	//	}
	//}
	//void PVX::Windows::Resizing_MinHeight(int type, int Height, RECT * rc) {
	//	if ((rc->bottom - rc->top) < Height) {
	//		if (type & PVX_Anchor::Top) {
	//			rc->top = rc->bottom - Height;
	//		} else {
	//			rc->bottom = rc->top + Height;
	//		}
	//	}
	//}
	//void PVX::Windows::Resizing_MaxWidth(int type, int Width, RECT * rc) {
	//	if ((rc->right - rc->left) > Width) {
	//		if (type & PVX_Anchor::Left) {
	//			rc->left = rc->right - Width;
	//		} else {
	//			rc->right = rc->left + Width;
	//		}
	//	}
	//}
	//void PVX::Windows::Resizing_MaxHeight(int type, int Height, RECT * rc) {
	//	if ((rc->bottom - rc->top) > Height) {
	//		if (type & PVX_Anchor::Top) {
	//			rc->top = rc->bottom - Height;
	//		} else {
	//			rc->bottom = rc->top + Height;
	//		}
	//	}
	//}
}