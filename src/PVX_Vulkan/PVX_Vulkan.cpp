#include <PVX_Vulkan.h>
#include <limits>
#include <PVX.inl>
#include <PVX_File.h>

#ifdef NDEBUG
constexpr std::array<const char*, 0> Layers{};
#else
constexpr std::array<const char*, 1> Layers{ "VK_LAYER_KHRONOS_validation" };
#endif

#ifdef __linux
#define PLATFORM_SURFACE_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAMEreturn ret;
#endif
#ifdef _WIN32
#define PLATFORM_SURFACE_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif


constexpr std::array PlatformExtensions{
	VK_KHR_SURFACE_EXTENSION_NAME,
	PLATFORM_SURFACE_NAME
};

namespace PVX::Vulkan {
	namespace {
#ifdef __linux
		VkSurfaceKHR CreateSurfaceKHR(VkInstance instance, xcb_connection_t* connection, xcb_window_t window) {
			VkXcbSurfaceCreateInfoKHR createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;
			createInfo.connection = connection;
			createInfo.window = window;

			VkSurfaceKHR ret;
			vkCreateXcbSurfaceKHR(instance, &createInfo, nullptr, &ret);
			return ret;
		}
#endif

#ifdef _WIN32
		inline Vk::SurfaceKHR CreateSurfaceKHR(Vk::Instance& instance, HWND hWnd) {
			vk::Win32SurfaceCreateInfoKHR info({}, GetModuleHandle(nullptr), hWnd);
			return instance.createWin32SurfaceKHR(info);
		}
#endif

		Vk::Instance CreateInstance(Vk::Context& ctx) {
			vk::ApplicationInfo AppInfo("PerVertEX App", VK_MAKE_VERSION(1, 0, 0), "PerVertEX Engine", VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_1);
			vk::InstanceCreateInfo InstInfo({}, &AppInfo, Layers.size(), Layers.data(), PlatformExtensions.size(), PlatformExtensions.data());
			return { ctx, InstInfo };
		}

		std::vector<VkQueueFamilyProperties> TestQueues(VkPhysicalDevice dev) {
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());
			return queueFamilies;
		}

