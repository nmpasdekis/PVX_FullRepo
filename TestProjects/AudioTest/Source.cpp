#include <PVX_OpenAL.h>
#include <PVX_ffMPEG.h>
#include <PVX_Network.h>
#include <PVX_File.h>
#include <PVX_Encode.h>
#include <PVX_Threading.h>

void CustomViews(PVX::Network::HttpServer& http);
void SqlServices(PVX::Network::HttpServer& http);

int main() {
	PVX::Threading::Pauser pauser;
	PVX::Network::HttpServer http;
	PVX::Network::TcpServer tcp;
	tcp.Serve(http);


	http.EnableWebToken("myAccessKey");

	http.Routes(L"/api/device/output/names", [&](PVX::Network::HttpResponse& resp) {
		resp.Json(PVX::Map(PVX::Audio::Engine::Devices(), [](const std::string& n) {
			return PVX::Decode::Windows1253(n.c_str());
		}));
	});

	http.Routes(L"/api/device/input/names", [&](PVX::Network::HttpResponse& resp) {
		resp.Json(PVX::Map(PVX::Audio::Engine::CaptureDevices(), [](const std::string& n) {
			return PVX::Decode::Windows1253(n.c_str());
		}));
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

	http.Routes(L"/api/exit", [&pauser] {
		pauser.Unpause();
	});
	/*
	http.Routes(L"/api/play", [] {
		Source.Play();
	});

	http.Routes(L"/api/stop", [] {
		Source.Stop();
	});
	*/

	http.Routes(L"/api/data/save/{name}", [](PVX::Network::HttpRequest& req, PVX::Network::HttpResponse& resp) {
		std::wstring name = req[L"name"];
		PVX::IO::Write(L"www\\data\\" + name, req.RawContent);
	});

	http.ContentRoute(L"/js", L"www\\js");
	http.ContentRoute(L"/customViews", L"www\\customViews");
	http.ContentRoute(L"/views", L"www\\views");
	http.ContentRoute(L"/data", L"www\\data");
	http.ContentRoute(L"/css", L"www\\css");
	http.ContentRoute(L"/modals", L"www\\modals");
	http.ServeFile(L"/api/config", L"www\\config\\config.json");
	
	CustomViews(http);
	SqlServices(http);
	
	http.DefaultHtml(L"www\\index.html");



	pauser.Pause();


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