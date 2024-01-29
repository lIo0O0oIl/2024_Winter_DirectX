#pragma once

#include "D3dApp.h"
#include <DirectXColors.h>
#include "../Common/Camera.h"
#include "../Common//MathHelper.h"
#include "../Common//GeometryGenerator.h"
#include "../Common/DDSTextureLoader.h"

using namespace DirectX;

#define MAX_LIGHTS 16

// ������ ���̾�
enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	Skybox,
	Count
};

// ���� ����
struct Vertex 
{
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
	XMFLOAT2 Uv;
	XMFLOAT3 Tangent;
};

// ���� ������Ʈ ��� ���� (������Ʈ ������ŭ)
struct ObjectConstants
{
	XMFLOAT4X4 World = MathHelper::Identity4x4();
	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
};

// ���� ���� ��� ����
struct MatConstants
{
	XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	XMFLOAT3 FresnelIR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;
	UINT Texture_On = 0;
	UINT Normal_On = 0;
	XMFLOAT2 padding = { 0.0f, 0.0f };
};

// ������ ����
struct LightInfo
{
	UINT LightType = 0;												// 0 : directional, 1 : Point, 2 : Spot
	XMFLOAT3 padding = { 0.0f, 0.0f, 0.0f };					// ������ ������, ����ü�� ũ�⸦ �����ٷ���
	XMFLOAT3 Strengh = { 0.5f, 0.5f, 0.5f };					// ���� ����
	float FalloffStart = 1.0f;		// �����ϴ� ����?		// point / spot
	XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };				// direction / spot
	float FalloffEnd = 10.0f;		// ���谡 ������ ����	// point / spot
	XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };					// point / spot
	float SpotPower = 64.0f;											// spot
};		// 16(4x4)�� ����� ���;���.

// ���� ��� ���� (�ϳ��� ������.)
struct PassConstants
{
	XMFLOAT4X4 View = MathHelper::Identity4x4();
	XMFLOAT4X4 InvView = MathHelper::Identity4x4();		// ī�޶�, �ι��� ī�޶�
	XMFLOAT4X4 Proj = MathHelper::Identity4x4();
	XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
	XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();		// ����

	XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };		// ȯ�汤
	XMFLOAT3 EyePosW{ 0.0f, 0.0f, 0.0f };		// ī�޶� ��ġ
	UINT LightCount;
	LightInfo Lights[MAX_LIGHTS];

	XMFLOAT4 FogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	float FogStart = 5.0f;
	float FogRange = 150.0f;
	XMFLOAT2 padding;
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

// �ؽ��� ����
struct TextureInfo
{
	std::string Name;
	std::wstring Filename;

	int DiffuseSrvHeapIndex = -1;

	ComPtr<ID3D12Resource> Resource = nullptr;
	ComPtr<ID3D12Resource> UploadHeap = nullptr;		// temp ��?
};

// ���� ����
struct MaterialInfo
{
	std::string Name;

	int MatCBIndex = -1;
	int DiffuseSrvHeapIndex = -1;
	int NormalSrvHeapIndex = -1;

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
	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
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
	void DrawRenderItems(const std::vector<RenderItem*>& ritems);
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

	void BuildTextures();
	void BuildMaterials();
	void BuildRenderItem();
	void BuildShader();
	void BuildConstantBuffer();
	void BuildDescriptorHeaps();
	void BuildRootSignature();		// ���� ������ ���������� �� �� �ñ״��� ������ֱ� �Ŀ� ���������������� ���������.
	void BuildPSO();		// ���������ν�����Ʈ ��ü �����, ������ ���ٷ���

private:
	// �Է� ��ġ
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	// ��Ʈ �ñ״�ó
	ComPtr<ID3D12RootSignature>					mRootSignature = nullptr;

	// ���̴� ��
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;

	// ������ ���������� ���� ��
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

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

	// ��ī�̹ڽ� �ؽ��� ������ �� �ε���
	UINT mSkyTexHeapIndex = -1;

	// ������ ��
	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	// ���� ���� ��
	std::unordered_map<std::string, std::unique_ptr<GeometryInfo>> mGeometries;

	// ���� ��
	std::unordered_map<std::string, std::unique_ptr<MaterialInfo>> mMaterials;

	// �ؽ��� ��
	std::unordered_map<std::string, std::unique_ptr<TextureInfo>> mTextures;

	// �������� ������Ʈ ����Ʈ
	std::vector<std::unique_ptr<RenderItem>> mRenderItems;

	// �������� ������Ʈ ������ : PSO
	std::vector<RenderItem*> mRenderItemLayer[(int)RenderLayer::Count];

	// ī�޶� Ŭ����
	Camera mCamera;

	float mCameraSpeed = 10.0f;

	// ���콺 ��ǥ
	POINT mLastMousePos = { 0, 0 };
};