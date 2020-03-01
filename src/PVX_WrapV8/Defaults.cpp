#include <iostream>
#include <string>
#include <PVX_V8.h>
#include <PVX_Encode.h>
#include <PVX_File.h>
#include <PVX_XML.h>
#include <conio.h>


namespace PVX::Javascript {
	static bool echo = false;
	void Defaults(PVX::Javascript::Engine& Engine) {
		using namespace std;
		Engine["input"] = [](std::vector<v8Value>& Params) -> v8Value {
			std::wstring inp;
			std::getline(std::wcin, inp);
			return inp;
		};
		Engine["readJson"] = [&Engine](std::vector<v8Value>& Params) -> v8Value {
			if (Params.size()) {
				auto json = PVX::IO::ReadUtf(Params[0].String());
				if (json.size()) {
					return Engine.parse(json);
				}
			}
			return v8Value::Type::Undefined;
		};
		Engine["writeJson"] = [&Engine](std::vector<v8Value>& Params) -> v8Value {
			if (Params.size()>1) {
				auto json = (Params.size()>2)?
					Engine.stringifyUTF(Params[1], Params[2].String()) :
					Engine.stringifyUTF(Params[1]);

				if (json.size())
					return !!PVX::IO::Write(Params[0].String(), json);
			}
			return false;
		};
		Engine["pause"] = [](std::vector<v8Value>& Params) -> v8Value {
			if (Params.size()) wcout << Params[0].String() << "\n";
			else wcout << L"Press any key to continue\n";
			_getch();
			return v8Value::Type::Undefined;
		};
		Engine["path"] = [](std::vector<v8Value>& Params) -> v8Value {
			if (Params.size()) PVX::IO::CurrentPath(Params[0].String());
			return PVX::IO::wCurrentPath();
		};
		Engine["dir"] = [](std::vector<v8Value>& Params) -> v8Value {
			return PVX::IO::Dir((Params.size()? Params[0].String() : PVX::IO::wCurrentPath()) + L"\\*.*");
		};
		Engine["echo"] = [](std::vector<v8Value>& Params) -> v8Value {
			if (Params.size()) echo = Params[0].Boolean();
			return echo;
		};
		Engine["write"] = [&Engine](std::vector<PVX::Javascript::v8Value>& Params) -> PVX::Javascript::v8Value {
			for (auto& p : Params) {
				std::wcout << p.String();
			}
			return PVX::Javascript::v8Value::Type::Undefined;
		};
		Engine["writeLine"] = [](std::vector<PVX::Javascript::v8Value>& Params) -> PVX::Javascript::v8Value {
			for (auto& p : Params) {
				std::wcout << p.String() << L"\n";
			}
			return PVX::Javascript::v8Value::Type::Undefined;
		};
		Engine["log"] = [&Engine](std::vector<PVX::Javascript::v8Value>& Params) -> PVX::Javascript::v8Value {
			for (auto& p : Params) {
				std::wcout << Engine.stringify(p, L"  ") << L"\n";
			}
			return PVX::Javascript::v8Value::Type::Undefined;
		};
		Engine["run"] = [](std::vector<PVX::Javascript::v8Value>& Params) -> PVX::Javascript::v8Value {
			return std::system(PVX::Encode::ToString(Params[0].String()).c_str());
		};
		Engine["exit"] = [](std::vector<v8Value>& Params) -> v8Value {
			exit(0);
			return {};
		};
		Engine["readText"] = [](std::vector<PVX::Javascript::v8Value>& Params)->PVX::Javascript::v8Value {
			if (Params.size()) {
				auto Filename = Params[0].String();
				if (PVX::IO::FileExists(Filename)) {
					return PVX::IO::ReadUtf(Filename.c_str());
				}
			}
			return PVX::Javascript::v8Value::Type::Undefined;
		};
		Engine["writeText"] = [](std::vector<v8Value>& Params)->v8Value {
			if (Params.size()> 1) {
				auto Filename = Params[0].String();
				PVX::IO::Write(Filename, PVX::Encode::UTF(Params[1].String()));
			}
			return v8Value::Type::Undefined;
		};
		{
			auto& xml = Engine["XML"] = v8Value::Type::Object;
			xml["parse"] = [](std::vector<PVX::Javascript::v8Value>& Params)->PVX::Javascript::v8Value {
				if (Params.size()) {
					auto xml = PVX::XML::Parse(Params[0].String(), Params.size()>1 ? !!Params[0] : false);
					return PVX::Javascript::v8Value::FromJson(PVX::XML::ToJson(xml));
				}
				return PVX::Javascript::v8Value::Type::Undefined;
			};
			xml["stringify"] = [](std::vector<PVX::Javascript::v8Value>& Params)->PVX::Javascript::v8Value {
				if (Params.size()) {
					return PVX::XML::Serialize(PVX::XML::FromJson(Params[0].ToJson()));
				}
				return PVX::Javascript::v8Value::Type::Undefined;
			};
		}
	}
}