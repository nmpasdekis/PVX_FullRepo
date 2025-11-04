#pragma once
#include "PVX_Window.h"
#include <string>

namespace PVX::WebView {
	class View {
		void * ptr;
		View(void*);
		friend View CreateComposition(PVX::Windows::Window& win, std::wstring url);
		friend View Create(PVX::Windows::Window& win, std::wstring url);

		//std::wstring RunScript(std::wstring);
		void Creation(std::function<void()> fnc);
	public:
		void NavigateTo(std::wstring url);
		void Reload();
		void OnWebMessageReceived(std::function<void(std::wstring msg)> clb);
	};
	View CreateComposition(PVX::Windows::Window& win, std::wstring url);
	View Create(PVX::Windows::Window& win, std::wstring url);
}