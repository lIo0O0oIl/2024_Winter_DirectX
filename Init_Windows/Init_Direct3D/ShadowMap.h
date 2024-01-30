#pragma once

#include "../Common//d3dUtil.h"

class ShadowMap
{
public:
	ShadowMap(ID3D12Device* device, UINT width, UINT height);
	ShadowMap(const ShadowMap& rhs) = delete;
	ShadowMap& operator=(const ShadowMap& rhs) = delete;
	~ShadowMap() = default;

public:
	UINT Width()const;
	UINT Height()const;
	ID3D12Resource* Resource();
	CD3DX12_GPU_DESCRIPTOR_HANDLE Srv()const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE Dsv()const;

	D3D12_VIEWPORT Viewport()const;
	D3D12_RECT ScissorRect()const;

	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv);

	void OnResize(UINT newWidth, UINT newHeight);

private:
	void BuildResource();
	void BuildDescriptors();

private:
	ID3D12Device* md3dDevice = nullptr;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R24G8_TYPELESS;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;		// 리소스 만들고
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;		// 렌더링 파이프라인에 바인딩
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuDsv;		// 댑스 스텐실 버퍼, 텍스쳐 만들려고

	Microsoft::WRL::ComPtr<ID3D12Resource> mShadowMap = nullptr;
};

