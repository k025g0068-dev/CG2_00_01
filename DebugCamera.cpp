#include "DebugCamera.h"
#include <cmath>
#include <algorithm>
#include <Windows.h> 
#include"Math.h"

// ---------------- ヘルパー関数 ----------------

Matrix4x4 DebugCamera::MakeRotateXMatrix(float radian) {
	Matrix4x4 result = {};
	float s = std::sin(radian), c = std::cos(radian);
	result.m[0][0] = 1;
	result.m[1][1] = c;  result.m[1][2] = s;
	result.m[2][1] = -s; result.m[2][2] = c;
	result.m[3][3] = 1;
	return result;
}

Matrix4x4 DebugCamera::MakeRotateYMatrix(float radian) {
	Matrix4x4 result = {};
	float s = std::sin(radian), c = std::cos(radian);
	result.m[0][0] = c;  result.m[0][2] = -s;
	result.m[1][1] = 1;
	result.m[2][0] = s;  result.m[2][2] = c;
	result.m[3][3] = 1;
	return result;
}

Matrix4x4 DebugCamera::MakeRotateZMatrix(float radian) {
	Matrix4x4 result = {};
	float s = std::sin(radian), c = std::cos(radian);
	result.m[0][0] = c;  result.m[0][1] = s;
	result.m[1][0] = -s; result.m[1][1] = c;
	result.m[2][2] = 1;
	result.m[3][3] = 1;
	return result;
}

Matrix4x4 DebugCamera::Multiply(const Matrix4x4& a, const Matrix4x4& b) {
	Matrix4x4 result = {};
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			for (int k = 0; k < 4; k++) {
				result.m[i][j] += a.m[i][k] * b.m[k][j];
			}
		}
	}
	return result;
}

Matrix4x4 DebugCamera::MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {
	Matrix4x4 rotateXYZ = Multiply(MakeRotateXMatrix(rotate.x),
		Multiply(MakeRotateYMatrix(rotate.y), MakeRotateZMatrix(rotate.z)));

	Matrix4x4 result = rotateXYZ;
	// スケール適用
	for (int i = 0; i < 3; i++) {
		float s = (i == 0) ? scale.x : (i == 1) ? scale.y : scale.z;
		result.m[i][0] *= s; result.m[i][1] *= s; result.m[i][2] *= s;
	}
	// 平行移動を4行目に代入
	result.m[3][0] = translate.x;
	result.m[3][1] = translate.y;
	result.m[3][2] = translate.z;
	result.m[3][3] = 1.0f;
	return result;
}

