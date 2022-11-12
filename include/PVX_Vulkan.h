#pragma once
#define NOMINMAX

#include <filesystem>
#include <optional>

#ifndef __linux
#define VK_USE_PLATFORM_WIN32_KHR
#else
#define VK_USE_PLATFORM_XCB_KHR
#endif

#include <vulkan/vulkan_raii.hpp>

#ifdef _WIN32
#include <Windows.h>
#elif defined(__linux)

#endif

namespace PVX::Vulkan {
	namespace Vk = vk::raii;

	class Swapchain {
		friend class Device;
		Vk::Device& _Device;
	public:
		Swapchain(Vk::SwapchainKHR& sc, Vk::Device& dev, vk::Extent2D ext, vk::Format fmt = {});
		vk::Extent2D Resolution;
		Vk::SwapchainKHR _Swapchain;
		std::vector<vk::Image> Images{};
		std::vector<Vk::ImageView> Views{};
	};

	class GraphicsPipelineBuilder {
		Vk::Device& dev;
		vk::Viewport Viewport;
		vk::Rect2D Scissor;
		vk::PipelineViewportStateCreateInfo ViewportState;
		std::vector<vk::PipelineShaderStageCreateInfo> Shaders;
		std::vector<vk::VertexInputAttributeDescription> VertexInputAttributes;
		std::vector<vk::VertexInputBindingDescription> VertexInputBinding;
		vk::PipelineVertexInputStateCreateInfo VertexInputState;
		vk::PipelineInputAssemblyStateCreateInfo InputAssemblyState;
		vk::PipelineRasterizationStateCreateInfo RasterizationState{
			{}, false, false,
			vk::PolygonMode::eFill,
			vk::CullModeFlagBits::eBack,
			vk::FrontFace::eClockwise,
			false, {}, {}, {},
			1.0f
		};
		vk::PipelineMultisampleStateCreateInfo MultisampleState{ {}, vk::SampleCountFlagBits::e1, false };
		vk::PipelineColorBlendAttachmentState ColorBlendAttachment = vk::PipelineColorBlendAttachmentState({}, 
			vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd, 
			vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd, 
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
		vk::PipelineColorBlendStateCreateInfo ColorBlendState = vk::PipelineColorBlendStateCreateInfo({}, {}, vk::LogicOp::eCopy, 1, &ColorBlendAttachment);
		vk::PipelineLayoutCreateInfo Layout;
		vk::GraphicsPipelineCreateInfo info;
	public:
		GraphicsPipelineBuilder(Vk::Device& dev);
		GraphicsPipelineBuilder(Vk::Device& dev, const vk::Extent2D& Resolution);
		void SetTopology(vk::PrimitiveTopology topo, bool EnableReset = false);
		void AddVertexShader(const vk::ShaderModule& s, const char* Name = nullptr);
		void AddFragmentShader(const vk::ShaderModule& s, const char* Name = nullptr);
		void AddGeometryShader(const vk::ShaderModule& s, const char* Name = nullptr);
		void AddVertexFormat(const std::vector<vk::Format>& vf, bool ForInstance = false, int32_t binding = -1);
	};

	class Buffer {
	protected:
		Vk::Buffer buffer;
		Vk::DeviceMemory devMemory;
		size_t Size;
		Buffer(Vk::Device& dev, vk::PhysicalDeviceMemoryProperties& memProps, uint32_t qIndex, vk::DeviceSize Size, vk::BufferUsageFlags Usage, vk::SharingMode sMode, vk::MemoryPropertyFlags properties);
		friend class Device;
	public:
		inline size_t size() const { return Size; }
	};

	class CommandPool {
		CommandPool(Vk::Device& dev, uint32_t qIndex, vk::CommandPoolCreateFlagBits flags);
		Vk::Device& dev;
		Vk::CommandPool pool;
		friend class Device;
	public:
		Vk::CommandBuffer CreatePrimary();
		Vk::CommandBuffer CreateSecondary();
	};

	class Device {
		Vk::SwapchainKHR CreateSwapChain(Vk::SurfaceKHR& Surface, vk::Extent2D& ext, vk::Format& fmt);
		Device(Vk::PhysicalDevice&&, Vk::Device&&, Vk::Queue&&, uint32_t queueFamily);
		friend class Instance;
	public:
		Vk::PhysicalDevice _PhysicalDevice;
		Vk::Device _Device;
		Vk::Queue _Queue;
		vk::PhysicalDeviceMemoryProperties MemoryProps;
		uint32_t queueFamily;
		CommandPool InternalCommandPool;
		Buffer TransferBuffer;
		Swapchain MakeSwapchain(Vk::SurfaceKHR& Surface, uint32_t Width, uint32_t Height);

		Vk::ShaderModule MakeShader(const std::vector<uint8_t>& bytecode);
		Vk::ShaderModule MakeShader(const std::vector<uint32_t>& bytecode);
		Vk::ShaderModule LoadShader(const std::filesystem::path& Filename);

		Buffer MakeVertexBuffer(const std::vector<uint8_t>& Data);
		Buffer MakeGeneralBuffer(vk::DeviceSize Size);

		CommandPool MakeResetCommandPool();
	};

	class Instance {
		Vk::Context _Context;
		Vk::Instance _Instance;
	public:
		Instance();
		Device GetDevice(VkSurfaceKHR surface = nullptr);
		Vk::SurfaceKHR CreateSurface(HWND hWnd);
	};
	std::vector<VkExtensionProperties> GetExtensionList();
}
namespace PVX::SpirV {
	std::vector<uint32_t> Compile(const std::string& Code, vk::ShaderStageFlagBits Type, const std::string_view& filename);
	std::vector<uint32_t> CompileFile(const std::filesystem::path& Filename);
}