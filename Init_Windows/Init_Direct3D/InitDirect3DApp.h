#pragma once

#include "D3dApp.h"
#include <DirectXColors.h>
#include "../Common//MathHelper.h"

using namespace DirectX;

// ���� ����
struct Vertex 
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

// ������Ʈ ��� ����
struct ObjectConstants
{
	XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
};

class InitDirect3DApp : public D3DApp
{
public:
	InitDirect3DApp(HINSTANCE hInstance);
	~InitDirect3DApp();

	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void DrawBegin(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;
	virtual void DrawEnd(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

private:
	void BuildInputLayout();
	void BuildGeometry();
	void BuildShader();
	void BuildConstantBuffer();
	void BuildRootSignature();		// ���� ������ ���������� �� �� �ñ״��� ������ֱ� �Ŀ� ���������������� ���������.
	void BuildPSO();		// ���������ν�����Ʈ ��ü �����, ������ ���ٷ���

private:
	// �Է� ��ġ
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	// ��Ʈ �ñ״�ó
	ComPtr<ID3D12RootSignature>					mRootSignature = nullptr;

	// ���������� ���� ��ü
	ComPtr<ID3D12PipelineState>						mPSO = nullptr;

	// ���� ���� ��
	D3D12_VERTEX_BUFFER_VIEW						VertexBufferView = {};
	ComPtr<ID3D12Resource>								VertexBuffer = nullptr;

	// �ε��� ���� ��
	D3D12_INDEX_BUFFER_VIEW							IndexBufferView = {};
	ComPtr<ID3D12Resource>								IndexBuffer = nullptr;

	// ���� ����
	int VertexCount = 0;

	// �ε��� ����
	int IndexCount = 0;

	// ���� ���̴��� �ȼ� ���̴�
	ComPtr<ID3DBlob> mVSByteCode = nullptr;
	ComPtr<ID3DBlob> mPSByteCode = nullptr;

	// ������Ʈ ��� ����
	ComPtr<ID3D12Resource> mObjectCB = nullptr;
	BYTE* mObjectMappedData = nullptr;
	UINT mObjectByteSize = 0;

	// ���� / �þ� / ���� ���
	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	// ���� ��ǥ ���� ��
	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 5.0f;

	// ���콺 ��ǥ
	POINT mLastMovesPos = { 0, 0 };
};