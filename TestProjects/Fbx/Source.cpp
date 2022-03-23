#include <PVX_Object3D.h>
#include <PVX_Window.h>
#include <PVX_OpenGL.h>


int main() {
	auto obj =  PVX::Object3D::LoadFbx("D:\\mine\\PVX_FullRepo\\TestProjects\\Fbx\\fbx\\1\\HoffmanHigh.fbx");

	PVX::Windows::Window win(1280, 720);
	PVX::OpenGL::Context gl(win.Handle(), true, [&](PVX::OpenGL::Context& gl) {
		
	});

	return 0;
}