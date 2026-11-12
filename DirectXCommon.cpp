#include "DirectXCommon.h"
#include "Logger.h"
#include "StringUtility.h"

#include <cassert>
#include <format>
#include <string>

#ifdef USE_IMGUI
#include "externals/imugi/imgui.h"
#include "externals/imugi/imgui_impl_dx12.h"
#include "externals/imugi/imgui_impl_win32.h"
#endif

#pragma comment(lib,"dxcompiler.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

using Microsoft::WRL::ComPtr;

// ============================================================
// 初期化（全体像）
// ============================================================
void DirectXCommon::Initialize(WinApp* winApp) {
	assert(winApp);
	winApp_ = winApp;

	InitializeDevice();           // デバイスの生成
	InitializeCommand();          // コマンド関連の生成
	CreateSwapChain();            // スワップチェーンの生成
	CreateDepthBuffer();          // 深度バッファの生成
	CreateDescriptorHeaps();      // 各種デスクリプタヒープの生成
	InitializeRenderTargetView(); // レンダーターゲットビューの初期化
	InitializeDepthStencilView(); // 深度ステンシルビューの初期化
	InitializeFence();            // フェンスの生成
	InitializeViewport();         // ビューポート矩形の初期化
	InitializeScissorRect();      // シザリング矩形の生成
	CreateDxcCompiler();          // DXCコンパイラの生成
	InitializeImGui();            // ImGuiの初期化
}

DirectXCommon::~DirectXCommon() {
	if (fenceEvent_) {
		CloseHandle(fenceEvent_);
		fenceEvent_ = nullptr;
	}
}

// ============================================================
// デバイスの生成
// ============================================================
void DirectXCommon::InitializeDevice() {
	HRESULT hr;

#ifdef _DEBUG
	// デバッグレイヤーはデバイス生成「前」に有効にする
	ComPtr<ID3D12Debug1> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		// デバッグレイヤーを有効にする
		debugController->EnableDebugLayer();
		// さらにGPU側のエラーもキャッチするようにする
		/*debugController->SetEnableGPUBasedValidation(TRUE);*/
	}
#endif

	// DXGIファクトリーの生成
	hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory_));
	assert(SUCCEEDED(hr));

	// 利用するアダプター用の変数
	ComPtr<IDXGIAdapter4> useAdapter;
	// 高性能なGPUを優先してアダプターを列挙していく
	for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i,
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) !=
		DXGI_ERROR_NOT_FOUND; ++i) {
		// アダプターの情報を取得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			// ソフトウェアアダプターでなければ採用
			Logger::Log(StringUtility::ConvertString(std::format(L"Use Adapter:{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr;
	}
	assert(useAdapter != nullptr);

	// 機能レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
	};
	const char* featureLevelStrings[] = { "12.2", "12.1", "12.0" };
	// 高い順に生成できるか試していく
	for (size_t i = 0; i < _countof(featureLevels); ++i) {
		hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device_));
		if (SUCCEEDED(hr)) {
			Logger::Log(std::format("FeatureLevel:{}\n", featureLevelStrings[i]));
			break;
		}
	}
	// デバイスが生成できているか確認
	assert(device_ != nullptr);
	Logger::Log("Complete create D3D12Device!!!\n");

#ifdef _DEBUG
	ComPtr<ID3D12InfoQueue> infoQueue;
	if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
		// やばいエラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		// エラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		// 警告時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		// 抑制するメッセージのID
		D3D12_MESSAGE_ID denyIds[] = {
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};
		// 抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		// 指定したメッセージの表示を抑制する
		infoQueue->PushStorageFilter(&filter);
	}
#endif
}

// ============================================================
// コマンド関連の生成（キュー・アロケーター・リスト）
// ============================================================
void DirectXCommon::InitializeCommand() {
	HRESULT hr;

	// コマンドキューの生成
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device_->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue_));
	assert(SUCCEEDED(hr));

	// コマンドアロケーターの生成
	hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
	assert(SUCCEEDED(hr));

	// コマンドリストの生成
	hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator_.Get(), nullptr, IID_PPV_ARGS(&commandList_));
	assert(SUCCEEDED(hr));
}

// ============================================================
// スワップチェーンの生成
// ============================================================
void DirectXCommon::CreateSwapChain() {
	swapChainDesc_.Width = WinApp::kClientWidth;                      // ウィンドウの幅
	swapChainDesc_.Height = WinApp::kClientHeight;                    // ウィンドウの高さ
	swapChainDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM;               // ピクセルフォーマット
	swapChainDesc_.SampleDesc.Count = 1;                              // マルチサンプルしない
	swapChainDesc_.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;     // 描画ターゲットとして使う
	swapChainDesc_.BufferCount = kSwapChainBufferCount;               // バッファ数
	swapChainDesc_.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;        // スワップエフェクト

	// ウィンドウハンドルを渡してスワップチェーンを生成
	HRESULT hr = dxgiFactory_->CreateSwapChainForHwnd(
		commandQueue_.Get(), winApp_->GetHwnd(), &swapChainDesc_, nullptr, nullptr,
		reinterpret_cast<IDXGISwapChain1**>(swapChain_.GetAddressOf()));
	assert(SUCCEEDED(hr));
}

