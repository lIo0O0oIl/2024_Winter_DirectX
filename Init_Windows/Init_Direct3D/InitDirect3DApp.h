#pragma once

#include "D3dApp.h"
#include <DirectXColors.h>
#include "../Common/Camera.h"
#include "../Common//MathHelper.h"
#include "../Common//GeometryGenerator.h"
#include "../Common/DDSTextureLoader.h"

using namespace DirectX;

#define MAX_LIGHTS 16

// 렌더링 레이어
enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	Skybox,
	Count
};

// 정점 정보
struct Vertex 
{
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
	XMFLOAT2 Uv;
	XMFLOAT3 Tangent;
};

// 개별 오브젝트 상수 버퍼 (오브젝트 갯수만큼)
struct ObjectConstants
{
	XMFLOAT4X4 World = MathHelper::Identity4x4();
	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
};

// 개별 재질 상수 버퍼
struct MatConstants
{
	XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	XMFLOAT3 FresnelIR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;
	UINT Texture_On = 0;
	UINT Normal_On = 0;
	XMFLOAT2 padding = { 0.0f, 0.0f };
};

// 라이팅 정보
struct LightInfo
{
	UINT LightType = 0;												// 0 : directional, 1 : Point, 2 : Spot
	XMFLOAT3 padding = { 0.0f, 0.0f, 0.0f };					// 버리는 데이터, 구조체의 크기를 맞춰줄려고
	XMFLOAT3 Strengh = { 0.5f, 0.5f, 0.5f };					// 빛의 세기
	float FalloffStart = 1.0f;		// 감쇠하는 지점?		// point / spot
	XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };				// direction / spot
	float FalloffEnd = 10.0f;		// 감쇠가 끝나는 지점	// point / spot
	XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };					// point / spot
	float SpotPower = 64.0f;											// spot
};		// 16(4x4)의 배수로 나와야함.

// 공용 상수 버퍼 (하나만 존재함.)
struct PassConstants
{
	XMFLOAT4X4 View = MathHelper::Identity4x4();
	XMFLOAT4X4 InvView = MathHelper::Identity4x4();		// 카메라, 인벌스 카메라
	XMFLOAT4X4 Proj = MathHelper::Identity4x4();
	XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
	XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();		// 투영

	XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };		// 환경광
	XMFLOAT3 EyePosW{ 0.0f, 0.0f, 0.0f };		// 카메라 위치
	UINT LightCount;
	LightInfo Lights[MAX_LIGHTS];

	XMFLOAT4 FogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	float FogStart = 5.0f;
	float FogRange = 150.0f;
	XMFLOAT2 padding;
};

// 기하도형 정보
struct GeometryInfo
{
	std::string Name;

	// 정점 버퍼 뷰
	D3D12_VERTEX_BUFFER_VIEW						VertexBufferView = {};
	ComPtr<ID3D12Resource>								VertexBuffer = nullptr;

	// 인덱스 버퍼 뷰
	D3D12_INDEX_BUFFER_VIEW							IndexBufferView = {};
	ComPtr<ID3D12Resource>								IndexBuffer = nullptr;

	// 정점 갯수
	int VertexCount = 0;

	// 인덱스 갯수
	int IndexCount = 0;
};

// 텍스쳐 정보
struct TextureInfo
{
	std::string Name;
	std::wstring Filename;

	int DiffuseSrvHeapIndex = -1;

	ComPtr<ID3D12Resource> Resource = nullptr;
	ComPtr<ID3D12Resource> UploadHeap = nullptr;		// temp 격?
};

// 재질 정보
struct MaterialInfo
{
	std::string Name;

	int MatCBIndex = -1;
	int DiffuseSrvHeapIndex = -1;
	int NormalSrvHeapIndex = -1;

	XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };		// 굴절률
	float Roughness = 0.25f;		// 거칠기
};

// 렌더링할 오브젝트 구조체
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
	void BuildRootSignature();		// 내가 렌러딩 파이프라인 할 때 시그니쳐 만들어주기 후에 렌더링파이프라인 묶어줘야함.
	void BuildPSO();		// 파이프라인스테이트 개체 만들기, 설정값 해줄려고

private:
	// 입력 배치
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	// 루트 시그니처
	ComPtr<ID3D12RootSignature>					mRootSignature = nullptr;

	// 쉐이더 맵
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;

	// 렌더링 파이프라인 상태 맵
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

	// 개별 오브젝트 상수 버퍼
	ComPtr<ID3D12Resource> mObjectCB = nullptr;
	BYTE* mObjectMappedData = nullptr;
	UINT mObjectByteSize = 0;

	// 개별 재질 상수 버퍼
	ComPtr<ID3D12Resource> mMaterialCB = nullptr;
	BYTE* mMaterialMappedData = nullptr;
	UINT mMaterialByteSize = 0;

	// 공용 상수 버퍼
	ComPtr<ID3D12Resource> mPassCB = nullptr;
	BYTE* mPassMappedData = nullptr;
	UINT mPassByteSize = 0;

	// 스카이박스 텍스쳐 서술자 힙 인덱스
	UINT mSkyTexHeapIndex = -1;

	// 서술자 힙
	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	// 기하 구조 맵
	std::unordered_map<std::string, std::unique_ptr<GeometryInfo>> mGeometries;

	// 재질 맵
	std::unordered_map<std::string, std::unique_ptr<MaterialInfo>> mMaterials;

	// 텍스쳐 맵
	std::unordered_map<std::string, std::unique_ptr<TextureInfo>> mTextures;

	// 렌더링할 오브젝트 리스트
	std::vector<std::unique_ptr<RenderItem>> mRenderItems;

	// 렌더링할 오브젝트 나누기 : PSO
	std::vector<RenderItem*> mRenderItemLayer[(int)RenderLayer::Count];

	// 카메라 클래스
	Camera mCamera;

	float mCameraSpeed = 10.0f;

	// 마우스 좌표
	POINT mLastMousePos = { 0, 0 };
};