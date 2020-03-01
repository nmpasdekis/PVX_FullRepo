#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <PVX_json.h>

namespace PVX::XML {
	class Element {
	public:
		enum class ElementType {
			Tag, Text, CDATA, HtmlSingle, OpenTag, CloseTag, Discard
		};
		ElementType Type;
		std::wstring Name;
		std::wstring Text;
		std::unordered_map<std::wstring, std::wstring> Attributes;
		std::vector<Element> Child;
	};
	Element Parse(const std::wstring& Text, bool IsHtml = false);
	std::wstring Serialize(const Element& xml);
	PVX::JSON::Item ToJson(const Element& xml);
	Element FromJson(const PVX::JSON::Item& xml);
}