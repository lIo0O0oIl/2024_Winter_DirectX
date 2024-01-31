#pragma once

#include "D3dHeader.h"

class InitDirect3DApp : public D3DApp
{
public:
	InitDirect3DApp(HINSTANCE hInstance);
	~InitDirect3DApp();

	virtual bool Initialize()override;

private:
	void CreateDsvDescriptorHeaps()override;        // �����ٽǹ���
	virtual void OnResize()override;

	virtual void Update(const GameTimer& gt)override;
	void UpdateLight(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void UpdateObjectCB(const GameTimer& gt);
	void UpdateMaterialCB(const GameTimer& gt);
	void UpdatePassCB(const GameTimer& gt);
	void UpdateShadowCB(const GameTimer& gt);
	void UpdateSkinnedCB(const GameTimer& gt);

	virtual void DrawBegin(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;
	void DrawSceneToShadowMap();
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
	void BuildQuadGeometry();
	void BuildSphereGeometry();
	void BildCylinderGeometry();
	void BuildSkullGeometry();
	void BuildSkinnedModel();

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

	// Skinned Model �Է� ��ġ
	std::vector<D3D12_INPUT_ELEMENT_DESC> mSkinnedInputLayout;

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

	// ��Ű�� �ִϸ��̼� ��� ����
	ComPtr<ID3D12Resource> mSkinnedCB = nullptr;
	BYTE* mSkinnedMappedData = nullptr;
	UINT mSkinnedByteSize = 0;

	// ��ī�̹ڽ� �ؽ��� ������ �� �ε���
	UINT mSkyTexHeapIndex = -1;

	// �׸��� �� ������ �� �ε���
	UINT mShadowMapHeapIndex = -1;

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

	// �׸��� ��
	std::unique_ptr<ShadowMap> mShadowMap;		// COmPtr �� �ٷ� ������. �Ϲ��� ������ �����.

	// Skinned Model ���� ����
	std::string mSkinnedModelFilename = "..\\Models\\soldier.m3d";
	SkinnedData mSkinnedInfo;		// �� ������. �ִϸ��̼� ����
	std::vector < M3DLoader::Subset> mSkinnedSubsets;		// �𵨸�
	std::vector<M3DLoader::M3dMaterial> mSkinnedMats;		// ���׸���

	std::unique_ptr<SkinnedModelInstance> mSkinnedModelInst;

	// ī�޶� Ŭ����
	Camera mCamera;
	float mCameraSpeed = 10.0f;

	// ��� ��, ���̰��� �����ִ� �۾�, �߽��� �������� �����ϵ�.
	BoundingSphere mSceneBound;

	// ����Ʈ ���
	float mLightRotationAngle = 0.0f;
	float mLightRotationSpeed = 0.1f;
	XMFLOAT3 mBaseLightDirection = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);
	XMFLOAT3 mRotatedLightDirection;

	float mLightNearZ = 0.0f;
	float mLightFarZ = 0.0f;
	XMFLOAT3 mLightPosW;
	XMFLOAT4X4 mLightView = MathHelper::Identity4x4();
	XMFLOAT4X4 mLightProj = MathHelper::Identity4x4();
	XMFLOAT4X4 mShadowTransform = MathHelper::Identity4x4();

	// ���콺 ��ǥ
	POINT mLastMousePos = { 0, 0 };
};