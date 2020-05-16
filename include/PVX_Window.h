#ifndef __PVX_WINDOW_2019_H__
#define __PVX_WINDOW_2019_H__

#include <Windows.h>
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <PVX.inl>

namespace PVX::Windows {
	class Anchor {
	public:
		enum {
			Top = 1,
			Bottom = 2,
			Left = 4,
			Right = 8,
			TopLeft = 3,
			BottomLeft = 6,
			TopRight = 9,
			BottomRight = 10,
			All = 15,
		};

		Anchor(HWND Main);
		Anchor();
		~Anchor();

		void SetMain(HWND main);
		void Set(int Point, HWND control);
		void Set(int Point, const HWND ctrlIds[], int count);
		void Set(int Point, const std::vector<HWND> ctrlIds);
		void Set(int Point, int ctrlId);
		void Set(int Point, const int ctrlIds[], int count);
		void Set(int Point, const std::vector<int> ctrlIds);
		void Apply();
	private:
		HWND Main;
		RECT MainRect;
		std::map<HWND, void*> Anchors;
		int IsDialog;
	};
	class Eventer {
	public:
		~Eventer();
		Eventer(HWND hWnd);
		Eventer(const Eventer&) = delete;

		enum class WindowMessage {
#include "wm_enum.inl"
		};
		static int DoEvents();
		static int DoEvents_NoRawInput();
		static int DoEventsAsync();
		static void EventLoop();

		static void DoEventThread(bool& Running);
		static void JoinEventThread();

		bool PostMsg(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);

		HWND Handle();
		HWND Handle(int DialogResource);

		void Override(unsigned int Message, std::function<LRESULT(HWND, WPARAM, LPARAM)> Event);

		void OnLeftButtonDownHWND(std::function<void(HWND, int x, int y)> lmd);
		void OnLeftButtonUpHWND(std::function<void(HWND, int x, int y)> lmd);
		void OnRightButtonDownHWND(std::function<void(HWND, int x, int y)> lmd);
		void OnRightButtonUpHWND(std::function<void(HWND, int x, int y)> lmd);
		void OnMiddleButtonDownHWND(std::function<void(HWND, int x, int y)> lmd);
		void OnMiddleButtonUpHWND(std::function<void(HWND, int x, int y)> lmd);
		void OnMouseWheelHWND(std::function<void(HWND, int x, int y, int delta)> lmd);
		void OnMouseMoveHWND(std::function<void(HWND, unsigned int Button, int x, int y)> lmd);
		void OnWindowActiveHWND(std::function<void(HWND, int Active)> lmd);
		void OnCloseHWND(std::function<void(HWND)> lmd);
		void OnResizeHWND(std::function<void(HWND, int Width, int Height)> lmd);
		void OnResizingHWND(std::function<void(HWND, int Type, RECT * Rectangle)> lmd);
		void OnResizeClientHWND(std::function<void(HWND, int Width, int Height)> lmd);
		void OnPaintHWND(std::function<void(HWND)> lmd);
		void OnVirtualKeyDownHWND(std::function<void(HWND, UCHAR)> lmd);
		void OnVirtualKeyUpHWND(std::function<void(HWND, UCHAR)> lmd);

		void OnLeftButtonDown(std::function<void(int x, int y)> lmd);
		void OnLeftButtonUp(std::function<void(int x, int y)> lmd);
		void OnRightButtonDown(std::function<void(int x, int y)> lmd);
		void OnRightButtonUp(std::function<void(int x, int y)> lmd);
		void OnMiddleButtonDown(std::function<void(int x, int y)> lmd);
		void OnMiddleButtonUp(std::function<void(int x, int y)> lmd);
		void OnMouseWheel(std::function<void(int x, int y, int delta)> lmd);
		void OnMouseMove(std::function<void(unsigned int Button, int x, int y)> lmd);
		void OnWindowActive(std::function<void(int Active)> lmd);
		void OnClose(std::function<int()> lmd);
		void OnResize(std::function<void(int Width, int Height)> lmd);
		void OnResizing(std::function<void(int Type, RECT * Rectangle)> lmd);
		void OnResizeClient(std::function<void(int Width, int Height)> lmd);
		void OnPaint(std::function<void()> lmd);
		void OnButton(unsigned int ControlId, std::function<void()> lmd);
		void OnNotification(unsigned int ControlId, unsigned int Notification, std::function<void(HWND control)> lmd);
		void OnVirtualKeyDown(std::function<void(UCHAR)> lmd);
		void OnVirtualKeyUp(std::function<void(UCHAR)> lmd);

		void OnRawInput(std::function<void(const RAWINPUT&)> lmd);
		void GetRawInput(std::function<void(const RAWINPUT&)> lmd);
		void RegisterRawInput(bool Mouse, bool Keyboard);

		void OnMouseRelative(std::function<void(int, int, int, unsigned int)> clb);
	protected:
		Eventer() {};
		void* WindowData = nullptr;
	};

	class ComboBox {
		HWND hWnd = nullptr;
		HWND Parent = nullptr;
		int Id = 0;
		ComboBox(HWND hWnd, int Id);
		void ResizeHeight();
	public:
		ComboBox() {}
		ComboBox(const ComboBox&) = default;
		ComboBox& operator=(const ComboBox&) = default;
		int AddItem(const std::wstring& Text, void * Data = nullptr);
		void Clear();
		int Count() const;
		int SelectedIndex() const;
		int Select(int i);
		std::wstring Text() const;
		void* getData();

		template<typename T>
		T& Data() {
			return *(T*)getData();
		}
		friend class Window;
	};

	class Window: public Eventer {
	public:
		Window(int Width, int Height, const char * WindowTitle = "PVX::Window");
		Window(int DialogResource);
		Window(const Window&) = delete;
		~Window();

		void Show();
		void Hide();

		void Resize(int Width, int Height);
		void ResizeClient(int Width, int Height);
		RECT ClientRect();

		Eventer& MakeEventer(int DialogItem);
		Eventer& MakeEventer(HWND hWnd);
		inline ComboBox GetComboBox(int Id) { return { *(HWND*)WindowData, Id }; };
	protected:
		void Init(int w, int h, const char* className);
		std::vector<Eventer*> Eventers;
	};

	class RawInput {
		HWND hWnd;
	};
}

#endif