Matrix4x4 DebugCamera::Inverse(const Matrix4x4& m) {
	// 4x4逆行列(余因子展開による一般的な実装)
	Matrix4x4 result = {};
	float a[16];
	int idx = 0;
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			a[idx++] = m.m[i][j];

	float inv[16];
	inv[0] = a[5] * a[10] * a[15] - a[5] * a[11] * a[14] - a[9] * a[6] * a[15] + a[9] * a[7] * a[14] + a[13] * a[6] * a[11] - a[13] * a[7] * a[10];
	inv[4] = -a[4] * a[10] * a[15] + a[4] * a[11] * a[14] + a[8] * a[6] * a[15] - a[8] * a[7] * a[14] - a[12] * a[6] * a[11] + a[12] * a[7] * a[10];
	inv[8] = a[4] * a[9] * a[15] - a[4] * a[11] * a[13] - a[8] * a[5] * a[15] + a[8] * a[7] * a[13] + a[12] * a[5] * a[11] - a[12] * a[7] * a[9];
	inv[12] = -a[4] * a[9] * a[14] + a[4] * a[10] * a[13] + a[8] * a[5] * a[14] - a[8] * a[6] * a[13] - a[12] * a[5] * a[10] + a[12] * a[6] * a[9];
	inv[1] = -a[1] * a[10] * a[15] + a[1] * a[11] * a[14] + a[9] * a[2] * a[15] - a[9] * a[3] * a[14] - a[13] * a[2] * a[11] + a[13] * a[3] * a[10];
	inv[5] = a[0] * a[10] * a[15] - a[0] * a[11] * a[14] - a[8] * a[2] * a[15] + a[8] * a[3] * a[14] + a[12] * a[2] * a[11] - a[12] * a[3] * a[10];
	inv[9] = -a[0] * a[9] * a[15] + a[0] * a[11] * a[13] + a[8] * a[1] * a[15] - a[8] * a[3] * a[13] - a[12] * a[1] * a[11] + a[12] * a[3] * a[9];
	inv[13] = a[0] * a[9] * a[14] - a[0] * a[10] * a[13] - a[8] * a[1] * a[14] + a[8] * a[2] * a[13] + a[12] * a[1] * a[10] - a[12] * a[2] * a[9];
	inv[2] = a[1] * a[6] * a[15] - a[1] * a[7] * a[14] - a[5] * a[2] * a[15] + a[5] * a[3] * a[14] + a[13] * a[2] * a[7] - a[13] * a[3] * a[6];
	inv[6] = -a[0] * a[6] * a[15] + a[0] * a[7] * a[14] + a[4] * a[2] * a[15] - a[4] * a[3] * a[14] - a[12] * a[2] * a[7] + a[12] * a[3] * a[6];
	inv[10] = a[0] * a[5] * a[15] - a[0] * a[7] * a[13] - a[4] * a[1] * a[15] + a[4] * a[3] * a[13] + a[12] * a[1] * a[7] - a[12] * a[3] * a[5];
	inv[14] = -a[0] * a[5] * a[14] + a[0] * a[6] * a[13] + a[4] * a[1] * a[14] - a[4] * a[2] * a[13] - a[12] * a[1] * a[6] + a[12] * a[2] * a[5];
	inv[3] = -a[1] * a[6] * a[11] + a[1] * a[7] * a[10] + a[5] * a[2] * a[11] - a[5] * a[3] * a[10] - a[9] * a[2] * a[7] + a[9] * a[3] * a[6];
	inv[7] = a[0] * a[6] * a[11] - a[0] * a[7] * a[10] - a[4] * a[2] * a[11] + a[4] * a[3] * a[10] + a[8] * a[2] * a[7] - a[8] * a[3] * a[6];
	inv[11] = -a[0] * a[5] * a[11] + a[0] * a[7] * a[9] + a[4] * a[1] * a[11] - a[4] * a[3] * a[9] - a[8] * a[1] * a[7] + a[8] * a[3] * a[5];
	inv[15] = a[0] * a[5] * a[10] - a[0] * a[6] * a[9] - a[4] * a[1] * a[10] + a[4] * a[2] * a[9] + a[8] * a[1] * a[6] - a[8] * a[2] * a[5];

	float det = a[0] * inv[0] + a[1] * inv[4] + a[2] * inv[8] + a[3] * inv[12];
	if (det == 0.0f) return result; // 逆行列なし

	det = 1.0f / det;
	idx = 0;
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			result.m[i][j] = inv[idx++] * det;

	return result;
}

Matrix4x4 DebugCamera::MakeTranslateMatrix(const Vector3& translate) {
	Matrix4x4 result = {};
	result.m[0][0] = 1;
	result.m[1][1] = 1;
	result.m[2][2] = 1;
	result.m[3][3] = 1;
	result.m[3][0] = translate.x;
	result.m[3][1] = translate.y;
	result.m[3][2] = translate.z;
	return result;
}

Vector3 DebugCamera::TransformNormal(const Vector3& v, const Matrix4x4& m) {
	Vector3 result;
	result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0];
	result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1];
	result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2];
	return result;
}

void DebugCamera::Initialize() {
	matRot_ = Matrix4x4{}; // 念のため全部0にクリア
	matRot_.m[0][0] = 1.0f;
	matRot_.m[1][1] = 1.0f;
	matRot_.m[2][2] = 1.0f;
	matRot_.m[3][3] = 1.0f;
}

