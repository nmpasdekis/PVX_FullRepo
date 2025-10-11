#include <iostream>
#include <PVX_Array.h>
#include <PVX_Lua.h>


int main() {
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
	tbl.IsObject();

	PVX::Scripting::Lua lua;
	lua["test2"] = tbl;
	lua["aString"] = "nikos";

	lua["aCppFunc"] = [](lua_State* L) -> int {
		std::cout << "Hello from C++\n";
		std::string val = lua_tostring(L, -1);
		std::cout << val << "\n";
		std::string val2 = lua_tostring(L, -1);
		std::cout << val << "\n";
		return 0;
	};

	lua.doString(R"(
g = 123;

aCppFunc('Hello C++');
aDictionary = {
	test1 = 123,
	test2 = "Hello",
	test3 = {
		test31 = 123
	}
}

test = aString .. " hello";
print(test2.member2);
function add(x, y)
	return x + y, x, y;
end
)");
	
	std::string x = lua["test"];
	auto add = lua["add"].func<int, int, int>();
	auto [sum, a, b] = add(5, 7);
	auto print = lua.func<>("print");
	print(sum, a, b);

	int64_t g = lua["g"];
	{
		lua["aDictionary"]["test2"];
		auto x = lua["aDictionary"]["test3"]["test31"];
		int64_t y = x;
	}

	lua["aDictionary"]["test3"]["test32"] = L"hello";

	PVX::JSON::Item dict = lua["aDictionary"];
	std::wstring str = dict["test2"];

	print((std::string)dict);
	print(PVX::JSON::stringify(dict, true));
	return 0;
}