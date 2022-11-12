#include <shaderc/shaderc.hpp>
#include <vector>
#include <string>
#include <PVX_Encode.h>
#include <PVX_File.h>
#include <vulkan/vulkan_raii.hpp>

#ifndef NDEBUG
#include <iostream>
#include "..\..\include\PVX_Vulkan.h"
#endif


namespace PVX::SpirV {
	std::vector<uint32_t> Compile(const std::string& Code, vk::ShaderStageFlagBits Type, const std::string_view& filename) {
		shaderc_shader_kind type = [](vk::ShaderStageFlagBits tp) {
			switch (tp) {
				case vk::ShaderStageFlagBits::eVertex: return shaderc_shader_kind::shaderc_glsl_vertex_shader;
				case vk::ShaderStageFlagBits::eTessellationControl: return shaderc_shader_kind::shaderc_tess_control_shader;
				case vk::ShaderStageFlagBits::eTessellationEvaluation: return shaderc_shader_kind::shaderc_glsl_tess_evaluation_shader;
				case vk::ShaderStageFlagBits::eGeometry: return shaderc_shader_kind::shaderc_glsl_geometry_shader;
				case vk::ShaderStageFlagBits::eFragment: return shaderc_shader_kind::shaderc_glsl_fragment_shader;
				case vk::ShaderStageFlagBits::eCompute: return shaderc_shader_kind::shaderc_glsl_compute_shader;
				case vk::ShaderStageFlagBits::eRaygenKHR: return shaderc_shader_kind::shaderc_glsl_raygen_shader;
				case vk::ShaderStageFlagBits::eAnyHitKHR: return shaderc_shader_kind::shaderc_glsl_anyhit_shader;
				case vk::ShaderStageFlagBits::eClosestHitKHR: return shaderc_shader_kind::shaderc_glsl_closesthit_shader;
				case vk::ShaderStageFlagBits::eMissKHR: return shaderc_shader_kind::shaderc_glsl_miss_shader;
				case vk::ShaderStageFlagBits::eIntersectionKHR: return shaderc_shader_kind::shaderc_glsl_intersection_shader;
				case vk::ShaderStageFlagBits::eCallableKHR: return shaderc_shader_kind::shaderc_glsl_callable_shader;
				case vk::ShaderStageFlagBits::eTaskNV: return shaderc_shader_kind::shaderc_glsl_task_shader;
				case vk::ShaderStageFlagBits::eMeshNV: return shaderc_shader_kind::shaderc_glsl_mesh_shader;

				default: return shaderc_shader_kind(0);
			}
		}(Type);

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetOptimizationLevel(shaderc_optimization_level::shaderc_optimization_level_performance);
		auto res = compiler.CompileGlslToSpv(Code, type, filename.data(), options);
#ifndef NDEBUG
		if (res.GetNumWarnings()!=0) {
			std::cout << res.GetErrorMessage() << "\n";
		}
#endif

		if (res.GetCompilationStatus() != shaderc_compilation_status::shaderc_compilation_status_success) {
			auto msg = res.GetErrorMessage();
			throw msg;
		}
		return std::vector<uint32_t>(res.cbegin(), res.cend());
	}
	std::vector<uint32_t> CompileFile(const std::filesystem::path& Filename) {
		std::unordered_map<std::string, vk::ShaderStageFlagBits> ExtesnionMap{
			{ ".vert", vk::ShaderStageFlagBits::eVertex },
			{ ".frag", vk::ShaderStageFlagBits::eFragment },
			{ ".geom", vk::ShaderStageFlagBits::eGeometry },
		};
		return Compile(PVX::IO::ReadText(Filename), ExtesnionMap.at(Filename.extension().string()), Filename.filename().string().c_str());
	}
}