		constexpr std::array DevExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		Vk::Device CreateDevice(Vk::PhysicalDevice& dev, uint32_t& index, VkSurfaceKHR surface) {
			vk::DeviceCreateInfo info;

			auto qq = dev.getQueueFamilyProperties();
			std::vector<vk::DeviceQueueCreateInfo> QueueInfos;

			auto fam = vk::QueueFlags(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT);

			index = 0;
			float prior[]{ 1.0f };
			for (auto& q : qq) {
				VkBool32 presentSupport = true;
				if (surface) {
					presentSupport = false;
					vkGetPhysicalDeviceSurfaceSupportKHR(*dev, index, surface, &presentSupport);
				}
				if (q.queueFlags & fam && presentSupport) {
					QueueInfos.push_back(vk::DeviceQueueCreateInfo({}, index, 1, prior));
					break;
				}
				index++;
			}

			info.enabledExtensionCount = DevExtensions.size();
			info.ppEnabledExtensionNames = DevExtensions.data();
			info.pQueueCreateInfos = QueueInfos.data();
			info.queueCreateInfoCount = QueueInfos.size();

			return { dev, info };
		}
	}

	Instance::Instance() : _Instance{ CreateInstance(_Context) } {}

	Device Instance::GetDevice(VkSurfaceKHR surface) {
		auto phyDevice = std::move(Vk::PhysicalDevices(_Instance)[0]);

		uint32_t qIndex;
		auto Dev = CreateDevice(phyDevice, qIndex, surface);
		auto Queue = Dev.getQueue(qIndex, 0);
		return Device{
			std::move(phyDevice),
			std::move(Dev),
			std::move(Queue),
			qIndex
		};
	}

	Vk::SurfaceKHR Instance::CreateSurface(HWND hWnd) {
		return CreateSurfaceKHR(_Instance, hWnd);
	}

	std::vector<VkExtensionProperties> GetExtensionList() {
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
		return extensions;
	}

	template<typename clbType, typename RetType>
	RetType find(const std::vector<RetType>& list, clbType clb) {
		for (const auto& it : list) {
			if (clb(it)) return it;
		}
		return list[0];
	}

	Device::Device(Vk::PhysicalDevice&& p, Vk::Device&& d, Vk::Queue&& q, uint32_t queueFamily) :
		_PhysicalDevice{ std::move(p) }, 
		_Device{ std::move(d) }, 
		_Queue{ std::move(q) },
		MemoryProps{ _PhysicalDevice.getMemoryProperties() },
		queueFamily{ queueFamily },
		InternalCommandPool{ MakeResetCommandPool() },
		TransferBuffer{ MakeGeneralBuffer(1024*1024) }

	{}

	Vk::SwapchainKHR Device::CreateSwapChain(Vk::SurfaceKHR& Surface, vk::Extent2D& ext, vk::Format& fmt) {
		auto caps = _PhysicalDevice.getSurfaceCapabilitiesKHR(*Surface);
		auto formats = _PhysicalDevice.getSurfaceFormatsKHR(*Surface);
		auto modes = _PhysicalDevice.getSurfacePresentModesKHR(*Surface);

		auto mode = PVX::find(modes, [](const vk::PresentModeKHR& i) {
			return i == vk::PresentModeKHR::eMailbox;
		}, [&modes] { return PVX::find(modes, [](const vk::PresentModeKHR& i) {
			return i == vk::PresentModeKHR::eFifoRelaxed;
		}, vk::PresentModeKHR::eFifo); });

		auto format = find(formats, [](const vk::SurfaceFormatKHR& i) {
			return i.format == vk::Format::eB8G8R8A8Srgb && i.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
		});
		fmt = format.format;

		uint32_t imageCount = caps.minImageCount + 1;
		if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) {
			imageCount = caps.maxImageCount;
		}

		ext = [&caps, ext] {
			if (caps.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
				return vk::Extent2D{
					std::clamp(ext.width, caps.minImageExtent.width, caps.maxImageExtent.width),
					std::clamp(ext.height, caps.minImageExtent.height, caps.maxImageExtent.height),
				};
			}
			return caps.currentExtent;
		}();

		return _Device.createSwapchainKHR(vk::SwapchainCreateInfoKHR({},
			*Surface,
			imageCount,
			format.format,
			format.colorSpace,
			ext,
			1,
			vk::ImageUsageFlagBits::eColorAttachment,
			vk::SharingMode::eExclusive,
			{},
			{},
			vk::SurfaceTransformFlagBitsKHR::eIdentity,
			vk::CompositeAlphaFlagBitsKHR::eOpaque,
			mode,
			VK_TRUE,
			VK_NULL_HANDLE
		));
	}
	Swapchain Device::MakeSwapchain(Vk::SurfaceKHR& Surface, uint32_t Width, uint32_t Height) {
		vk::Format fmt;
		vk::Extent2D Resolution{ Width, Height };
		auto sc = CreateSwapChain(Surface, Resolution, fmt);
		return Swapchain(sc, _Device, Resolution, fmt);
	}

	Vk::ShaderModule Device::MakeShader(const std::vector<uint8_t>& bytecode) {
		return _Device.createShaderModule(vk::ShaderModuleCreateInfo({}, bytecode.size(), (uint32_t*)bytecode.data()));
	}
	Vk::ShaderModule Device::MakeShader(const std::vector<uint32_t>& bytecode) {
		return _Device.createShaderModule(vk::ShaderModuleCreateInfo({}, bytecode));
	}

	Vk::ShaderModule Device::LoadShader(const std::filesystem::path& Filename) {
		return MakeShader(PVX::IO::ReadBinary(Filename));
	}

	Swapchain::Swapchain(Vk::SwapchainKHR& sc, Vk::Device& dev, vk::Extent2D ext, vk::Format fmt) :
		_Device{ dev }, Resolution{ ext }, _Swapchain{ std::move(sc) },
		Images{ PVX::Map(_Swapchain.getImages(), [](const VkImage& img) { return vk::Image{ img }; }) },
		Views{ PVX::Map(Images, [this, fmt](const vk::Image& img) { return Vk::ImageView(_Device, vk::ImageViewCreateInfo({}, img, vk::ImageViewType::e2D, fmt, vk::ComponentMapping(), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))); }) } {}

	const std::map<vk::Format, int> FormatSize{
		// { vk::Format::eUndefined, 0 },
		{ vk::Format::eR4G4UnormPack8, 1 },
		{ vk::Format::eR4G4B4A4UnormPack16, 2 },
		{ vk::Format::eB4G4R4A4UnormPack16, 2 },
		{ vk::Format::eR5G6B5UnormPack16, 2 },
		{ vk::Format::eB5G6R5UnormPack16, 2 },
		{ vk::Format::eR5G5B5A1UnormPack16, 2 },
		{ vk::Format::eB5G5R5A1UnormPack16, 2 },
		{ vk::Format::eA1R5G5B5UnormPack16, 2 },
		{ vk::Format::eR8Unorm, 1 },
		{ vk::Format::eR8Snorm, 1 },
		{ vk::Format::eR8Uscaled, 1 },
		{ vk::Format::eR8Sscaled, 1 },
		{ vk::Format::eR8Uint, 1 },
		{ vk::Format::eR8Sint, 1 },
		{ vk::Format::eR8Srgb, 1 },
		{ vk::Format::eR8G8Unorm, 2 },
		{ vk::Format::eR8G8Snorm, 2 },
		{ vk::Format::eR8G8Uscaled, 2 },
		{ vk::Format::eR8G8Sscaled, 2 },
		{ vk::Format::eR8G8Uint, 2 },
		{ vk::Format::eR8G8Sint, 2 },
		{ vk::Format::eR8G8Srgb, 2 },
		{ vk::Format::eR8G8B8Unorm, 3 },
		{ vk::Format::eR8G8B8Snorm, 3 },
		{ vk::Format::eR8G8B8Uscaled, 3 },
		{ vk::Format::eR8G8B8Sscaled, 3 },
		{ vk::Format::eR8G8B8Uint, 3 },
		{ vk::Format::eR8G8B8Sint, 3 },
		{ vk::Format::eR8G8B8Srgb, 3 },
		{ vk::Format::eB8G8R8Unorm, 3 },
		{ vk::Format::eB8G8R8Snorm, 3 },
		{ vk::Format::eB8G8R8Uscaled, 3 },
		{ vk::Format::eB8G8R8Sscaled, 3 },
		{ vk::Format::eB8G8R8Uint, 3 },
		{ vk::Format::eB8G8R8Sint, 3 },
		{ vk::Format::eB8G8R8Srgb, 3 },
		{ vk::Format::eR8G8B8A8Unorm, 4 },
		{ vk::Format::eR8G8B8A8Snorm, 4 },
		{ vk::Format::eR8G8B8A8Uscaled, 4 },
		{ vk::Format::eR8G8B8A8Sscaled, 4 },
		{ vk::Format::eR8G8B8A8Uint, 4 },
		{ vk::Format::eR8G8B8A8Sint, 4 },
		{ vk::Format::eR8G8B8A8Srgb, 4 },
		{ vk::Format::eB8G8R8A8Unorm, 4 },
		{ vk::Format::eB8G8R8A8Snorm, 4 },
		{ vk::Format::eB8G8R8A8Uscaled, 4 },
		{ vk::Format::eB8G8R8A8Sscaled, 4 },
		{ vk::Format::eB8G8R8A8Uint, 4 },
		{ vk::Format::eB8G8R8A8Sint, 4 },
		{ vk::Format::eB8G8R8A8Srgb, 4 },
		{ vk::Format::eA8B8G8R8UnormPack32, 4 },
		{ vk::Format::eA8B8G8R8SnormPack32, 4 },
		{ vk::Format::eA8B8G8R8UscaledPack32, 4 },
		{ vk::Format::eA8B8G8R8SscaledPack32, 4 },
		{ vk::Format::eA8B8G8R8UintPack32, 4 },
		{ vk::Format::eA8B8G8R8SintPack32, 4 },
		{ vk::Format::eA8B8G8R8SrgbPack32, 4 },
		{ vk::Format::eA2R10G10B10UnormPack32, 4 },
		{ vk::Format::eA2R10G10B10SnormPack32, 4 },
		{ vk::Format::eA2R10G10B10UscaledPack32, 4 },
		{ vk::Format::eA2R10G10B10SscaledPack32, 4 },
		{ vk::Format::eA2R10G10B10UintPack32, 4 },
		{ vk::Format::eA2R10G10B10SintPack32, 4 },
		{ vk::Format::eA2B10G10R10UnormPack32, 4 },
		{ vk::Format::eA2B10G10R10SnormPack32, 4 },
		{ vk::Format::eA2B10G10R10UscaledPack32, 4 },
		{ vk::Format::eA2B10G10R10SscaledPack32, 4 },
		{ vk::Format::eA2B10G10R10UintPack32, 4 },
		{ vk::Format::eA2B10G10R10SintPack32, 4 },
		{ vk::Format::eR16Unorm, 2 },
		{ vk::Format::eR16Snorm, 2 },
		{ vk::Format::eR16Uscaled, 2 },
		{ vk::Format::eR16Sscaled, 2 },
		{ vk::Format::eR16Uint, 2 },
		{ vk::Format::eR16Sint, 2 },
		{ vk::Format::eR16Sfloat, 2 },
		{ vk::Format::eR16G16Unorm, 4 },
		{ vk::Format::eR16G16Snorm, 4 },
		{ vk::Format::eR16G16Uscaled, 4 },
		{ vk::Format::eR16G16Sscaled, 4 },
		{ vk::Format::eR16G16Uint, 4 },
		{ vk::Format::eR16G16Sint, 4 },
		{ vk::Format::eR16G16Sfloat, 4 },
		// { vk::Format::eR16G16B16Unorm, 0 },
		// { vk::Format::eR16G16B16Snorm, 0 },
		// { vk::Format::eR16G16B16Uscaled, 0 },
		// { vk::Format::eR16G16B16Sscaled, 0 },
		// { vk::Format::eR16G16B16Uint, 0 },
		// { vk::Format::eR16G16B16Sint, 0 },
		// { vk::Format::eR16G16B16Sfloat, 0 },
		{ vk::Format::eR16G16B16A16Unorm, 8 },
		{ vk::Format::eR16G16B16A16Snorm, 8 },
		{ vk::Format::eR16G16B16A16Uscaled, 8 },
		{ vk::Format::eR16G16B16A16Sscaled, 8 },
		{ vk::Format::eR16G16B16A16Uint, 8 },
		{ vk::Format::eR16G16B16A16Sint, 8 },
		{ vk::Format::eR16G16B16A16Sfloat, 8 },
		{ vk::Format::eR32Uint, 4 },
		{ vk::Format::eR32Sint, 4 },
		{ vk::Format::eR32Sfloat, 4 },
		{ vk::Format::eR32G32Uint, 8 },
		{ vk::Format::eR32G32Sint, 8 },
		{ vk::Format::eR32G32Sfloat, 8 },
		{ vk::Format::eR32G32B32Uint, 12 },
		{ vk::Format::eR32G32B32Sint, 12 },
		{ vk::Format::eR32G32B32Sfloat, 12 },
		{ vk::Format::eR32G32B32A32Uint, 16 },
		{ vk::Format::eR32G32B32A32Sint, 16 },
		{ vk::Format::eR32G32B32A32Sfloat, 16 },
		{ vk::Format::eR64Uint, 8 },
		{ vk::Format::eR64Sint, 8 },
		{ vk::Format::eR64Sfloat, 8 },
		{ vk::Format::eR64G64Uint, 16 },
		{ vk::Format::eR64G64Sint, 16 },
		{ vk::Format::eR64G64Sfloat, 16 },
		// { vk::Format::eR64G64B64Uint, 0 },
		// { vk::Format::eR64G64B64Sint, 0 },
		// { vk::Format::eR64G64B64Sfloat, 0 },
		{ vk::Format::eR64G64B64A64Uint, 32 },
		{ vk::Format::eR64G64B64A64Sint, 32 },
		{ vk::Format::eR64G64B64A64Sfloat, 32 },
		{ vk::Format::eB10G11R11UfloatPack32, 4 },
		{ vk::Format::eE5B9G9R9UfloatPack32, 3 },
		// { vk::Format::eD16Unorm, 0 },
		// { vk::Format::eX8D24UnormPack32, 0 },
		// { vk::Format::eD32Sfloat, 0 },
		// { vk::Format::eS8Uint, 0 },
		// { vk::Format::eD16UnormS8Uint, 0 },
		// { vk::Format::eD24UnormS8Uint, 0 },
		// { vk::Format::eD32SfloatS8Uint, 0 },
		// { vk::Format::eBc1RgbUnormBlock, 0 },
		// { vk::Format::eBc1RgbSrgbBlock, 0 },
		// { vk::Format::eBc1RgbaUnormBlock, 0 },
		// { vk::Format::eBc1RgbaSrgbBlock, 0 },
		// { vk::Format::eBc2UnormBlock, 0 },
		// { vk::Format::eBc2SrgbBlock, 0 },
		// { vk::Format::eBc3UnormBlock, 0 },
		// { vk::Format::eBc3SrgbBlock, 0 },
		// { vk::Format::eBc4UnormBlock, 0 },
		// { vk::Format::eBc4SnormBlock, 0 },
		// { vk::Format::eBc5UnormBlock, 0 },
		// { vk::Format::eBc5SnormBlock, 0 },
		// { vk::Format::eBc6HUfloatBlock, 0 },
		// { vk::Format::eBc6HSfloatBlock, 0 },
		// { vk::Format::eBc7UnormBlock, 0 },
		// { vk::Format::eBc7SrgbBlock, 0 },
		{ vk::Format::eEtc2R8G8B8UnormBlock, 3 },
		{ vk::Format::eEtc2R8G8B8SrgbBlock, 3 },
		{ vk::Format::eEtc2R8G8B8A1UnormBlock, 3 },
		{ vk::Format::eEtc2R8G8B8A1SrgbBlock, 3 },
		{ vk::Format::eEtc2R8G8B8A8UnormBlock, 4 },
		{ vk::Format::eEtc2R8G8B8A8SrgbBlock, 4 },
		{ vk::Format::eEacR11UnormBlock, 1 },
		{ vk::Format::eEacR11SnormBlock, 1 },
		{ vk::Format::eEacR11G11UnormBlock, 2 },
		{ vk::Format::eEacR11G11SnormBlock, 2 },
		// { vk::Format::eAstc4x4UnormBlock, 0 },
		// { vk::Format::eAstc4x4SrgbBlock, 0 },
		// { vk::Format::eAstc5x4UnormBlock, 0 },
		// { vk::Format::eAstc5x4SrgbBlock, 0 },
		// { vk::Format::eAstc5x5UnormBlock, 0 },
		// { vk::Format::eAstc5x5SrgbBlock, 0 },
		// { vk::Format::eAstc6x5UnormBlock, 0 },
		// { vk::Format::eAstc6x5SrgbBlock, 0 },
		// { vk::Format::eAstc6x6UnormBlock, 0 },
		// { vk::Format::eAstc6x6SrgbBlock, 0 },
		// { vk::Format::eAstc8x5UnormBlock, 0 },
		// { vk::Format::eAstc8x5SrgbBlock, 0 },
		// { vk::Format::eAstc8x6UnormBlock, 0 },
		// { vk::Format::eAstc8x6SrgbBlock, 0 },
		// { vk::Format::eAstc8x8UnormBlock, 0 },
		// { vk::Format::eAstc8x8SrgbBlock, 0 },
		// { vk::Format::eAstc10x5UnormBlock, 0 },
		// { vk::Format::eAstc10x5SrgbBlock, 0 },
		// { vk::Format::eAstc10x6UnormBlock, 0 },
		// { vk::Format::eAstc10x6SrgbBlock, 0 },
		// { vk::Format::eAstc10x8UnormBlock, 0 },
		// { vk::Format::eAstc10x8SrgbBlock, 0 },
		// { vk::Format::eAstc10x10UnormBlock, 0 },
		// { vk::Format::eAstc10x10SrgbBlock, 0 },
		// { vk::Format::eAstc12x10UnormBlock, 0 },
		// { vk::Format::eAstc12x10SrgbBlock, 0 },
		// { vk::Format::eAstc12x12UnormBlock, 0 },
		// { vk::Format::eAstc12x12SrgbBlock, 0 },
		// { vk::Format::eG8B8G8R8422Unorm, 0 },
		// { vk::Format::eB8G8R8G8422Unorm, 0 },
		// { vk::Format::eG8B8R83Plane420Unorm, 0 },
		// { vk::Format::eG8B8R82Plane420Unorm, 0 },
		// { vk::Format::eG8B8R83Plane422Unorm, 0 },
		// { vk::Format::eG8B8R82Plane422Unorm, 0 },
		// { vk::Format::eG8B8R83Plane444Unorm, 0 },
		{ vk::Format::eR10X6UnormPack16, 1 },
		{ vk::Format::eR10X6G10X6Unorm2Pack16, 2 },
		// { vk::Format::eR10X6G10X6B10X6A10X6Unorm4Pack16, 0 },
		// { vk::Format::eG10X6B10X6G10X6R10X6422Unorm4Pack16, 0 },
		// { vk::Format::eB10X6G10X6R10X6G10X6422Unorm4Pack16, 0 },
		{ vk::Format::eG10X6B10X6R10X63Plane420Unorm3Pack16, 3 },
		{ vk::Format::eG10X6B10X6R10X62Plane420Unorm3Pack16, 3 },
		{ vk::Format::eG10X6B10X6R10X63Plane422Unorm3Pack16, 3 },
		{ vk::Format::eG10X6B10X6R10X62Plane422Unorm3Pack16, 3 },
		{ vk::Format::eG10X6B10X6R10X63Plane444Unorm3Pack16, 3 },
		{ vk::Format::eR12X4UnormPack16, 1 },
		{ vk::Format::eR12X4G12X4Unorm2Pack16, 3 },
		// { vk::Format::eR12X4G12X4B12X4A12X4Unorm4Pack16, 0 },
		// { vk::Format::eG12X4B12X4G12X4R12X4422Unorm4Pack16, 0 },
		// { vk::Format::eB12X4G12X4R12X4G12X4422Unorm4Pack16, 0 },
		{ vk::Format::eG12X4B12X4R12X43Plane420Unorm3Pack16, 4 },
		{ vk::Format::eG12X4B12X4R12X42Plane420Unorm3Pack16, 4 },
		{ vk::Format::eG12X4B12X4R12X43Plane422Unorm3Pack16, 4 },
		{ vk::Format::eG12X4B12X4R12X42Plane422Unorm3Pack16, 4 },
		{ vk::Format::eG12X4B12X4R12X43Plane444Unorm3Pack16, 4 },
		{ vk::Format::eG16B16G16R16422Unorm, 8 },
		{ vk::Format::eB16G16R16G16422Unorm, 8 },
		// { vk::Format::eG16B16R163Plane420Unorm, 0 },
		// { vk::Format::eG16B16R162Plane420Unorm, 0 },
		// { vk::Format::eG16B16R163Plane422Unorm, 0 },
		// { vk::Format::eG16B16R162Plane422Unorm, 0 },
		// { vk::Format::eG16B16R163Plane444Unorm, 0 },
		// { vk::Format::eG8B8R82Plane444Unorm, 0 },
		{ vk::Format::eG10X6B10X6R10X62Plane444Unorm3Pack16, 3 },
		{ vk::Format::eG12X4B12X4R12X42Plane444Unorm3Pack16, 4 },
		// { vk::Format::eG16B16R162Plane444Unorm, 0 },
		{ vk::Format::eA4R4G4B4UnormPack16, 2 },
		{ vk::Format::eA4B4G4R4UnormPack16, 2 },
		// { vk::Format::eAstc4x4SfloatBlock, 0 },
		// { vk::Format::eAstc5x4SfloatBlock, 0 },
		// { vk::Format::eAstc5x5SfloatBlock, 0 },
		// { vk::Format::eAstc6x5SfloatBlock, 0 },
		// { vk::Format::eAstc6x6SfloatBlock, 0 },
		// { vk::Format::eAstc8x5SfloatBlock, 0 },
		// { vk::Format::eAstc8x6SfloatBlock, 0 },
		// { vk::Format::eAstc8x8SfloatBlock, 0 },
		// { vk::Format::eAstc10x5SfloatBlock, 0 },
		// { vk::Format::eAstc10x6SfloatBlock, 0 },
		// { vk::Format::eAstc10x8SfloatBlock, 0 },
		// { vk::Format::eAstc10x10SfloatBlock, 0 },
		// { vk::Format::eAstc12x10SfloatBlock, 0 },
		// { vk::Format::eAstc12x12SfloatBlock, 0 },
		// { vk::Format::ePvrtc12BppUnormBlockIMG, 0 },
		// { vk::Format::ePvrtc14BppUnormBlockIMG, 0 },
		// { vk::Format::ePvrtc22BppUnormBlockIMG, 0 },
		// { vk::Format::ePvrtc24BppUnormBlockIMG, 0 },
		// { vk::Format::ePvrtc12BppSrgbBlockIMG, 0 },
		// { vk::Format::ePvrtc14BppSrgbBlockIMG, 0 },
		// { vk::Format::ePvrtc22BppSrgbBlockIMG, 0 },
		// { vk::Format::ePvrtc24BppSrgbBlockIMG, 0 },
		{ vk::Format::eA4B4G4R4UnormPack16EXT, 2 },
		{ vk::Format::eA4R4G4B4UnormPack16EXT, 2 },
		// { vk::Format::eAstc10x10SfloatBlockEXT, 0 },
		// { vk::Format::eAstc10x5SfloatBlockEXT, 0 },
		// { vk::Format::eAstc10x6SfloatBlockEXT, 0 },
		// { vk::Format::eAstc10x8SfloatBlockEXT, 0 },
		// { vk::Format::eAstc12x10SfloatBlockEXT, 0 },
		// { vk::Format::eAstc12x12SfloatBlockEXT, 0 },
		// { vk::Format::eAstc4x4SfloatBlockEXT, 0 },
		// { vk::Format::eAstc5x4SfloatBlockEXT, 0 },
		// { vk::Format::eAstc5x5SfloatBlockEXT, 0 },
		// { vk::Format::eAstc6x5SfloatBlockEXT, 0 },
		// { vk::Format::eAstc6x6SfloatBlockEXT, 0 },
		// { vk::Format::eAstc8x5SfloatBlockEXT, 0 },
		// { vk::Format::eAstc8x6SfloatBlockEXT, 0 },
		// { vk::Format::eAstc8x8SfloatBlockEXT, 0 },
		// { vk::Format::eB10X6G10X6R10X6G10X6422Unorm4Pack16KHR, 0 },
		// { vk::Format::eB12X4G12X4R12X4G12X4422Unorm4Pack16KHR, 0 },
		{ vk::Format::eB16G16R16G16422UnormKHR, 8 },
		// { vk::Format::eB8G8R8G8422UnormKHR, 0 },
		// { vk::Format::eG10X6B10X6G10X6R10X6422Unorm4Pack16KHR, 0 },
		{ vk::Format::eG10X6B10X6R10X62Plane420Unorm3Pack16KHR, 3 },
		{ vk::Format::eG10X6B10X6R10X62Plane422Unorm3Pack16KHR, 3 },
		{ vk::Format::eG10X6B10X6R10X62Plane444Unorm3Pack16EXT, 3 },
		{ vk::Format::eG10X6B10X6R10X63Plane420Unorm3Pack16KHR, 3 },
		{ vk::Format::eG10X6B10X6R10X63Plane422Unorm3Pack16KHR, 3 },
		{ vk::Format::eG10X6B10X6R10X63Plane444Unorm3Pack16KHR, 3 },
		// { vk::Format::eG12X4B12X4G12X4R12X4422Unorm4Pack16KHR, 0 },
		{ vk::Format::eG12X4B12X4R12X42Plane420Unorm3Pack16KHR, 4 },
		{ vk::Format::eG12X4B12X4R12X42Plane422Unorm3Pack16KHR, 4 },
		{ vk::Format::eG12X4B12X4R12X42Plane444Unorm3Pack16EXT, 4 },
		{ vk::Format::eG12X4B12X4R12X43Plane420Unorm3Pack16KHR, 4 },
		{ vk::Format::eG12X4B12X4R12X43Plane422Unorm3Pack16KHR, 4 },
		{ vk::Format::eG12X4B12X4R12X43Plane444Unorm3Pack16KHR, 4 },
		{ vk::Format::eG16B16G16R16422UnormKHR, 8 },
		// { vk::Format::eG16B16R162Plane420UnormKHR, 0 },
		// { vk::Format::eG16B16R162Plane422UnormKHR, 0 },
		// { vk::Format::eG16B16R162Plane444UnormEXT, 0 },
		// { vk::Format::eG16B16R163Plane420UnormKHR, 0 },
		// { vk::Format::eG16B16R163Plane422UnormKHR, 0 },
		// { vk::Format::eG16B16R163Plane444UnormKHR, 0 },
		// { vk::Format::eG8B8G8R8422UnormKHR, 0 },
		// { vk::Format::eG8B8R82Plane420UnormKHR, 0 },
		// { vk::Format::eG8B8R82Plane422UnormKHR, 0 },
		// { vk::Format::eG8B8R82Plane444UnormEXT, 0 },
		// { vk::Format::eG8B8R83Plane420UnormKHR, 0 },
		// { vk::Format::eG8B8R83Plane422UnormKHR, 0 },
		// { vk::Format::eG8B8R83Plane444UnormKHR, 0 },
		// { vk::Format::eR10X6G10X6B10X6A10X6Unorm4Pack16KHR, 0 },
		{ vk::Format::eR10X6G10X6Unorm2Pack16KHR, 2 },
		{ vk::Format::eR10X6UnormPack16KHR, 1 },
		// { vk::Format::eR12X4G12X4B12X4A12X4Unorm4Pack16KHR, 0 },
		{ vk::Format::eR12X4G12X4Unorm2Pack16KHR, 12 },
		{ vk::Format::eR12X4UnormPack16KHR, 2 }
	};

	GraphicsPipelineBuilder::GraphicsPipelineBuilder(Vk::Device& dev) : 
		dev{ dev }
	{}

	GraphicsPipelineBuilder::GraphicsPipelineBuilder(Vk::Device& dev, const vk::Extent2D& Resolution) :
		dev{ dev },
		Viewport{ 0,0, Resolution.width, Resolution.height, 0, 1.0f },
		Scissor{ vk::Offset2D{}, Resolution },
		ViewportState({}, 1, &Viewport, 1, &Scissor)
	{
		
	}

	void GraphicsPipelineBuilder::SetTopology(vk::PrimitiveTopology topo, bool EnableReset) {
		InputAssemblyState.topology = topo;
		InputAssemblyState.primitiveRestartEnable = EnableReset;
	}
	void GraphicsPipelineBuilder::AddVertexShader(const vk::ShaderModule& s, const char* Name) {
		Shaders.emplace_back(vk::PipelineShaderStageCreateInfo{ {}, vk::ShaderStageFlagBits::eVertex, s, Name });
	}
	void GraphicsPipelineBuilder::AddFragmentShader(const vk::ShaderModule& s, const char* Name) {
		Shaders.emplace_back(vk::PipelineShaderStageCreateInfo{ {}, vk::ShaderStageFlagBits::eFragment, s, Name });
	}
	void GraphicsPipelineBuilder::AddGeometryShader(const vk::ShaderModule& s, const char* Name) {
		Shaders.emplace_back(vk::PipelineShaderStageCreateInfo{ {}, vk::ShaderStageFlagBits::eGeometry, s, Name});
	}

	void GraphicsPipelineBuilder::AddVertexFormat(const std::vector<vk::Format>& vf, bool ForInstance, int32_t binding) {
		if (binding<0) binding = VertexInputBinding.size();
		uint32_t offset = 0, index = 0;
		for (auto f : vf) {
			VertexInputAttributes.push_back(vk::VertexInputAttributeDescription(index++, binding, f, offset));
			offset += FormatSize.at(f);
		}
		VertexInputBinding.push_back(vk::VertexInputBindingDescription(binding, offset, ForInstance? vk::VertexInputRate::eInstance: vk::VertexInputRate::eVertex));

		VertexInputState.vertexBindingDescriptionCount = VertexInputBinding.size();
		VertexInputState.pVertexBindingDescriptions = VertexInputBinding.data();
		VertexInputState.vertexAttributeDescriptionCount = VertexInputAttributes.size();
		VertexInputState.pVertexAttributeDescriptions = VertexInputAttributes.data();
	}

	Vk::Pipeline GraphicsPipelineBuilder::Build() {
		info.stageCount = Shaders.size();
		info.pStages = Shaders.data();
		return dev.createGraphicsPipeline(nullptr, info);
	}

	Vk::DeviceMemory MakeBufferMemory(Vk::Device& dev, Vk::Buffer& buf, vk::PhysicalDeviceMemoryProperties& memProps, vk::MemoryPropertyFlags properties) {
		auto req = buf.getMemoryRequirements();
		uint32_t memoryTypeIndex = 0;
		for (memoryTypeIndex = 0; memoryTypeIndex < memProps.memoryTypeCount; memoryTypeIndex++) {
			if ((req.memoryTypeBits & (1 << memoryTypeIndex)) &&
				(memProps.memoryTypes[memoryTypeIndex].propertyFlags&properties) == properties)
				break;
		}
		return dev.allocateMemory(vk::MemoryAllocateInfo(req.size, memoryTypeIndex));
	}

	Buffer::Buffer(Vk::Device& dev, vk::PhysicalDeviceMemoryProperties& memProps, uint32_t qIndex, vk::DeviceSize Size, vk::BufferUsageFlags Usage, vk::SharingMode sMode, vk::MemoryPropertyFlags properties) :
		buffer(dev.createBuffer(vk::BufferCreateInfo({}, Size, Usage, sMode, 1, & qIndex))),
		devMemory(MakeBufferMemory(dev, buffer, memProps, properties)),
		Size{ Size }
	{
		buffer.bindMemory(*devMemory, 0);
	}

	Buffer Device::MakeVertexBuffer(const std::vector<uint8_t>&Data) {
		auto ret = Buffer(_Device, MemoryProps, queueFamily, Data.size(), vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::SharingMode::eExclusive, {});
		if (TransferBuffer.size()<Data.size())
			TransferBuffer = MakeGeneralBuffer(Data.size());
		

		return ret;
	}

	Buffer Device::MakeGeneralBuffer(vk::DeviceSize Size) {
		return Buffer(_Device, MemoryProps, queueFamily, Size, 
			vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive, 
			vk::MemoryPropertyFlagBits::eHostVisible);
	}

	CommandPool Device::MakeResetCommandPool() {
		return CommandPool(_Device, queueFamily, vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
	}

	CommandPool::CommandPool(Vk::Device& dev, uint32_t qIndex, vk::CommandPoolCreateFlagBits flags):
		dev{ dev },
		pool{ dev, vk::CommandPoolCreateInfo(flags, qIndex) }
	{}
	Vk::CommandBuffer CommandPool::CreatePrimary() {
		auto ret = Vk::CommandBuffers(dev, vk::CommandBufferAllocateInfo(*pool, vk::CommandBufferLevel::ePrimary, 1));
		return std::move(ret.at(0));
	}
	Vk::CommandBuffer CommandPool::CreateSecondary() {
		auto ret = Vk::CommandBuffers(dev, vk::CommandBufferAllocateInfo(*pool, vk::CommandBufferLevel::eSecondary, 1));
		return std::move(ret.at(0));
	}
}