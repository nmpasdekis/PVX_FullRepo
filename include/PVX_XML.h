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
		std::unordered_map<std::wstring, std::wstring> _Attributes;
		std::vector<Element> _Child;

		Element Attributes(const std::unordered_map<std::wstring, std::wstring>& attribs);
		Element Attribute(const std::wstring& Name, const std::wstring& value);
		Element Children(const std::vector<Element>& children);
		Element Child(const Element& child);
		Element WithEnding();
	};
	Element Parse(const std::wstring& Text, bool IsHtml = false);
	std::wstring Serialize(const Element& xml);
	PVX::JSON::Item ToJson(const Element& xml);
	Element FromJson(const PVX::JSON::Item& xml);

	Element Tag(const std::wstring& name);
	Element Tag(const std::wstring& name, const std::vector<Element>& Children);
	Element TagWithAttributes(const std::wstring& name, const std::unordered_map<std::wstring, std::wstring>& Attributes);
	Element TagWithAttributes(const std::wstring& name, const std::unordered_map<std::wstring, std::wstring>& Attributes, const std::vector<Element>& Children);
	Element Text(const std::wstring& text);
	Element Script(const std::wstring& src);
	Element ScriptCode(const std::wstring& src);
	Element Style(const std::wstring& src);
	Element StyleCode(const std::wstring& src);
}