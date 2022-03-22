#include <PVX_OpenAL.h>
#include <PVX_ffMPEG.h>
#include <PVX_Network.h>
#include <PVX_File.h>
#include <PVX_Encode.h>

PVX::Audio::Engine al;
PVX::Audio::Buffer song(PVX::AudioVideo::LoadFile("AfterDark.mp3").ReadAllAudio<short>());
PVX::Audio::Source Source;


int main() {
	PVX::Network::HttpServer http;
	PVX::Network::TcpServer tcp;
	tcp.Serve(http);

	Source.SetBuffer(song);


	http.Routes(L"/api/device/output/names", [&](PVX::Network::HttpResponse& resp) {
		auto devs = PVX::Map(PVX::Audio::Engine::Devices(), [](const std::string& n) {
			return PVX::Decode::Windows1253(n.c_str());
		});
		resp.Json(devs);
	});

	http.Routes(L"/api/device/input/names", [&](PVX::Network::HttpResponse& resp) {
		auto devs = PVX::Map(PVX::Audio::Engine::CaptureDevices(), [](const std::string& n) {
			return PVX::Decode::Windows1253(n.c_str());
		});
		resp.Json(devs);
	});

	http.Routes(L"/api/devices", [&](PVX::Network::HttpResponse& resp) {
		auto devsOut = PVX::Map(PVX::Audio::Engine::Devices(), [](const std::string& n) {
			return PVX::Decode::Windows1253(n.c_str());
		});
		auto devsIn = PVX::Map(PVX::Audio::Engine::CaptureDevices(), [](const std::string& n) {
			return PVX::Decode::Windows1253(n.c_str());
		});
		resp.Json({
			{ L"Output", devsOut },
			{ L"Input", devsIn }
		});
	});

	http.Routes(L"/api/play", [] {
		Source.Play();
	});

	http.Routes(L"/api/stop", [] {
		Source.Stop();
	});

	http.Routes(L"/api/data/save/{name}", [](PVX::Network::HttpRequest& req, PVX::Network::HttpResponse& resp) {
		std::wstring name = req[L"name"];
		PVX::IO::Write(L"www\\data\\" + name, req.RawContent);
	});

	http.ContentRoute(L"/js", L"www\\js");
	http.ContentRoute(L"/js", L"www\\customViews");
	http.ContentRoute(L"/views", L"www\\views");
	http.ContentRoute(L"/data", L"www\\data");
	http.DefaultHtml(L"www\\index.html");

	getchar();


	//auto song = PVX::AudioVideo::LoadFile("AfterDark.mp3");

	//std::vector<short> data;

	//auto aChannels = song.AudioChannels();
	//auto bps = song.AudioBitsPerSample();
	//auto sRate = song.AudioSampleRate();

	//PVX::Audio::StreamOut PlayMusic(aChannels, bps, sRate);

	//PlayMusic.Start();
	//while (song.ReadAudioStream(data)) {
	//	PlayMusic.Stream(data);
	//}
	//PlayMusic.Stop();
	//getchar();

	
	//return 0;


	//Engine.InitCapture(1, 16, 44100);

	//Engine.SetCapture(1024, PVX::Audio::CaptureMono16([&](const short * Samples, int SampleCount) {
	//	SampleCount++;
	//}), 512);

	//Engine.CaptureStart();
	//
	//while (true) {
	//	Engine.Capture();
	//}

	//Engine.CaptureStop();

	return 0;
}