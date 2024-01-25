#pragma once

#include "D3dApp.h"
#include <DirectXColors.h>
#include "../Common//MathHelper.h"
#include "../Common//GeometryGenerator.h"

using namespace DirectX;

// ���� ����
struct Vertex 
{
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
};

// ���� ������Ʈ ��� ���� (������Ʈ ������ŭ)
struct ObjectConstants
{
	XMFLOAT4X4 World = MathHelper::Identity4x4();
};

// ���� ���� ��� ����
struct MaterialsConstants
{
	XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	XMFLOAT3 FresnelIR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;
};

// ���� ��� ���� (�ϳ��� ������.)
struct PassConstants
{
	XMFLOAT4X4 View = MathHelper::Identity4x4();
	XMFLOAT4X4 InvView = MathHelper::Identity4x4();		// ī�޶�, �ι��� ī�޶�
	XMFLOAT4X4 Proj = MathHelper::Identity4x4();
	XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
	XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();		// ����
};

// ���ϵ��� ����
struct GeometryInfo
{
	std::string Name;

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
};

// ��¡ ����
struct MaterialInfo
{
	std::string Name;

	int MatCBIndex = -1;
	int DiffuseSrvHeapIndex = -1;
	int Texture_On = 0;

	XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };		// ������
	float Roughness = 0.25f;		// ��ĥ��
};

// �������� ������Ʈ ����ü
struct RenderItem
{
	RenderItem() = default;

	UINT ObjCBIndex = -1;

	XMFLOAT4X4 World = MathHelper::Identity4x4();
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	GeometryInfo* Geo = nullptr;
	MaterialInfo* Mat = nullptr;

	int IndexCount = 0;
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
	void UpdateCamera(const GameTimer& gt);
	void UpdateObjectCB(const GameTimer& gt);
	void UpdateMaterialCB(const GameTimer& gt);
	void UpdatePassCB(const GameTimer& gt);

	virtual void DrawBegin(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;
	void DrawRenderItems();
	virtual void DrawEnd(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

private:
	void BuildInputLayout();
	void BuildGeometry();
	void BuildBoxGeometry();
	void BuildGridGeometry();
	void BuildSphereGeometry();
	void BildCylinderGeometry();
	void BuildSkullGeometry();
	void BuildMaterials();
	void BuildRenderItem();
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

	// ���� ���̴��� �ȼ� ���̴�
	ComPtr<ID3DBlob> mVSByteCode = nullptr;
	ComPtr<ID3DBlob> mPSByteCode = nullptr;

	// ���� ������Ʈ ��� ����
	ComPtr<ID3D12Resource> mObjectCB = nullptr;
	BYTE* mObjectMappedData = nullptr;
	UINT mObjectByteSize = 0;

	// ���� ���� ��� ����
	ComPtr<ID3D12Resource> mMaterialCB = nullptr;
	BYTE* mMaterialMappedData = nullptr;
	UINT mMaterialByteSize = 0;

	// ���� ��� ����
	ComPtr<ID3D12Resource> mPassCB = nullptr;
	BYTE* mPassMappedData = nullptr;
	UINT mPassByteSize = 0;

	// ���� ���� ��
	std::unordered_map<std::string, std::unique_ptr<GeometryInfo>> mGeometries;

	// ���� ��
	std::unordered_map<std::string, std::unique_ptr<MaterialInfo>> mMaterials;

	// �������� ������Ʈ ����Ʈ
	std::vector<std::unique_ptr<RenderItem>> mRenderItems;

	// ���� / �þ�(ī�޶�) / ���� ���
	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	// ���� ��ǥ ���� ��
	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 5.0f;

	// ���콺 ��ǥ
	POINT mLastMousePos = { 0, 0 };
};