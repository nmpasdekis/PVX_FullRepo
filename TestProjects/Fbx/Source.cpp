#include <PVX_Window.h>
#include <PVX_OpenGL.h>
#include <PVX_Object3D.h>
#include <PVX_OpenGL_Helpers.h>

void SetWindow(PVX::Windows::Window& win, PVX::OpenGL::Context& gl, PVX::OpenGL::Camera& cam, int& Dragging) {
	win.OnRightButtonDown([&](int x, int y) {
		win.LockCursor();
		Dragging |= 1;
	});
	win.OnRightButtonUp([&](int x, int y) {
		win.UnlockCursor();
		Dragging &= ~1;
	});
	win.OnMouseWheel([&](int x, int y, int delta) {
		cam.OrbitDistance *= (1000 - delta) * 0.001f;
	});

	win.OnMiddleButtonDown([&](int x, int y) {
		win.LockCursor();
		Dragging |= 2;
	});
	win.OnMiddleButtonUp([&](int x, int y) {
		win.UnlockCursor();
		Dragging &= ~2;
	});
	win.OnMouseMove([&](auto btn, int x, int y) {
		if (Dragging) {
			auto [rx, ry] = win.GetLockedRelative();
			if (Dragging&1) {
				cam.Rotation.Yaw += rx * 0.01f;
				cam.Rotation.Pitch += ry * 0.01f;
			}
			if (Dragging&2) {
				cam.MoveCenter(rx * -0.002f * cam.OrbitDistance, ry * 0.002f * cam.OrbitDistance, 0);
			}
		}
	});
	win.OnMouseWheel([&](int x, int y, int delta) {
		cam.OrbitDistance *= (1000 - delta) * 0.001f;
	});
	win.OnResizeClient([&](int w, int h) {
		gl.AddTask([&](PVX::OpenGL::Context& gl) {
			gl.Resized(w, h);
			cam.SetSizePerspective(w, h);
		});
	});
}

int main() {
	PVX::Windows::Window win(1280, 720);
	win.Show();
	win.DefaultOnClose();

	PVX::OpenGL::Camera Camera(1280, 720, 60.0f, 0.001f, 1000.0f);
	//Camera.OrbitCenter.y = 3.0f;
	Camera.OrbitDistance = 5.0f;

	PVX::OpenGL::Context glContext(win.Handle(), false, [&Camera](PVX::OpenGL::Context& gl) {
		//wglSwapInterval(1);
		//glClearColor(0.5, 0.5, 0.5, 1.0);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		PVX::OpenGL::Helpers::ResourceManager Memory;
		PVX::OpenGL::Helpers::Renderer Engine(Memory, gl.Width, gl.Height, gl);

		Engine.Lights.light[0] = { { -1.0f, 2.5f, 0.0f, 1.0f }, { PVX::Vector3D{ 1.0f, 1.0f, 1.0f } *2.0f, 1.0f } };
		Engine.Lights.light[1] = { { -1.0f, 2.5f, -3.0f, 1.0f }, { PVX::Vector3D{ 1.0f, 1.0f, 1.0f } *2.0f, 1.0f } };
		Engine.Lights.light[2] = { { PVX::Vector3D{ 0.5f, -1.0f, -1.0f }.Normalized(), 0.0f }, { PVX::Vector3D{ 1.0f, 1.0f, 1.0f } *5.0f, 1.0f } };
		Engine.Lights.Count = 3;

		auto CameraBuffer = Camera.GetConstantBuffer();
		CameraBuffer.Name("CameraBuffer");
		Engine.SetCameraBuffer(CameraBuffer);

		//auto objId = Engine.LoadObject("D:\\Work\\fbx\\1\\HoffmanHigh2.fbx");
		auto objId = Engine.LoadObject("box.fbx");
		auto instId = Engine.CreateInstance(objId);

		while (gl.Running) {
			gl.DoTasks();
			Camera.UpdateView_Orbit();

			Engine.Render_gBuffer([&] {
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				Camera.UpdateConstantBuffer(CameraBuffer);

				Engine.UpdateInstances(Camera.Position, Camera.GetLookVector());

				glEnable(GL_DEPTH_TEST);
				Engine.DrawInstances();
				glDisable(GL_DEPTH_TEST);
			});

			Engine.DoPostProcess();

			gl.Present();
		}
	});

	int Dragging = 0;

	SetWindow(win, glContext, Camera, Dragging);

	win.EventLoop();
	//glContext.Running = false;
	return 0;
}