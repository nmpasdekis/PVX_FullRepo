#pragma once
#include "PVX_Window.h"
#include "PVX_json.h"
#include <string>

namespace PVX::WebView {
	struct internalPanel;
	class View {
		internalPanel * internal;
		View(internalPanel*);
		friend View CreateComposition(PVX::Windows::Window& win, std::wstring url);
		friend View Create(PVX::Windows::Window& win, std::wstring url);

		//std::wstring RunScript(std::wstring);
		void Creation(std::function<void()> fnc);
	public:
		void NavigateTo(std::wstring url);
		void Reload();
		void OnWebMessageReceived_JSON(std::function<void(const PVX::JSON::Item& msg)> clb);
		void OnWebMessageReceived(std::function<void(const std::wstring& msg)> clb);
	};
	View CreateComposition(PVX::Windows::Window& win, std::wstring url);
	View Create(PVX::Windows::Window& win, std::wstring url);
}