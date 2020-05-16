#include <PVX_XML.h>
#include <PVX_String.h>
#include <PVX_Regex.h>
#include <PVX_Encode.h>
#include <sstream>
#include <set>
#include <PVX_json.h>

namespace PVX::XML {
	inline std::vector<std::wstring> RemoveStrings(std::wstring& txt) {
		std::wstring ret;
		std::vector<std::wstring> Strings;
		int Escape = 0;
		int Start = -1;
		int Out = 1;
		for (auto i = 0; i < txt.size(); i++) {
			auto c = txt[i];
			if (c == '\"' && !Escape) {
				if (Out) {
					Start = i +1;
					Out = 0;
				} else {
					Strings.push_back(PVX::Decode::Unescape(txt.substr(Start, i - Start)));
					Out = 1;
				}
			}

			if (Out) ret.push_back(c);
			Escape = c == '\\';
		}
		txt = ret;
		return Strings;
	}
	inline std::vector<std::wstring> RemoveCDATA(std::wstring& ret, int& Error) {
		std::vector<std::wstring> CDATA;
		auto Start = ret.find(L"<![CDATA[");
		while (Start != std::wstring::npos) {
			Start += 9;
			auto End = ret.find(L"]]>");
			if (End==std::wstring::npos) { Error++; break; }
			CDATA.push_back(ret.substr(Start, End-Start));
			ret.replace(Start-9, End - Start + 12, L"<!-->");
			Start = ret.find(L"<![CDATA[", Start - (9 - 5));
		}
		return CDATA;
	}
	inline std::wstring RemoveComments(const std::wstring& txt, int& Error) {
		std::wstring ret = txt;
		auto Start = ret.find(L"<!--");
		while (Start != std::wstring::npos) {
			Start += 4;
			auto End = ret.find(L"-->");
			if (End==std::wstring::npos) { Error++; break; }
			ret.replace(Start-4, End - Start + 7, L"");
			Start = ret.find(L"<!--", Start - 4);
		}
		return ret;
	}
	inline void removeHead(std::wstring& txt, int& Error) {
		size_t c = 0;
		for (; c<txt.size()&&(txt[c]==L' '||txt[c]==L'\t'||txt[c]==L'\r'||txt[c]==L'\n'); c++);
		if (c<(txt.size()-2) && c == txt.find(L"<?xml")) {
			auto end = txt.find(L"?>", c);
			if (end!=std::wstring::npos) {
				txt.replace(c, end - c + 2, L"");
				return;
			}
			Error++;
		}
	}

	//const std::wregex jsonSpaces(LR"regex(\s+)regex", std::regex_constants::optimize);
	//const std::wregex jsonObjectStrings(LR"regex(\"((\\\"|[^\"])*)\")regex", std::regex_constants::optimize);

