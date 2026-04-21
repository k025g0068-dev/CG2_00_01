#include <windows.h>
#include <cstdint>
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
	//windowの×ボタンが押されるまでループ
	while (msg.message != WM_QUIT) {
		//windowにメッセージが来たら最優先で処理させる
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			//ゲームの処理
			
		}
		//出力ウィンドウへの文字出力
		OutputDebugStringA("Hello,DirectX!\n");
	}

	



	return 0;
}