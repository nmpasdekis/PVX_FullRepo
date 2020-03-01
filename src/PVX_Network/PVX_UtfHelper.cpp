#include <PVX_Network.h>
#include <PVX_Encode.h>

using namespace PVX;
using namespace PVX::Network;

UtfHelper::UtfHelper(const std::wstring & v) {
	Text = v;
}
PVX::Network::UtfHelper::UtfHelper(const wchar_t * v) {
	Text = v;
}
UtfHelper::UtfHelper(const std::vector<unsigned char> & v) {
	Text = Decode::UTF(v);
}
UtfHelper::UtfHelper(const std::string & v) {
	Text = Decode::Uri(v);
}
UtfHelper & PVX::Network::UtfHelper::operator=(const std::wstring & v) {
	Text = v;
	return *this;
}
std::wstring * PVX::Network::UtfHelper::operator->() {
	return &Text;
}
std::wstring & PVX::Network::UtfHelper::operator()() {
	return Text;
}
UtfHelper::operator std::wstring&() {
	return Text;
}

PVX::Network::UtfHelper::operator std::wstring() const {
	return Text;
}