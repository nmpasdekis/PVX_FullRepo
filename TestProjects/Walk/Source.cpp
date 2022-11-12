#include <PVX_Window.h>
#include <PVX_OpenGL.h>
#include <PVX_OpenGL_Object.h>


void GameMain(PVX::Windows::Window& Window, PVX::OpenGL::Context& gl) {


	auto obj = PVX::OpenGL::LoadObj(L"walk.obj")[0];
	auto verts = obj.GetTriangles();

	PVX::OpenGL::BufferObject Grid = PVX::OpenGL::MakeGrid();
	PVX::OpenGL::BufferObject Path = obj;
	PVX::OpenGL::Camera cam;
	cam.SetPerspective();
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(cam.GetProjectionMatrix().m16);
	glMatrixMode(GL_MODELVIEW);
	cam.OrbitDistance = 10.0f;
	cam.Rotation.Pitch = PVX::ToRAD(45.0);


	Window.OnLockedDrag(2, [&cam](int btn, int x, int y) {
		if (btn&2) {
			cam.Rotation.Pitch += 0.005 * y;
			cam.Rotation.Yaw += 0.005 * x;
		}
	});
	Window.OnMouseWheel([&cam](int x, int y, int delta) {
		cam.OrbitDistance *= (1000 - delta) / 1000.0f;
	});


	wglSwapInterval(1);
	while (gl.Running) {
		gl.DoTasks();
		cam.UpdateView_Orbit();
		glLoadMatrixf(cam.GetViewMatrix().m16);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		Grid.BindDrawUnbind();
		Path.BindDrawUnbind();

		gl.Present();
	}
}