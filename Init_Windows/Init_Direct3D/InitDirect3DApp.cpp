#include "InitDirect3DApp.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,         // 진입점.
    PSTR cmdLine, int showCmd)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        InitDirect3DApp theApp(hInstance);      // 생성해서
        if (!theApp.Initialize())       // 초기화
            return 0;

        return theApp.Run();        // 여기서 런~
    }
    catch (DxException& e)      // 오류나면 알려주려고
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

InitDirect3DApp::InitDirect3DApp(HINSTANCE hInstance)
    : D3DApp(hInstance)
{
}

InitDirect3DApp::~InitDirect3DApp()
{
}

bool InitDirect3DApp::Initialize()
{
    if (!D3DApp::Initialize())
        return false;

    // 초기화 명령들을 준비하기 위해 명령 목록 재설정
    ThrowIfFailed(mCommandList->Reset(mCommandListAlloc.Get(), nullptr));

    // 초기화 명령들
    BuildInputLayout();
    BuildGeometry();
    BuildShader();
    BuildConstantBuffer();
    BuildRootSignature();
    BuildPSO();

    // 초기화 명령들 
    ThrowIfFailed(mCommandList->Close());

    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // 초기화 완료까지 기다리기
    FlushCommandQueue();
    return true;
}

void InitDirect3DApp::OnResize()
{
    D3DApp::OnResize();

    // 창의 크키가 바뀌었을 때, 종횡비 갱신 -> 투영 행렬 다시
    XMMATRIX proj = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, proj);
}

void InitDirect3DApp::Update(const GameTimer& gt)
{
    // 구면 좌표를 직교 좌표로 변환
    float x = mRadius * sinf(mPhi) * cosf(mTheta);
    float z = mRadius * sinf(mPhi) * sinf(mTheta);
    float y = mRadius * cosf(mPhi);

    // 시야행렬
    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);      // 카메라 행렬
    XMStoreFloat4x4(&mView, view);

    XMMATRIX world = XMLoadFloat4x4(&mWorld);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);
    XMMATRIX worldViewProj = world * view * proj;

    // 상수 버퍼 갱신
    ObjectConstants objConstants;
    XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
    ::memcpy(&mObjectMappedData[0], &objConstants, sizeof(ObjectConstants));
}

void InitDirect3DApp::DrawBegin(const GameTimer& gt)        // 그리는 데 필요한 기본 셋팅들
{
    ThrowIfFailed(mCommandListAlloc->Reset());

    ThrowIfFailed(mCommandList->Reset(mCommandListAlloc.Get(), nullptr));
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
}

void InitDirect3DApp::Draw(const GameTimer& gt)     // GPU 에 보낼 명령 정리해서 쌓음
{
    // 그리기 연산
    mCommandList->SetPipelineState(mPSO.Get());

    // 루트 시그니처 바인딩
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    // 개별 오브젝트 렌더링
    // 상수 버퍼 바인딩
    D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = mObjectCB->GetGPUVirtualAddress();
    mCommandList->SetGraphicsRootConstantBufferView(0, objCBAddress);

    // 삼각형 정점 정보와 인덱스 정보 셋팅
    mCommandList->IASetVertexBuffers(0, 1, &VertexBufferView);
    mCommandList->IASetIndexBuffer(&IndexBufferView);
    mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 삼각형 그리기
    mCommandList->DrawIndexedInstanced(IndexCount, 1, 0, 0, 0);
}

void InitDirect3DApp::DrawEnd(const GameTimer& gt)
{
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    ThrowIfFailed(mCommandList->Close());

    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrentBackBuffer = (mCurrentBackBuffer + 1) % SwapChainBufferCount;

    FlushCommandQueue();
}

void InitDirect3DApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMovesPos.x = x;
    mLastMovesPos.y = y;

    SetCapture(mhMainWnd);
}

void InitDirect3DApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void InitDirect3DApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if ((btnState & MK_LBUTTON) != 0) 
    {
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMovesPos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMovesPos.y));

        mTheta += dx;
        mPhi += dy;

        mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
    }
    else if ((btnState & MK_RBUTTON) != 0)
    {
        float dx = 0.2f * static_cast<float>(x - mLastMovesPos.x);
        float dy = 0.2f * static_cast<float>(y - mLastMovesPos.y);

        mRadius += dx - dy;

        mRadius = MathHelper::Clamp(mRadius, 3.0f, 150.0f);
    }

    mLastMovesPos.x = x;
    mLastMovesPos.y = y;
}

void InitDirect3DApp::BuildInputLayout()        // 처음 시작할 때 초기화
{
    mInputLayout =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
}

