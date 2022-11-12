#include <PVX_Window.h>
#include <PVX_Vulkan.h>
#include <PVX_File.h>

int main() {

	//auto bytecode = PVX::SpirV::Compile(PVX::IO::ReadText("glsl/defaults/Vertex00001.vert"), vk::ShaderStageFlagBits::eVertex, "Vertex00001.vert");

	//PVX::IO::Write("Vertex00001.vert", bytecode.data(), bytecode.size() * 4);

	//auto bytecode = PVX::IO::ReadBinary("Vertex00001.vert");
	auto bytecode = PVX::SpirV::CompileFile("glsl/defaults/Vertex00001.vert");

	namespace Vk = vk::raii;
	PVX::Windows::Window win(1280, 720);

	auto r = win.ClientRect();

	win.DefaultOnClose();
	win.Show();


	auto list = PVX::Vulkan::GetExtensionList();

	PVX::Vulkan::Instance Context;
	auto surface = Context.CreateSurface(win.Handle());

	auto Dev = Context.GetDevice(*surface);

	auto s = Dev.MakeShader(bytecode);
	auto s2 = Dev.LoadShader("Vertex00001.vert");

	auto Swapchain = Dev.MakeSwapchain(surface, r.right - r.left, r.bottom - r.top);

	auto PipelineBuilder = PVX::Vulkan::GraphicsPipelineBuilder(Dev._Device, Swapchain.Resolution);

	PipelineBuilder.SetTopology(vk::PrimitiveTopology::eTriangleList);
	PipelineBuilder.AddVertexShader(*s, "Vertex00001.vert");
	PipelineBuilder.AddVertexFormat({
		vk::Format::eR32G32B32Sfloat,
		vk::Format::eR32G32B32Sfloat,
		vk::Format::eR32G32Sfloat,
	});

	auto pool = Dev.MakeResetCommandPool();
	auto cmd = pool.CreatePrimary();

	auto bc = vk::BufferCopy(0, 0, 1024);
	cmd.copyBuffer({}, {}, { bc });

	std::vector<uint8_t> Data(1024);

	auto buf = Dev.MakeVertexBuffer(Data);

	win.EventLoop();
	return 0;
}