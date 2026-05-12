#include <windows.h>
#include <cstdint>
#include <string>
#include<filesystem>
#include<fstream>
#include<chrono>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<cassert>
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
//現在時刻を取得
std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
//ログファイルの名前にコンマ何秒入らないので、削って秒にする。
std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
//日本時間（PCの設定時間）に変換
std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSeconds };
//formatを使って年月日_時分秒の文字列に変換
std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
//時刻を使ってファイル名を作成
std::string logFilePath = std::string("log/") + dateString + ".log";
//ファイルを作って書き込む準備
std::ofstream logStream(logFilePath);
std::wstring ConvertString(const std::string& str) {
	if (str.empty()) {
		return std::wstring();
	}

	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
	if (sizeNeeded == 0) {
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}

std::string ConvertString(const std::wstring& str) {
	if (str.empty()) {
		return std::string();
	}

	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if (sizeNeeded == 0) {
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
}

void Log(const std::string& message) {
	OutputDebugStringA(message.c_str());
}

void Log(std::ostream& os, const std::string& message) {
	os << message << std::endl;
	OutputDebugStringA(message.c_str());
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	//メッセージに応じてゲームの固有処理を行う
	switch (msg) {
		//ウィンドウが破棄されたときの処理
	case WM_DESTROY:
		//アプリケーションの終了
		PostQuitMessage(0);
		return 0;
	}
	//標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}



////これから書き込むバックバッファのインデックスを取得
//UINT BackBufferIndex = swapChain->GetCurrentBackBufferIndex();
////描画先のRTV設定する
//commandList->OMSetRenderTargets(1, &rtvHandles[BackBufferIndex], false,nullptr);



// Windowsアプリケーションのエントリーポイント（main関数）
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	WNDCLASS wc{};
	//ウィンドウプロシージャの指定
	wc.lpfnWndProc = WndProc;
	//ウィンドウクラスの名前	
	wc.lpszClassName = L"CG2WindowClass";
	//インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);
	//カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	//ウィンドウクラスの登録
	RegisterClass(&wc);

	//ウィンドウの領域サイズ
	const int32_t kClientWidth = 1280;
	const int32_t kClientHeight = 720;
	//ウィンドウサイズを表す構造体にクライアント領域を入れる
	RECT wrc{ 0,0,kClientWidth,kClientHeight };
	//クライアント領域をもとに実際サイズにwrcを変更してもらう
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	//ウィンドウの作成
	HWND hwnd = CreateWindow(
		wc.lpszClassName,//利用するクラス名
		L"CG2",//タイトルバーの文字(なんでもいい)
		WS_OVERLAPPEDWINDOW,//よく見るウィンドウスタイル
		CW_USEDEFAULT,//表示X座標（OSに任せる）
		CW_USEDEFAULT,//表示Y座標（OSに任せる）
		wrc.right - wrc.left,//ウィンドウ横幅
		wrc.bottom - wrc.top,//ウィンドウ縦幅
		nullptr,//親ウィンドウハンドル
		nullptr,//メニューハンドル
		wc.hInstance,//インスタンスハンドル	
		nullptr);//オプション

	//ウィンドウの表示
	ShowWindow(hwnd, SW_SHOW);

	MSG msg{};
	//ログのディレクトリに関する操作を行うライブラリ
	std::filesystem::create_directory("logs");

	//DXGIファクトリーの生成	
	IDXGIFactory7* dxgiFactory = nullptr;
	//HRESULTはエラーコードで関数が成功かを判断する
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	assert(SUCCEEDED(hr));

	//利用するアダプター用の変数
	IDXGIAdapter4* useAdapter = nullptr;
	//高性能なGPUを優先してアダプターを列挙していく
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i,
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) !=
		DXGI_ERROR_NOT_FOUND; ++i) {
		//アダプターの情報を取得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			//ソフトウェアアダプターでなければ採用
			Log(ConvertString(std::format(L"Use Adapter:{}\n",adapterDesc.Description)));
			break;
		}
		useAdapter=nullptr;
	}
	assert(useAdapter != nullptr);

	ID3D12Device* device = nullptr;
	//昨日レベルトログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[]={
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,		
	};
	const char* featureLevelStrings[] = {
		"12.2",
		"12.1",
		"12.0",
	};
	for (size_t i = 0; i < _countof(featureLevels); ++i)
	{

	}

	//windowの×ボタンが押されるまでループ
	while (msg.message != WM_QUIT) {
		//windowにメッセージが来たら最優先で処理させる

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			//ゲームの処理
				//出力ウィンドウへの文字出力
			OutputDebugStringA("Hello,DirectX!\n");
		}

	}





	return 0;
}