	const std::wregex  openTag(LR"regex(\s*<([a-zA-Z][-a-zA-Z0-9]*)(\s+([^>/]*))?>)regex", std::regex_constants::optimize);
	const std::wregex  fullTag(LR"regex(\s*<([a-zA-Z][-a-zA-Z0-9]*)(\s+([^>]*))?/>)regex", std::regex_constants::optimize);
	const std::wregex closeTag(LR"regex(\s*</([a-zA-Z][-a-zA-Z0-9]*)\s*>)regex", std::regex_constants::optimize);
	const std::wregex   rCDATA(LR"regex(\s*<!-->)regex", std::regex_constants::optimize);
	const std::wregex  attribs(LR"regex(([a-zA-Z][-a-zA-Z0-9]*)\s*(=\s*"\s)?)regex", std::regex_constants::optimize);


	inline std::vector<Element> Tokenize(const std::wstring& Text, int& Error) {
		std::wstring txt = RemoveComments(Text, Error);
		removeHead(txt, Error);
		auto CDATA = RemoveCDATA(txt, Error);
		auto NextCDATA = 0;
		auto Strings = RemoveStrings(txt);
		auto NextString = 0;
		//txt = std::regex_replace(txt, jsonSpaces, L" ");

		size_t cur = 0;
		std::wsmatch match;

		std::vector<Element> tags;

		while (cur < txt.size()) {
			if (std::regex_search(txt.cbegin()+cur, txt.cend(), match, openTag, std::regex_constants::match_continuous)) {
				std::unordered_map<std::wstring, std::wstring> attrs;
				if (match.size()>3) PVX::onMatch(match[3].str(), attribs, [&](const std::wsmatch& m) { attrs[m[1]] = m.size()>2 ? Strings[NextString++] : L""; });

				tags.push_back({ PVX::XML::Element::ElementType::OpenTag, PVX::String::ToLower(match[1].str()), match[1].str(), attrs });
				cur += match.str().size();
			} else if (std::regex_search(txt.cbegin()+cur, txt.cend(), match, fullTag, std::regex_constants::match_continuous)) {
				std::unordered_map<std::wstring, std::wstring> attrs;
				if (match.size()>3) PVX::onMatch(match[3].str(), attribs, [&](const std::wsmatch& m) { attrs[m[1]] = m.size()>2 ? Strings[NextString++] : L""; });

				tags.push_back({ PVX::XML::Element::ElementType::Tag, PVX::String::ToLower(match[1].str()), match[1].str(), attrs });
				cur += match.str().size();
			} else if (std::regex_search(txt.cbegin()+cur, txt.cend(), match, closeTag, std::regex_constants::match_continuous)) {
				tags.push_back({ PVX::XML::Element::ElementType::CloseTag, PVX::String::ToLower(match[1].str()), match[1].str() });
				cur += match.str().size();
			} else if (std::regex_search(txt.cbegin()+cur, txt.cend(), match, rCDATA, std::regex_constants::match_continuous)) {
				tags.push_back({ PVX::XML::Element::ElementType::CDATA, L"", CDATA[NextCDATA++] });
				cur += match.str().size();
			} else {
				auto end = txt.find(L"<", cur);
				if (end==std::wstring::npos) end = txt.size();
				tags.push_back({ PVX::XML::Element::ElementType::Text, L"", txt.substr(cur, end-cur) });
				cur = end;
			}
		}
		return tags;
	}

	Element Parse(const std::wstring& Text, bool IsHtml) {
		int Error = 0;
		auto Tokens = Tokenize(Text, Error);

		if (IsHtml) {
			std::set<std::wstring> singles{ L"img" };
			size_t i = 1;
			for (auto& t : Tokens) {
				if (t.Name==L"br") t.Type = Element::ElementType::Tag;
				else if (singles.count(t.Name)) {
					if (t.Type==Element::ElementType::OpenTag && i<Tokens.size() && Tokens[i].Type==Element::ElementType::CloseTag&&Tokens[i].Name==t.Name) {
						Tokens[i].Type = Element::ElementType::Discard;
					}
					t.Type = Element::ElementType::HtmlSingle;
				}
				i++;
			}
		}

		if (!Error && Tokens.size() && (Tokens[0].Type == Element::ElementType::Tag || Tokens[0].Type == Element::ElementType::OpenTag)) {
			std::vector<Element> Stack;
			Stack.push_back(Tokens[0]);

			for (auto i = 1; i<Tokens.size() && Stack.size(); i++) {
				auto& cur = Tokens[i];
				if (cur.Type==Element::ElementType::Discard)continue;
				if (!(cur.Type == Element::ElementType::OpenTag || cur.Type == Element::ElementType::CloseTag)) {
					Stack.back()._Child.push_back(cur);
				} else if (cur.Type==Element::ElementType::OpenTag) {
					Stack.push_back(cur);
				} else if (cur.Type==Element::ElementType::CloseTag && cur.Name==Stack.back().Name) {
					if (Stack.size()==1)
						return Stack[0];
					auto bk = Stack.back();
					Stack.pop_back();
					Stack.back()._Child.push_back(bk);
				} else {
					return Element();
				}
			}
		}
		return Element();
	}

	static void _Print(const Element& xml, int level, std::wstringstream& out) {
		auto tp = xml.Type;
		if (tp==Element::ElementType::Tag&xml._Child.size())tp = Element::ElementType::OpenTag;
		switch (tp) {
			case PVX::XML::Element::ElementType::CDATA:
				out << L"<![CDATA[" << xml.Text << L"]]>";
				break;
			case PVX::XML::Element::ElementType::Text:
				out << xml.Text;
				break;
			case PVX::XML::Element::ElementType::Tag:
				out << L"\n" << std::wstring(level<<1, L' ') << L"<" << xml.Text;
				for (auto& [Name, Value] : xml._Attributes)
					out << L" " << Name << L"=\"" << Value << L"\"";
				out << L" />";
				break;
			case PVX::XML::Element::ElementType::OpenTag:
				out << L"\n" << std::wstring(level<<1, L' ') << L"<" << xml.Text;
				for (auto& [Name, Value] : xml._Attributes)
					out << L" " << Name << L"=\"" << Value << L"\"";
				out << L">";
				if (xml._Child.size()) {
					for (auto& c : xml._Child) {
						_Print(c, level + 1, out);
					}
					if (xml._Child[0].Type != PVX::XML::Element::ElementType::CDATA && xml._Child[0].Type != PVX::XML::Element::ElementType::Text)
						out << L"\n" << std::wstring(level<<1, L' ');
				}
				out << L"</" << xml.Text << L">";
				break;
			case PVX::XML::Element::ElementType::HtmlSingle:
				out << L"\n" << std::wstring(level<<1, L' ') << L"<" << xml.Text;
				for (auto& [Name, Value] : xml._Attributes)
					out << L" " << Name << L"=\"" << Value << L"\"";
				out << L">";
				break;
		}
	}

	Element Tag(const std::wstring& name) {
		return std::move(Element{ Element::ElementType::Tag, PVX::String::ToLower(name), name });
	}
	Element Tag(const std::wstring& name, const std::vector<Element>& _Children) {
		return std::move(Element{ Element::ElementType::OpenTag, PVX::String::ToLower(name), name, {}, _Children });
	}
	Element TagWithAttributes(const std::wstring& name, const std::unordered_map<std::wstring, std::wstring>& _Attributes){
		return std::move(Element{ Element::ElementType::Tag, PVX::String::ToLower(name), name, _Attributes });
	}
	Element TagWithAttributes(const std::wstring& name, const std::unordered_map<std::wstring, std::wstring>& _Attributes, const std::vector<Element>& _Children) {
		return std::move(Element{ Element::ElementType::OpenTag, PVX::String::ToLower(name), name, _Attributes, _Children });
	}
	Element Text(const std::wstring& text) {
		return std::move(Element{ Element::ElementType::Text, {}, text });
	}

	Element Script(const std::wstring& src) { return std::move(Element{ Element::ElementType::OpenTag, L"script", L"script", { { L"src", src } } }); }
	Element ScriptCode(const std::wstring& src) { return std::move(Element{ Element::ElementType::OpenTag, L"script", L"script", {}, { Text(src) } }); }
	Element Style(const std::wstring& src) { return std::move(Element{ Element::ElementType::OpenTag, L"link", L"link", { { L"href", src } } }); }
	Element StyleCode(const std::wstring& src) { return std::move(Element{ Element::ElementType::OpenTag, L"style", L"style", {}, { Text(src) } }); }

	std::wstring Serialize(const Element& xml) {
		std::wstringstream out;
		_Print(xml, 0, out);
		return out.str();
	}
	static const std::array<std::wstring, 7> JsonType{
		L"Tag", L"Text", L"CDATA", L"HtmlSingle", L"OpenTag", L"CloseTag", L"Discard"
	};
	PVX::JSON::Item ToJson(const Element& xml) {
		PVX::JSON::Item ret = PVX::JSON::jsElementType::Object;
		ret[L"Type"] = JsonType[(int)xml.Type];
		switch (xml.Type) {
			case Element::ElementType::Tag:
			case Element::ElementType::OpenTag:
			case Element::ElementType::HtmlSingle: {
				ret[L"Name"] = xml.Text;
				if (xml._Attributes.size()) {
					ret[L"Attributes"] = xml._Attributes;
				}
				if (xml._Child.size()) {
					auto& _Children = ret["Children"];
					_Children = PVX::JSON::jsElementType::Array;
					for (auto& x: xml._Child) _Children.push(ToJson(x));
				}
				break;
			}
			case Element::ElementType::Text:
			case Element::ElementType::CDATA:
				ret[L"Value"] = xml.Text;
				break;
		}
		return ret;
	}
	static const std::unordered_map<std::wstring, Element::ElementType> FromJsonType{
		{ L"Tag", Element::ElementType::Tag },
		{ L"Text", Element::ElementType::Text },
		{ L"CDATA", Element::ElementType::CDATA },
		{ L"HtmlSingle", Element::ElementType::HtmlSingle },
		{ L"OpenTag", Element::ElementType::OpenTag },
		{ L"CloseTag", Element::ElementType::CloseTag },
		{ L"Discard", Element::ElementType::Discard }
	};
	Element::ElementType GetType(const PVX::JSON::Item& xml) {
		const PVX::JSON::Item def = L"Discard";
		return FromJsonType.at(xml.Get(L"Type", def).GetString());
	}
	Element FromJson(const PVX::JSON::Item& xml) {
		Element ret;
		ret.Type = GetType(xml);
		if (auto &Value = *xml.Has(L"Value"); &Value) {
			ret.Text = Value.GetString();
		} else {
			ret.Text = xml.Get(L"Name").GetString();
			ret.Name = PVX::String::ToLower(ret.Text);
			if (auto &attributes = *xml.Has(L"_Attributes"); &attributes) {
				attributes.eachInObject([&](auto Name, auto Value) {
					ret._Attributes[Name] = Value.GetString();
				});
			}
			if (auto & children = *xml.Has(L"Children"); &children) {
				children.each([&](const auto& Child) {
					ret._Child.push_back(FromJson(Child));
				});
			}
		}
		return ret;
	}

	Element Element::Attributes(const std::unordered_map<std::wstring, std::wstring>& Attribs) {
		for (auto& [Name, Value] : Attribs) _Attributes[Name] = Value;
		return std::move(*this);
	}
	Element Element::Attribute(const std::wstring& Name, const std::wstring& Value) {
		_Attributes[Name] = Value;
		return std::move(*this);
	}
	Element Element::Children(const std::vector<Element>& _Children) {
		for (auto& c : _Children) this->_Child.push_back(c);
		return std::move(*this);
	}
	Element Element::Child(const Element& c) {
		this->_Child.push_back(c);
		return std::move(*this);
	}
	Element Element::WithEnding() {
		Type = ElementType::OpenTag;
		return std::move(*this);
	}
}