void DebugCamera::Update() {
	// ① 入力によるカメラの移動・回転 -----------------------

	const float moveSpeed = 0.5f;
	const float rotateSpeed = 0.02f;

	float rotateX = 0.0f, rotateY = 0.0f, rotateZ = 0.0f;
	if (GetAsyncKeyState(VK_LEFT) & 0x8000)  rotateY -= rotateSpeed;
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000) rotateY += rotateSpeed;
	if (GetAsyncKeyState(VK_UP) & 0x8000)    rotateX -= rotateSpeed;
	if (GetAsyncKeyState(VK_DOWN) & 0x8000)  rotateX += rotateSpeed;
	if (GetAsyncKeyState('Z') & 0x8000)      rotateZ -= rotateSpeed;
	if (GetAsyncKeyState('X') & 0x8000)      rotateZ += rotateSpeed;

	// 今フレームの増分回転行列を作る
	Matrix4x4 deltaRotMatrix = Multiply(MakeRotateXMatrix(rotateX),
		Multiply(MakeRotateYMatrix(rotateY), MakeRotateZMatrix(rotateZ)));

	// 累積回転行列に掛け合わせる(ローカル軸で回したいのでmatRot_を先に掛ける)
	matRot_ = Multiply(matRot_, deltaRotMatrix);

	// 移動(WASDで前後左右、Q/Eで上下の例)
	Vector3 move = { 0,0,0 };
	if (GetAsyncKeyState('W') & 0x8000) move.z += moveSpeed;
	if (GetAsyncKeyState('S') & 0x8000) move.z -= moveSpeed;

	translation_.y += move.y;
	translation_.z += move.z;

	if ((GetAsyncKeyState('W') & 0x8000) || (GetAsyncKeyState('S') & 0x8000)) {
		float speed = 0.0f;
		if (GetAsyncKeyState('W') & 0x8000) {
			speed = moveSpeed;
		}
		else if (GetAsyncKeyState('S') & 0x8000) {
			speed = -moveSpeed;
		}

		Vector3 forwardMove = { 0,0,speed };

		forwardMove = TransformNormal(forwardMove, matRot_);

		translation_.x += forwardMove.x;
		translation_.y += forwardMove.y;
		translation_.z += forwardMove.z;

	}

	if ((GetAsyncKeyState('E') & 0x8000) || (GetAsyncKeyState('Q') & 0x8000)) {
		float upSpeed = 0.0f;
		if (GetAsyncKeyState('E') & 0x8000) {
			upSpeed = moveSpeed;
		}
		else if (GetAsyncKeyState('Q') & 0x8000) {
			upSpeed = -moveSpeed;
		}

		Vector3 upMove = { 0, upSpeed, 0 };


		upMove = TransformNormal(upMove, matRot_);

		translation_.x += upMove.x;
		translation_.y += upMove.y;
		translation_.z += upMove.z;
	}

	if ((GetAsyncKeyState('D') & 0x8000) || (GetAsyncKeyState('A') & 0x8000)) {
		float rightSpeed = 0.0f;
		if (GetAsyncKeyState('D') & 0x8000) {
			rightSpeed = moveSpeed;
		}
		else if (GetAsyncKeyState('A') & 0x8000) {
			rightSpeed = -moveSpeed;
		}

		Vector3 rightMove = { rightSpeed,0,0 };

		rightMove = TransformNormal(rightMove, matRot_);

		translation_.x += rightMove.x;
		translation_.y += rightMove.y;
		translation_.z += rightMove.z;
	}


	// ② ビュー行列の更新 -----------------------------------

	Matrix4x4 translateMatrix = MakeTranslateMatrix(translation_);
	Matrix4x4 worldMatrix = Multiply(matRot_, translateMatrix);
	viewMatrix_ = Inverse(worldMatrix);
}


