#include <PVX_File.h>
#include <PVX_json.h>
#include <sstream>

int main() {
	auto code = PVX::IO::ReadUtf(L"D:\\GitProjects2022\\PVX_FullRepo\\PVX_Vulkan\\PVX_Vulkan.cpp");

	auto start = std::wregex(LR"reg(.*\/\/\s+Auto Generated\s+\-\s+Start.*)reg", std::wregex::optimize);
	auto end = std::wregex(LR"reg(.*\/\/\s+Auto Generated\s+\-\s+End.*)reg", std::wregex::optimize);
	auto normalize = std::wregex(LR"reg(\r*\n)reg");

	auto mStart = PVX::regex_match(code, start);
	if (mStart.empty())return 0;
	size_t startIndex = mStart.prefix().length() + mStart.length();

	auto mEnd = PVX::regex_match(code, end);
	if (mEnd.empty())return 0;
	size_t endIndex = mEnd.prefix().str().length();

	auto Begin = code.substr(0, startIndex);
	auto End = code.substr(endIndex);

	auto functions = PVX::JSON::parse(PVX::IO::ReadBinary("vulkan.json"));
	std::wstringstream ret;
	/*
	   Vk${c.name} Create${c.name}(${c.params?
                c.params.map(({type, name})=>`${type} ${name}`).join(", "):""}){
    	Vk${c.name}CreateInfo createInfo{};
    	createInfo.sType = VK_STRUCTURE_TYPE_${c.name.toUpperCase()}_CREATE_INFO;
    	createInfo.pNext = nullptr;${c.options?Object.keys(c.options).map(k => `
    	createInfo.${k} = ${c.options[k]};`).join("\n"):""}${c.params?c.params.map(({ name }) => `
    	createInfo.${name} = ${name};`).join("\n"):""}
    
    	Vk${c.name} object;
    	vkCreate${c.name}(&createInfo, nullptr, &object);
    	return object;
    }`).join("\n\n")}
}
	
	)code" << LR"code(
	*/

	auto Params1 = [](const PVX::JSON::Item& f) {
		std::wstringstream ret;
		f.If(L"params", [&](const PVX::JSON::Item& params) {
			ret << PVX::String::Join(params.map_T([](const PVX::JSON::Item& it) {
				return it[L"type"].String() + L" " + it[L"name"].String();
			}), L", ");
		});
		return ret.str();
	};

	auto Params2 = [](const PVX::JSON::Item& f) {
		std::wstringstream ret;
		f.If(L"params", [&](const PVX::JSON::Item& params) {
			params.each([&](const PVX::JSON::Item& it) {
				auto name = it[L"name"].String();
				ret << L"\t\tcreateInfo." + name + L" = " + name + L";\n";
			});
		});
		return ret.str();
	};

	auto Options = [](const PVX::JSON::Item& f) {
		std::wstringstream ret;
		f.If(L"options", [&](const PVX::JSON::Item& options) {
			options.eachInObject([&](const std::wstring& name, const PVX::JSON::Item& it) {
				ret << L"\t\tcreateInfo." + name + L" = " + it.String() + L";\n";
			});
		});
		return ret.str();
	};

	functions.each([&](const PVX::JSON::Item& f) {
		auto name = f[L"name"].String();
		ret << L"\tVk" << name << L" Create(" << Params1(f) << LR"code(){
		Vk)code" << name << LR"code(CreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_)code" << PVX::String::ToUpper(name) << LR"code(_CREATE_INFO;
		createInfo.pNext = nullptr;
)code" << Params2(f) << Options(f) << LR"code(
		Vk)code" << name << LR"code( object;
		vkCreate)code" << name << LR"code((&createInfo, nullptr, &object);
		return object;
	}

)code";
	});


	auto str = std::regex_replace(Begin +L"\n\n" + ret.str() + End, normalize, L"\r\n");

	PVX::IO::Write(L"D:\\GitProjects2022\\PVX_FullRepo\\PVX_Vulkan\\PVX_Vulkan.cpp", PVX::Encode::UTF(str));


	return 0;
}