void InitDirect3DApp::BuildGeometry()
{
    // 정점 정보
    std::array<Vertex, 8> vertices =
    {
        Vertex({XMFLOAT3(-0.5f, 0.5f, -0.5f), XMFLOAT4(Colors::White)}),            // 0       왼쪽 위
        Vertex({XMFLOAT3(0.5f, 0.5f, -0.5f), XMFLOAT4(Colors::Blue)}),          // 1        오른쪽 위
        Vertex({XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT4(Colors::Blue)}),       // 2       오른쪽 아래
        Vertex({XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT4(Colors::White)}),       // 3          왼쪽 아래  // 여기까지가 앞면

        Vertex({XMFLOAT3(-0.5f, 0.5f, 0.5f), XMFLOAT4(Colors::Yellow)}),            // 4       왼쪽 위
        Vertex({XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT4(Colors::White)}),          // 5        오른쪽 위
        Vertex({XMFLOAT3(0.5f, -0.5f, 0.5f), XMFLOAT4(Colors::White)}),       // 6       오른쪽 아래
        Vertex({XMFLOAT3(-0.5f, -0.5f, 0.5f), XMFLOAT4(Colors::Yellow)}),       // 7          왼쪽 아래  // 여기까지 뒷면
    };

    // 인덱스 정보
    std::array<std::uint16_t, 36> indices =
    {
        0, 1, 2, 0, 2, 3,       // 앞면 맞니...??
        0, 4, 1, 4, 5, 1,        // 윗면
        3, 2, 7, 7, 2, 6,    // 아래
        5, 4, 7, 5, 7, 6,        // 뒤
        1, 5, 6, 1, 6, 2,
        4, 0, 3, 4, 3, 7
    };

    // 정점 버퍼 만들기
    VertexCount = (UINT)vertices.size();
    const UINT vbByteSize = VertexCount * sizeof(Vertex);

    D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(vbByteSize);

    md3dDevice->CreateCommittedResource(
        &heapProperty,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&VertexBuffer));

    void* vertexDataBuffer = nullptr;
    CD3DX12_RANGE vertexRange(0, 0);
    VertexBuffer->Map(0, &vertexRange, &vertexDataBuffer);
    ::memcpy(vertexDataBuffer, &vertices, vbByteSize);      // 업로드 버퍼일 때 카피 하는 것.
    VertexBuffer->Unmap(0, nullptr);

    VertexBufferView.BufferLocation = VertexBuffer->GetGPUVirtualAddress();
    VertexBufferView.StrideInBytes = sizeof(Vertex);
    VertexBufferView.SizeInBytes = vbByteSize;

    // 인덱스 버퍼 만들기
    IndexCount = (UINT)indices.size();
    const UINT ibByteSize = IndexCount * sizeof(std::uint16_t);

    heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    desc = CD3DX12_RESOURCE_DESC::Buffer(ibByteSize);

    md3dDevice->CreateCommittedResource(
        &heapProperty,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&IndexBuffer));

    void* indexDataBuffer = nullptr;
    CD3DX12_RANGE indexRange(0, 0);
    IndexBuffer->Map(0, &indexRange, &indexDataBuffer);
    ::memcpy(indexDataBuffer, &indices, ibByteSize);
    IndexBuffer->Unmap(0, nullptr);

    IndexBufferView.BufferLocation = IndexBuffer->GetGPUVirtualAddress();
    IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
    IndexBufferView.SizeInBytes = ibByteSize;
}

void InitDirect3DApp::BuildShader()
{
    mVSByteCode = d3dUtil::CompileShader(L"Color.hlsl", nullptr, "VS", "vs_5_0");
    mPSByteCode = d3dUtil::CompileShader(L"Color.hlsl", nullptr, "PS", "ps_5_0");
}

void InitDirect3DApp::BuildConstantBuffer()
{
    UINT size = sizeof(ObjectConstants);
    mObjectByteSize = (size + 255) & ~255;      // 나머지 연산 느낌으로.. 256의 배수단위로 짤려서
    D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);       // 항상 열어두는. 쉽게 쓸려고
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(mObjectByteSize);

    md3dDevice->CreateCommittedResource(
        &heapProperty,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&mObjectCB));
    
    mObjectCB->Map(0, nullptr, reinterpret_cast<void**>(&mObjectMappedData));
}

void InitDirect3DApp::BuildRootSignature()
{
    CD3DX12_ROOT_PARAMETER param[1];
    param[0].InitAsConstantBufferView(0);       // 0번 루트 시그니쳐에 b0 : CBV(상수버퍼뷰)     레지스터 연결, 후 바인딩

    D3D12_ROOT_SIGNATURE_DESC sigDesc = CD3DX12_ROOT_SIGNATURE_DESC(_countof(param), param);       // 매핑
    sigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> blobSignature;     // blob 는 파일을 만들어서 
    ComPtr<ID3DBlob> blobError;

    ::D3D12SerializeRootSignature(&sigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blobSignature, &blobError);      // 시그니쳐 파일 생성하자        // :: -> 내장함수 호출용임. 없어도 똑같음

    md3dDevice->CreateRootSignature(0, blobSignature->GetBufferPointer(), blobSignature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature));       // 루트 시그니쳐 생성.

}

void InitDirect3DApp::BuildPSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

    psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS =
    {
        reinterpret_cast<BYTE*>(mVSByteCode->GetBufferPointer()),
        mVSByteCode->GetBufferSize()
    };
    psoDesc.PS =
    {
        reinterpret_cast<BYTE*>(mPSByteCode->GetBufferPointer()),
        mPSByteCode->GetBufferSize()
    };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;     // 삼각형 사용
    psoDesc.NumRenderTargets = 1;       // 깊이값하면 2개가 됨.
    psoDesc.RTVFormats[0] = mBackBufferFormat;
    psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    psoDesc.SampleDesc.Quality = m4xMsaaQuality ? (m4xMsaaQuality - 1) : 0;
    psoDesc.DSVFormat = mDepthStencilFormat;

    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}
