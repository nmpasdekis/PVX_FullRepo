#include <PVX_Window.h>
#include <PVX_OpenGL.h>
#include <PVX_OpenGL_Helpers.h>

namespace PVX::Application {
	struct Mode {
		std::function<void(int btn, int x, int y)> OnDrag = nullptr;
		std::function<void(int btn, int x, int y)> OnMouseDown = nullptr;
		std::function<void(int btn, int x, int y)> OnMouseUp = nullptr;
		std::function<void()> OnDraw = nullptr;
	};


}