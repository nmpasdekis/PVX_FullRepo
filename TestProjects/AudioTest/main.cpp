#include <iostream>

#include <PVX_OpenSSL.h>
#include <PVX_OpenAL.h>
#include <PVX_ffMPEG.h>
#include <PVX_Network.h>
#include <PVX_json.h>
#include <PVX_File.h>
#include <thread>
//extern "C" {
//#include <libavutil/avutil.h>
//}

void CustomViews(PVX::Network::HttpServer& http);
void Account(PVX::Network::HttpServer& http);

/*class AudioPlayer {
public:
	PVX::AudioVideo::Media File;
	PVX::Audio::StreamOut Streamer;
	std::thread PlayThread;
	AudioPlayer(const char* Filename) :
		File{ PVX::AudioVideo::LoadFile(Filename) },
		Streamer{ File.AudioChannels(), File.AudioBitsPerSample(), File.AudioSampleRate() },
		PlayThread{ [this] {
			File.SetAudioEvent([this](const std::vector<short>& data) {
				Streamer.Stream(data);
			});
			while (Live && File.DoEvents());
	} } {}
	AudioPlayer(const char* Filename, std::function<void(AudioPlayer&)> onRead) :
		File{ PVX::AudioVideo::LoadFile(Filename) },
		Streamer{ File.AudioChannels(), File.AudioBitsPerSample(), File.AudioSampleRate() },
		PlayThread{ [this, onRead] {
		auto that = this;
		//Streamer.Start();
		File.SetAudioEvent([that, onRead](const std::vector<short>& data) {
			that->Streamer.Stream(data);
			onRead(*that);
		});
		while (Live) {
			while (Live && !File.DoEvents());
			File.Seek(0);
		}
	} } {
	}
	~AudioPlayer() {
		Live = false;
		PlayThread.join();
	}
	operator PVX::Audio::Source& () {
		return Streamer.Get();
	}
	PVX::Audio::Source& operator()() {
		return Streamer.Get();
	}
	inline void Seek(int64_t tm) {
		File.Seek(tm);
	}
protected:
	bool Live = true;
};*/

int main() {
	//PVX::Audio::Engine al;

	std::cout << "Start\n";
	using namespace PVX::Network;
	using namespace PVX::JSON;
	
	
	HttpServer http;	
	TcpServer server("443");
	PVX::Security::OpenSSL ssl("PerVertEX.pem", "PerVertEX.pem");
	server.Serve(http, [&ssl](PVX::Network::TcpSocket& s) {
		ssl.ConvertServerSocket(s);
	});

	TcpServer port80("80");
	port80.Serve(http);


	http.EnableWebToken("myAccessKey");
	
	
	PVX::Threading::Pauser pauser;
	http.Routes(L"/api/exit", [&pauser](HttpRequest& req, HttpResponse& resp) {
		std::cout << "Exiting\n";
		pauser.Unpause();
	});
	http.Routes(L"/api/data/save/{name}", [](PVX::Network::HttpRequest& req, PVX::Network::HttpResponse& resp) {
		std::wstring name = req[L"name"];
		PVX::IO::Write(L"./www/data/" + name, req.RawContent);
	});
	http.Routes(L"/api/data/{name}", [](PVX::Network::HttpRequest& req, PVX::Network::HttpResponse& resp) {
		std::wstring name = L"./www/data/" + req[L"name"];
		if(PVX::IO::FileExists(name) && PVX::IO::FileSize(name) < (1024*1024))
			resp.ServeFile(name);
		else
			resp.StreamFile(req, name);
	});

	http.Routes(L"/api/data", [](PVX::Network::HttpResponse& resp){
		resp.Json(PVX::IO::Dir(L"www/data"));
	});
	
	http.ContentRoute(L"/js", L"www/js");
	http.ContentRoute(L"/customViews", L"www/customViews");
	http.ContentRoute(L"/views", L"www/views");
	http.ContentRoute(L"/data", L"www/data");
	http.ContentRoute(L"/content", L"www/content");
	http.ContentRoute(L"/modals", L"www/modals");
	http.Routes(L"/api/saveConfig", [](PVX::Network::HttpRequest& req) {
		PVX::IO::Write(L"www/config/config.json", req.RawContent);
	});
	http.ServeFile(L"/api/config", L"www/config/config.json");
	
	CustomViews(http);
	Account(http);
	
/*	auto& WebSocketServer = http.CreateWebSocketServer(L"/api/socket");

	AudioPlayer song("Seven Lions & Blastoyz - After Dark (ft. Fiora) [Ophelia Records].mp3", [&WebSocketServer](AudioPlayer& player) {
		WebSocketServer.SendAll([&player](WebSocketPacket& p) {
			p.Run(L"SetTime", { { L"cur", player.File.CurrentTimeSecs() } });
		});
	});

	song().Volume(0.2);

	http.Routes(L"/api/audio/start", [&song](PVX::Network::HttpResponse& resp) {
		if (song.Streamer.IsPlaying()) {
			song.Streamer.Stop();
			resp.Json(PVX::JSON::Item{ L"Stopped" });
		}
		else {
			song.Streamer.Start();
			resp.Json(PVX::JSON::Item{ L"Started" });
		}
	});

	http.Routes(L"/api/audio/seek", [&song](PVX::Network::HttpRequest& req, PVX::Network::HttpResponse& resp) {
		auto ts = req.Json()[L"timestamp"].Integer();
		song.Seek(ts);
	});
*/

	http.DefaultHtml(L"www/index.html");
	
	pauser.Pause();
	//getchar();
	std::cout << "End\n";
	return 0;
}