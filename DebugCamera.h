#pragma once
#include"Math.h"
class DebugCamera {
public:
	void Initialize();
	void Update();

	//X,Y,Z軸回りのローカル回転角
	Matrix4x4 matRot_;
	//ローカル座標
	Vector3 translation_ = { 0,0,-50 };
	//ビュー行列
	Matrix4x4 viewMatrix_;
	//射影行列
	Matrix4x4 projectionMatrix_;

private:
	//行列計算のヘルパー
	Matrix4x4 MakeRotateXMatrix(float radian);
	Matrix4x4 MakeRotateYMatrix(float radian);
	Matrix4x4 MakeRotateZMatrix(float radian);
	Matrix4x4 MakeTranslateMatrix(const Vector3& translate);
	Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m);
	Matrix4x4 Multiply(const Matrix4x4& a, const Matrix4x4& b);
	Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);
	Matrix4x4 Inverse(const Matrix4x4& m);
};