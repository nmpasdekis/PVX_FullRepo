#include <PVX_Window.h>
#include <PVX_OpenGL.h>
#include <PVX_OpenGL_Object.h>

void GameMain(PVX::Windows::Window& Window, PVX::OpenGL::Context& gl);

void main() {
	PVX::Windows::Window win(1280, 720);
	win.DefaultOnClose();
	win.Show();

	PVX::OpenGL::Context gl(win.Handle(), true, [&win](PVX::OpenGL::Context& gl) {
		GameMain(win, gl);
	});
	win.EventLoop();
}