// ============================================================
// 深度バッファの生成
// ============================================================
void DirectXCommon::CreateDepthBuffer() {
	depthStencilResource_ = CreateDepthStencilTextureResource(WinApp::kClientWidth, WinApp::kClientHeight);
}

// ============================================================
// 各種デスクリプタヒープの生成
// ============================================================
void DirectXCommon::CreateDescriptorHeaps() {
	// ディスクリプタ1個あたりのサイズを取得しておく
	descriptorSizeSRV_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descriptorSizeRTV_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	descriptorSizeDSV_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	// RTV用（ダブルバッファ分の2個）
	rtvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kSwapChainBufferCount, false);
	// SRV用（シェーダーから見える）
	srvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
	// DSV用
	dsvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
}

// ============================================================
// レンダーターゲットビューの初期化
// ============================================================
void DirectXCommon::InitializeRenderTargetView() {
	// RTVの設定
	rtvDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc_.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (uint32_t i = 0; i < kSwapChainBufferCount; ++i) {
		// swapChainからResourceを引っ張ってくる
		HRESULT hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&swapChainResources_[i]));
		assert(SUCCEEDED(hr));

		// ヒープ内の i 番目の位置にRTVを作る
		rtvHandles_[i] = GetCPUDescriptorHandle(rtvDescriptorHeap_.Get(), descriptorSizeRTV_, i);
		device_->CreateRenderTargetView(swapChainResources_[i].Get(), &rtvDesc_, rtvHandles_[i]);
	}
}

// ============================================================
// 深度ステンシルビューの初期化
// ============================================================
void DirectXCommon::InitializeDepthStencilView() {
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	device_->CreateDepthStencilView(
		depthStencilResource_.Get(),
		&dsvDesc,
		dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());
}

// ============================================================
// フェンスの生成
// ============================================================
void DirectXCommon::InitializeFence() {
	fenceValue_ = 0;
	HRESULT hr = device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	assert(SUCCEEDED(hr));

	fenceEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent_ != nullptr);
}

// ============================================================
// ビューポート矩形の初期化
// ============================================================
void DirectXCommon::InitializeViewport() {
	viewport_.Width = WinApp::kClientWidth;
	viewport_.Height = WinApp::kClientHeight;
	viewport_.TopLeftX = 0;
	viewport_.TopLeftY = 0;
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;
}

// ============================================================
// シザリング矩形の生成
// ============================================================
void DirectXCommon::InitializeScissorRect() {
	scissorRect_.left = 0;
	scissorRect_.right = WinApp::kClientWidth;
	scissorRect_.top = 0;
	scissorRect_.bottom = WinApp::kClientHeight;
}

// ============================================================
// DXCコンパイラの生成
// ============================================================
void DirectXCommon::CreateDxcCompiler() {
	HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
	assert(SUCCEEDED(hr));

	// 現時点でincludeに対応するための設定
	hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
	assert(SUCCEEDED(hr));
}

// ============================================================
// ImGuiの初期化
// ============================================================
void DirectXCommon::InitializeImGui() {
#ifdef USE_IMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(winApp_->GetHwnd());
	ImGui_ImplDX12_Init(
		device_.Get(),
		swapChainDesc_.BufferCount,
		rtvDesc_.Format,
		srvDescriptorHeap_.Get(),
		srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart());
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Build();
#endif
}

// ============================================================
// 描画前処理
// ============================================================
void DirectXCommon::PreDraw() {
	// 1. バックバッファの番号取得
	UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	// 2. リソースバリアで書き込み可能に変更（PRESENT -> RENDER_TARGET）
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	commandList_->ResourceBarrier(1, &barrier);

	// 3. 描画先のRTVとDSVを指定する
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	commandList_->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], false, &dsvHandle);

	// 4. 画面全体の色をクリア
	float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
	commandList_->ClearRenderTargetView(rtvHandles_[backBufferIndex], clearColor, 0, nullptr);

	// 5. 画面全体の深度をクリア
	commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// 6. SRV用のデスクリプタヒープを指定する
	ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap_.Get() };
	commandList_->SetDescriptorHeaps(1, descriptorHeaps);

	// 7. ビューポート領域の設定
	commandList_->RSSetViewports(1, &viewport_);

	// 8. シザー矩形の設定
	commandList_->RSSetScissorRects(1, &scissorRect_);
}

