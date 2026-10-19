#include "Input.h"
#include <cassert>

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")

void Input::Initialize(HINSTANCE hInstance, HWND hwnd) {
	HRESULT result;

	//DirectInputの初期化	
	result = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8,
		(void**)&directInput, nullptr);
	assert(SUCCEEDED(result));

	//キーボードデバイスの生成	
	result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(result));
	//入力データの形式のセット
	result = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(result));

	result = keyboard->SetCooperativeLevel(
		hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));
}

void Input::Update() {
	memcpy(keyPre, key, sizeof(key));

	keyboard->Acquire();
	keyboard->GetDeviceState(sizeof(key), key);
}

bool Input::PushKey(BYTE keyNumber) {
	return key[keyNumber] != 0;
}

bool Input::TriggerKey(BYTE keyNumber) {
	return key[keyNumber] && !keyPre[keyNumber];
}