#include <iostream>
#include <PVX_Array.h>
#include <PVX_Lua.h>



int main() {
	using namespace PVX::Scripting;
	//PVX::Array<int> Test {
	//	1, 2, 3
	//};
	//auto x2 = Test.map([](const int& x) {
	//	return x*2;
	//});
	//for (auto& x : x2) {
	//	x*=3;
	//}
	PVX::JSON::Item tbl{
		{ L"member1", 123 },
		{ L"member2", "nikos" },
	};

	Lua lua;
	lua["test2"] = tbl;
	lua["aString"] = "nikos";

	lua["aCppFunc"] = [&](const LuaParams & Params, auto& ret) -> void {
		std::cout << std::string(Params[0]) << "\n";
		Params[1]["test3"]["fromCpp"] = "Hello!!!";
		Params[1]["test3"]["fromCppFnc"] = [](const LuaParams& Params, auto& ret) -> void {
			std::cout << "OMG! It worked!!\n";

			ret << "Returned Hello";
			ret << "Returned World";
		};
		auto ks = Params[2].keys();
		std::cout << "Hello from C++\n";

		auto vals = Params[1]["test4"].values();
		for(auto& v : vals) 
			v = 321;
	};

	auto rez = lua.doString(R"lua(
g = 123;

aDictionary = {
	test1 = 123,
	test2 = "Hello",
	test3 = {
		test31 = 123
	},
	test4 = {
		k1 = 1,
		k2 = 1,
		k3 = 1,
		k4 = 1,
	}
}
aCppFunc('Hello C++', aDictionary, { k1 = 1, k2 = 2, k3 = 3});
print(aDictionary.test3.fromCppFnc());

test = aString .. " hello";
print(test2.member2);
function add(x, y)
	return x + y, x, y;
end
)lua");
	
	std::string x = lua["test"];
	auto add = lua["add"].func<int, int, int>();
	auto [sum, a, b] = add(5, 7);
	auto print = lua.func<>("print");
	print(sum, a, b);

	int64_t g = lua["g"];

	lua["aDictionary"]["test3"]["test32"] = L"hello";

	PVX::JSON::Item dict = lua["aDictionary"];
	//std::wstring str = dict["test2"];

	//print((std::string)dict);
	print(PVX::JSON::stringify(dict, true));

	lua.doString(R"lua(
aCppFunc = nil;
aDictionary = nil;
collectgarbage();
	)lua");

	return 0;
}

int main2() {
	using namespace PVX::Scripting;
	Lua lua;
	lua["aFunction"] = [](auto params, auto& ret) {
		ret << "Hello from C++";
	};
	lua.doString(R"lua(
print(aFunction())

aFunction = nil;
collectgarbage();

	)lua");
	return 0;
}