// ============================================================
// 描画後処理
// ============================================================
void DirectXCommon::PostDraw() {
	// 1. バックバッファの番号取得（PreDrawと同じ処理をコピー）
	UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	// 2. リソースバリアで表示状態に変更（RENDER_TARGET -> PRESENT）
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	commandList_->ResourceBarrier(1, &barrier);

	// 3. グラフィックスコマンドをクローズ
	HRESULT hr = commandList_->Close();
	assert(SUCCEEDED(hr));

	// 4. GPUコマンドの実行
	ID3D12CommandList* commandLists[] = { commandList_.Get() };
	commandQueue_->ExecuteCommandLists(1, commandLists);

	// 5. GPU画面の交換を通知
	swapChain_->Present(1, 0);

	// 6. Fenceの値を更新
	fenceValue_++;

	// 7. コマンドキューにシグナルを送る
	commandQueue_->Signal(fence_.Get(), fenceValue_);

	// 8. コマンド完了待ち
	if (fence_->GetCompletedValue() < fenceValue_) {
		fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
		WaitForSingleObject(fenceEvent_, INFINITE);
	}

	// 9. コマンドアロケーターのリセット
	hr = commandAllocator_->Reset();
	assert(SUCCEEDED(hr));

	// 10. コマンドリストのリセット
	hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
	assert(SUCCEEDED(hr));
}

// ============================================================
// 補助関数
// ============================================================
ComPtr<ID3D12DescriptorHeap> DirectXCommon::CreateDescriptorHeap(
	D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {

	ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device_->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}

ComPtr<ID3D12Resource> DirectXCommon::CreateDepthStencilTextureResource(int32_t width, int32_t height) {
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	ComPtr<ID3D12Resource> resource;
	HRESULT hr = device_->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));

	return resource;
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetCPUDescriptorHandle(
	ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {

	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetGPUDescriptorHandle(
	ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {

	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}

// ============================================================
// SRV専用のデスクリプタハンドル取得（番号だけ指定すればよい）
// ============================================================
D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVCPUDescriptorHandle(uint32_t index) {
	return GetCPUDescriptorHandle(srvDescriptorHeap_.Get(), descriptorSizeSRV_, index);
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVGPUDescriptorHandle(uint32_t index) {
	return GetGPUDescriptorHandle(srvDescriptorHeap_.Get(), descriptorSizeSRV_, index);
}

// ============================================================
// シェーダーのコンパイル
// ============================================================
ComPtr<IDxcBlob> DirectXCommon::CompileShader(
	const std::wstring& filePath,
	const wchar_t* profile) {

	// 1. hlslファイルを読む
	Logger::Log(StringUtility::ConvertString(
		std::format(L"Begin CompileShader, path:{},profile:{}\n", filePath, profile)));

	ComPtr<IDxcBlobEncoding> shaderSource;
	HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	assert(SUCCEEDED(hr));

	// コンパイル対象の情報
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;

	// 2. Compileする
	LPCWSTR arguments[] = {
		filePath.c_str(),
		L"-E", L"main",   // エントリーポイント
		L"-T", profile,   // Shader Profile
		L"-Zi",           // デバッグ情報
		L"-Qembed_debug",
		L"-Od",           // 最適化無効
		L"-Zpr"           // 行列をrow-major
	};

	ComPtr<IDxcResult> shaderResult;
	hr = dxcCompiler_->Compile(
		&shaderSourceBuffer,
		arguments,
		_countof(arguments),
		includeHandler_.Get(),
		IID_PPV_ARGS(&shaderResult));
	assert(SUCCEEDED(hr));

	// 3. 警告・エラー確認
	ComPtr<IDxcBlobUtf8> shaderError;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		Logger::Log(shaderError->GetStringPointer());
		assert(false);
	}

	// 4. Compile結果を取得
	ComPtr<IDxcBlob> shaderBlob;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));

	return shaderBlob;
}

// ============================================================
// バッファリソースの生成
// ============================================================
ComPtr<ID3D12Resource> DirectXCommon::CreateBufferResource(size_t sizeInBytes) {
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeInBytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ComPtr<ID3D12Resource> resource;
	HRESULT hr = device_->CreateCommittedResource(
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(resource.GetAddressOf()));
	assert(SUCCEEDED(hr));

	return resource;
}

// ============================================================
// テクスチャデータの転送
// ============================================================
void DirectXCommon::UploadTextureData(
	const ComPtr<ID3D12Resource>& texture,
	const DirectX::ScratchImage& mipImages) {

	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	for (size_t mipLevel = 0; mipLevel < metadata.mipLevels; ++mipLevel) {
		const DirectX::Image* img = mipImages.GetImage(mipLevel, 0, 0);
		HRESULT hr = texture->WriteToSubresource(
			UINT(mipLevel),
			nullptr,
			img->pixels,
			UINT(img->rowPitch),
			UINT(img->slicePitch));
		assert(SUCCEEDED(hr));
	}
}

// ============================================================
// テクスチャファイルの読み込み
// ============================================================
DirectX::ScratchImage DirectXCommon::LoadTexture(const std::string& filePath) {
	// テクスチャファイルを読み込んでプログラムで扱えるようにする
	DirectX::ScratchImage image{};
	std::wstring filePathW = StringUtility::ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	// ミップマップの作成
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(
		image.GetImages(), image.GetImageCount(), image.GetMetadata(),
		DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));

	// ミップマップ付きのデータを返す
	return mipImages;
}