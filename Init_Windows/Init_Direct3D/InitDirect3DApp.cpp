#include "InitDirect3DApp.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,         // ������.
    PSTR cmdLine, int showCmd)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        InitDirect3DApp theApp(hInstance);      // �����ؼ�
        if (!theApp.Initialize())       // �ʱ�ȭ
            return 0;

        return theApp.Run();        // ���⼭ ��~
    }
    catch (DxException& e)      // �������� �˷��ַ���
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

    // �ʱ�ȭ ��ɵ��� �غ��ϱ� ���� ��� ��� �缳��
    ThrowIfFailed(mCommandList->Reset(mCommandListAlloc.Get(), nullptr));

    // ī�޶� ����
    mCamera.SetPosition(0.0f, 2.0f, -15.0f);

    // �ʱ�ȭ ��ɵ�
    BuildInputLayout();
    BuildGeometry();
    BuildTextures();
    BuildMaterials();
    BuildRenderItem();      // �������� �������� ��dp������  ���̴� ������ �߿���. BuildConstantBuffer �̰͵� 2�� �ϰ� �ؾ���.
    BuildShader();
    BuildConstantBuffer();
    BuildDescriptorHeaps();
    BuildRootSignature();
    BuildPSO();

    // �ʱ�ȭ ��ɵ� 
    ThrowIfFailed(mCommandList->Close());

    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // �ʱ�ȭ �Ϸ���� ��ٸ���
    FlushCommandQueue();
    return true;
}

void InitDirect3DApp::OnResize()
{
    D3DApp::OnResize();

    // â�� ũŰ�� �ٲ���� ��, ��Ⱦ�� ���� -> ���� ��� �ٽ�
    mCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}

void InitDirect3DApp::Update(const GameTimer& gt)
{
    UpdateCamera(gt);
    UpdateObjectCB(gt);
    UpdateMaterialCB(gt);
    UpdatePassCB(gt);
}

void InitDirect3DApp::UpdateCamera(const GameTimer& gt)
{
    const float dt = gt.DeltaTime();

    if (GetAsyncKeyState('W') & 0x8000)
        mCamera.Walk(mCameraSpeed * dt);

    if (GetAsyncKeyState('S') & 0x8000)
        mCamera.Walk(-mCameraSpeed * dt);

    if (GetAsyncKeyState('D') & 0x8000)
        mCamera.Strafe(mCameraSpeed * dt);

    if (GetAsyncKeyState('A') & 0x8000)
        mCamera.Strafe(-mCameraSpeed * dt);

    if (GetAsyncKeyState('E') & 0x8000)
        mCamera.UpAndDown(mCameraSpeed * dt);

    if (GetAsyncKeyState('Q') & 0x8000)
        mCamera.UpAndDown(-mCameraSpeed * dt);

    mCamera.UpdateViewMatrix();         // �ٲ� ���� �������� ���� ������־��.
}

void InitDirect3DApp::UpdateObjectCB(const GameTimer& gt)
{
    // ���� ������Ʈ ��� ���� ����
    for (auto& item : mRenderItems)
    {
        XMMATRIX world = XMLoadFloat4x4(&item->World);      // ������ �ֱ�. �ε�.
        XMMATRIX texTransform = XMLoadFloat4x4(&item->TexTransform);

        // ��� ���� ����
        ObjectConstants objConstants;
        XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));     // world���� �����ͼ� ��������. 
        XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

        UINT elementIndex = item->ObjCBIndex;
        UINT elementByteSIze = (sizeof(ObjectConstants) + 255) & ~255;
        memcpy(&mObjectMappedData[elementIndex * elementByteSIze], &objConstants, sizeof(ObjectConstants));       // �޸� ī��
    }
}

void InitDirect3DApp::UpdateMaterialCB(const GameTimer& gt)
{
    for (auto& item : mMaterials) {
        MaterialInfo* mat = item.second.get();

        MatConstants matConstants;
        matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
        matConstants.FresnelIR0 = mat->FresnelR0;
        matConstants.Roughness = mat->Roughness;
        matConstants.Texture_On = (mat->DiffuseSrvHeapIndex == -1) ? 0 : 1;
        matConstants.Normal_On = (mat->NormalSrvHeapIndex == -1) ? 0 : 1;

        UINT elementIndex = mat->MatCBIndex;
        UINT elementByteSize = (sizeof(MatConstants) + 255) & ~255;
        memcpy(&mMaterialMappedData[elementIndex * elementByteSize], &matConstants, sizeof(matConstants));
    }
}

void InitDirect3DApp::UpdatePassCB(const GameTimer& gt)
{
    // ���� ��� ���� ����
    PassConstants mainPass;
    XMMATRIX view = mCamera.GetView();
    XMMATRIX proj = mCamera.GetProj();

    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX viewProj = XMMatrixMultiply(view, proj);

    XMStoreFloat4x4(&mainPass.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&mainPass.InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&mainPass.Proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&mainPass.InvProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&mainPass.ViewProj, XMMatrixTranspose(viewProj));

    mainPass.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
    mainPass.EyePosW = mCamera.GetPosition3f();
    mainPass.LightCount = 11;

    mainPass.Lights[0].LightType = 0;       // �𷺼ų� ����Ʈ
    mainPass.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
    mainPass.Lights[0].Strengh = { 0.6f, 0.6f, 0.6f };

    for (int i = 0; i < 5; ++i)
    {
        mainPass.Lights[1 + i].LightType = 1;
        mainPass.Lights[1 + i].Strengh = { 0.6f, 0.6f, 0.6f };
        mainPass.Lights[1 + i].Position = XMFLOAT3(-5.0f, 3.5f, -10.0f + i * 5.0f);
        mainPass.Lights[1 + i].FalloffStart = 2;
        mainPass.Lights[1 + i].FalloffEnd = 5;
    }

    for (int i = 0; i < 5; ++i)
    {
        mainPass.Lights[6 + i].LightType = 1;
        mainPass.Lights[6 + i].Strengh = { 0.6f, 0.6f, 0.6f };
        mainPass.Lights[6 + i].Position = XMFLOAT3(+5.0f, 3.5f, -10.0f + i * 5.0f);
        mainPass.Lights[6 + i].FalloffStart = 2;
        mainPass.Lights[6 + i].FalloffEnd = 5;
    }

    memcpy(&mPassMappedData[0], &mainPass, sizeof(PassConstants));      // PassConstants�� ũ�� ��ŭ ��������.
}

void InitDirect3DApp::DrawBegin(const GameTimer& gt)        // �׸��� �� �ʿ��� �⺻ ���õ�
{
    ThrowIfFailed(mCommandListAlloc->Reset());

    ThrowIfFailed(mCommandList->Reset(mCommandListAlloc.Get(), nullptr));
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::Gray, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
}

void InitDirect3DApp::Draw(const GameTimer& gt)     // GPU �� ���� ��� �����ؼ� ����
{
    // ������ ���������� ����
    mCommandList->SetPipelineState(mPSOs["opaque"].Get());

    // ��Ʈ �ñ״�ó ���ε�
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    // ������ ������ ���������� ���ε�
    ID3D12DescriptorHeap* descriptorHeap[] = { mSrvDescriptorHeap.Get() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeap), descriptorHeap);     // ������ ������ ���������ο� �� �����ֱ�

    // ���� ��� ���� ���ε�
    D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = mPassCB->GetGPUVirtualAddress();
    mCommandList->SetGraphicsRootConstantBufferView(2, passCBAddress);

    // ��ī�̹ڽ� �ؽ��� ���� ���ε�
    CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    skyTexDescriptor.Offset(mSkyTexHeapIndex, mCbvSrvUavDescriptorSize);    // �����ȰŸ�ŭ ������
    mCommandList->SetGraphicsRootDescriptorTable(3, skyTexDescriptor);

    DrawRenderItems(mRenderItemLayer[(int)RenderLayer::Opaque]);

    mCommandList->SetPipelineState(mPSOs["alphaTested"].Get());
    DrawRenderItems(mRenderItemLayer[(int)RenderLayer::AlphaTested]);

    mCommandList->SetPipelineState(mPSOs["transparent"].Get());
    DrawRenderItems(mRenderItemLayer[(int)RenderLayer::Transparent]);

    mCommandList->SetPipelineState(mPSOs["skybox"].Get());
    DrawRenderItems(mRenderItemLayer[(int)RenderLayer::Skybox]);
}

void InitDirect3DApp::DrawRenderItems(const std::vector<RenderItem*>& ritems)
{
    UINT objCBByteSize = (sizeof(ObjectConstants) + 255) & ~255;
    UINT matCBByteSize = (sizeof(MatConstants) + 255) & ~255;

    for (size_t i = 0; i < ritems.size(); ++i)
    {
        auto item = ritems[i];

        // ���� ������Ʈ ��� ���� �� ����
        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = mObjectCB->GetGPUVirtualAddress();
        objCBAddress += item->ObjCBIndex * objCBByteSize;

        mCommandList->SetGraphicsRootConstantBufferView(0, objCBAddress);

        // ���� ���� ��� ���� �� ����
        D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = mMaterialCB->GetGPUVirtualAddress();
        matCBAddress += item->Mat->MatCBIndex * matCBByteSize;

        mCommandList->SetGraphicsRootConstantBufferView(1, matCBAddress);

        // �ؽ��� ���� ������ �� ����
        CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        tex.Offset(item->Mat->DiffuseSrvHeapIndex, mCbvSrvUavDescriptorSize);

        if (item->Mat->DiffuseSrvHeapIndex != -1) 
        {
            mCommandList->SetGraphicsRootDescriptorTable(4, tex);       // 4���� ��Ʈ�ñ״��� ��ȣ��.
        }

        // �븻 �ؽ��� ���� ������ �� ����
        CD3DX12_GPU_DESCRIPTOR_HANDLE normal(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        normal.Offset(item->Mat->NormalSrvHeapIndex, mCbvSrvUavDescriptorSize);

        if (item->Mat->NormalSrvHeapIndex != -1)
        {
            mCommandList->SetGraphicsRootDescriptorTable(5, normal);       // 4���� ��Ʈ�ñ״��� ��ȣ��.
        }

        mCommandList->IASetVertexBuffers(0, 1, &item->Geo->VertexBufferView);
        mCommandList->IASetIndexBuffer(&item->Geo->IndexBufferView);
        mCommandList->IASetPrimitiveTopology(item->PrimitiveType);

        // ������
        mCommandList->DrawIndexedInstanced(item->Geo->IndexCount, 1, 0, 0, 0);
    }
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
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void InitDirect3DApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void InitDirect3DApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if ((btnState & MK_RBUTTON) != 0) 
    {
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));
        
        mCamera.Pitch(dy);      // ī�޶� ����
        mCamera.RotateY(dx);        // ī�޶� �¿�
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

void InitDirect3DApp::BuildInputLayout()        // ó�� ������ �� �ʱ�ȭ
{
    mInputLayout =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},      // 12ĭ
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},       // 12ĭ
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},        // 8ĭ
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}        // 12ĭ
    };
}

void InitDirect3DApp::BuildGeometry()
{
    BuildBoxGeometry();
    BuildGridGeometry();
    BuildSphereGeometry();
    BildCylinderGeometry();
    BuildSkullGeometry();
}

void InitDirect3DApp::BuildBoxGeometry()
{
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData box = geoGen.CreateBox(1.5f, 0.5f, 1.5f, 3);

    std::vector<Vertex> vertices(box.Vertices.size());      // ����
    for (UINT i = 0; i < box.Vertices.size(); ++i)
    {
        vertices[i].Pos = box.Vertices[i].Position;
        vertices[i].Normal = box.Vertices[i].Normal;
        vertices[i].Uv = box.Vertices[i].TexC;
        vertices[i].Tangent = box.Vertices[i].TangentU;
    }

    std::vector<std::uint16_t> indices;     // �ε���
    indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));        // �ѹ��� �� �־���.

    auto geo = std::make_unique<GeometryInfo>();
    geo->Name = "Box";

    // ���� ���� �����
    geo->VertexCount = (UINT)vertices.size();
    const UINT vbByteSize = geo->VertexCount * sizeof(Vertex);

    D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);    // �� ������Ƽ
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(vbByteSize);

    md3dDevice->CreateCommittedResource(
        &heapProperty,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&geo->VertexBuffer));

    void* vertexDataBuffer = nullptr;
    CD3DX12_RANGE vertexRange(0, 0);
    geo->VertexBuffer->Map(0, &vertexRange, &vertexDataBuffer);
    memcpy(vertexDataBuffer, vertices.data(), vbByteSize);
    geo->VertexBuffer->Unmap(0, nullptr);

    geo->VertexBufferView.BufferLocation = geo->VertexBuffer->GetGPUVirtualAddress();
    geo->VertexBufferView.StrideInBytes = sizeof(Vertex);
    geo->VertexBufferView.SizeInBytes = vbByteSize;

    // �ε��� ���� �����
    geo->IndexCount = (UINT)indices.size();
    const UINT ibByteSize = geo->IndexCount * sizeof(std::uint16_t);

    heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    desc = CD3DX12_RESOURCE_DESC::Buffer(ibByteSize);

    md3dDevice->CreateCommittedResource(
        &heapProperty,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&geo->IndexBuffer));

    void* indexDataBuffer = nullptr;
    CD3DX12_RANGE indexRange(0, 0);
    geo->IndexBuffer->Map(0, &indexRange, &indexDataBuffer);
    memcpy(indexDataBuffer, indices.data(), ibByteSize);
    geo->IndexBuffer->Unmap(0, nullptr);

    geo->IndexBufferView.BufferLocation = geo->IndexBuffer->GetGPUVirtualAddress();
    geo->IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferView.SizeInBytes = ibByteSize;

    mGeometries[geo->Name] = std::move(geo);        // ������ ������ �ű��
}

void InitDirect3DApp::BuildGridGeometry()
{
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);

    std::vector<Vertex> vertices(grid.Vertices.size());      // ����
    for (UINT i = 0; i < grid.Vertices.size(); ++i)
    {
        vertices[i].Pos = grid.Vertices[i].Position;
        vertices[i].Normal = grid.Vertices[i].Normal;
        vertices[i].Uv = grid.Vertices[i].TexC;
        vertices[i].Tangent = grid.Vertices[i].TangentU;
    }

    std::vector<std::uint16_t> indices;     // �ε���
    indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));        // �ѹ��� �� �־���.

    auto geo = std::make_unique<GeometryInfo>();
    geo->Name = "Grid";

    // ���� ���� �����
    geo->VertexCount = (UINT)vertices.size();
    const UINT vbByteSize = geo->VertexCount * sizeof(Vertex);

    D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);    // �� ������Ƽ
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(vbByteSize);

    md3dDevice->CreateCommittedResource(
        &heapProperty,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&geo->VertexBuffer));

    void* vertexDataBuffer = nullptr;
    CD3DX12_RANGE vertexRange(0, 0);
    geo->VertexBuffer->Map(0, &vertexRange, &vertexDataBuffer);
    memcpy(vertexDataBuffer, vertices.data(), vbByteSize);
    geo->VertexBuffer->Unmap(0, nullptr);

    geo->VertexBufferView.BufferLocation = geo->VertexBuffer->GetGPUVirtualAddress();
    geo->VertexBufferView.StrideInBytes = sizeof(Vertex);
    geo->VertexBufferView.SizeInBytes = vbByteSize;

    // �ε��� ���� �����
    geo->IndexCount = (UINT)indices.size();
    const UINT ibByteSize = geo->IndexCount * sizeof(std::uint16_t);

    heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    desc = CD3DX12_RESOURCE_DESC::Buffer(ibByteSize);

    md3dDevice->CreateCommittedResource(
        &heapProperty,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&geo->IndexBuffer));

    void* indexDataBuffer = nullptr;
    CD3DX12_RANGE indexRange(0, 0);
    geo->IndexBuffer->Map(0, &indexRange, &indexDataBuffer);
    memcpy(indexDataBuffer, indices.data(), ibByteSize);
    geo->IndexBuffer->Unmap(0, nullptr);

    geo->IndexBufferView.BufferLocation = geo->IndexBuffer->GetGPUVirtualAddress();
    geo->IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferView.SizeInBytes = ibByteSize;

    mGeometries[geo->Name] = std::move(geo);        // ������ ������ �ű��
}

void InitDirect3DApp::BuildSphereGeometry()
{
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);

    std::vector<Vertex> vertices(sphere.Vertices.size());      // ����
    for (UINT i = 0; i < sphere.Vertices.size(); ++i)
    {
        vertices[i].Pos = sphere.Vertices[i].Position;
        vertices[i].Normal = sphere.Vertices[i].Normal;
        vertices[i].Uv = sphere.Vertices[i].TexC;
        vertices[i].Tangent = sphere.Vertices[i].TangentU;
    }

    std::vector<std::uint16_t> indices;     // �ε���
    indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));        // �ѹ��� �� �־���.

    auto geo = std::make_unique<GeometryInfo>();
    geo->Name = "Sphere";

    // ���� ���� �����
    geo->VertexCount = (UINT)vertices.size();
    const UINT vbByteSize = geo->VertexCount * sizeof(Vertex);

    D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);    // �� ������Ƽ
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(vbByteSize);

    md3dDevice->CreateCommittedResource(
        &heapProperty,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&geo->VertexBuffer));

    void* vertexDataBuffer = nullptr;
    CD3DX12_RANGE vertexRange(0, 0);
    geo->VertexBuffer->Map(0, &vertexRange, &vertexDataBuffer);
    memcpy(vertexDataBuffer, vertices.data(), vbByteSize);
    geo->VertexBuffer->Unmap(0, nullptr);

    geo->VertexBufferView.BufferLocation = geo->VertexBuffer->GetGPUVirtualAddress();
    geo->VertexBufferView.StrideInBytes = sizeof(Vertex);
    geo->VertexBufferView.SizeInBytes = vbByteSize;

    // �ε��� ���� �����
    geo->IndexCount = (UINT)indices.size();
    const UINT ibByteSize = geo->IndexCount * sizeof(std::uint16_t);

    heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    desc = CD3DX12_RESOURCE_DESC::Buffer(ibByteSize);

    md3dDevice->CreateCommittedResource(
        &heapProperty,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&geo->IndexBuffer));

    void* indexDataBuffer = nullptr;
    CD3DX12_RANGE indexRange(0, 0);
    geo->IndexBuffer->Map(0, &indexRange, &indexDataBuffer);
    memcpy(indexDataBuffer, indices.data(), ibByteSize);
    geo->IndexBuffer->Unmap(0, nullptr);

    geo->IndexBufferView.BufferLocation = geo->IndexBuffer->GetGPUVirtualAddress();
    geo->IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferView.SizeInBytes = ibByteSize;

    mGeometries[geo->Name] = std::move(geo);        // ������ ������ �ű��
}

void InitDirect3DApp::BildCylinderGeometry()
{
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.25f, 3.0f, 20, 20);

    std::vector<Vertex> vertices(cylinder.Vertices.size());      // ����
    for (UINT i = 0; i < cylinder.Vertices.size(); ++i)
    {
        vertices[i].Pos = cylinder.Vertices[i].Position;
        vertices[i].Normal = cylinder.Vertices[i].Normal;
        vertices[i].Uv = cylinder.Vertices[i].TexC;
        vertices[i].Tangent = cylinder.Vertices[i].TangentU;
    }

    std::vector<std::uint16_t> indices;     // �ε���
    indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));        // �ѹ��� �� �־���.

    auto geo = std::make_unique<GeometryInfo>();
    geo->Name = "Cylinder";

    // ���� ���� �����
    geo->VertexCount = (UINT)vertices.size();
    const UINT vbByteSize = geo->VertexCount * sizeof(Vertex);

    D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);    // �� ������Ƽ
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(vbByteSize);

    md3dDevice->CreateCommittedResource(
        &heapProperty,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&geo->VertexBuffer));

    void* vertexDataBuffer = nullptr;
    CD3DX12_RANGE vertexRange(0, 0);
    geo->VertexBuffer->Map(0, &vertexRange, &vertexDataBuffer);
    memcpy(vertexDataBuffer, vertices.data(), vbByteSize);
    geo->VertexBuffer->Unmap(0, nullptr);

    geo->VertexBufferView.BufferLocation = geo->VertexBuffer->GetGPUVirtualAddress();
    geo->VertexBufferView.StrideInBytes = sizeof(Vertex);
    geo->VertexBufferView.SizeInBytes = vbByteSize;

    // �ε��� ���� �����
    geo->IndexCount = (UINT)indices.size();
    const UINT ibByteSize = geo->IndexCount * sizeof(std::uint16_t);

    heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    desc = CD3DX12_RESOURCE_DESC::Buffer(ibByteSize);

    md3dDevice->CreateCommittedResource(
        &heapProperty,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&geo->IndexBuffer));

    void* indexDataBuffer = nullptr;
    CD3DX12_RANGE indexRange(0, 0);
    geo->IndexBuffer->Map(0, &indexRange, &indexDataBuffer);
    memcpy(indexDataBuffer, indices.data(), ibByteSize);
    geo->IndexBuffer->Unmap(0, nullptr);

    geo->IndexBufferView.BufferLocation = geo->IndexBuffer->GetGPUVirtualAddress();
    geo->IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferView.SizeInBytes = ibByteSize;

    mGeometries[geo->Name] = std::move(geo);        // ������ ������ �ű��
}

void InitDirect3DApp::BuildSkullGeometry()
{
    std::ifstream fin("../Models/skull.txt");            // ��ǲ���Ͻ�Ʈ��
    if (!fin) {
        MessageBox(0, L"../Models/skull.txt not found", 0, 0);
        return;
    }

    UINT vCount = 0;        // ���ؽ� ī��Ʈ
    UINT tCount = 0;        // Ʈ���̾ޱ� ī���
    std::string ignore;

    fin >> ignore >> vCount;        // ���� �̴°�
    fin >> ignore >> tCount;
    fin >> ignore >> ignore >> ignore >> ignore;

    std::vector<Vertex> vertices(vCount);
    for (UINT i = 0; i < vCount; ++i)
    {
        fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
        fin >> vertices[i].Normal.x >> vertices[i].Normal   .y >> vertices[i].Normal.z;
    }

    fin >> ignore;
    fin >> ignore;
    fin >> ignore;

    std::vector<std::int32_t> indices(3 * tCount);
    for (UINT i = 0; i < tCount; ++i)
    {
        fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
    }

    fin.close();

    auto geo = std::make_unique<GeometryInfo>();
    geo->Name = "Skull";

    // ���� ���� �����
    geo->VertexCount = (UINT)vertices.size();
    const UINT vbByteSize = geo->VertexCount * sizeof(Vertex);

    D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);    // �� ������Ƽ
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(vbByteSize);

    md3dDevice->CreateCommittedResource(
        &heapProperty,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&geo->VertexBuffer));

    void* vertexDataBuffer = nullptr;
    CD3DX12_RANGE vertexRange(0, 0);
    geo->VertexBuffer->Map(0, &vertexRange, &vertexDataBuffer);
    memcpy(vertexDataBuffer, vertices.data(), vbByteSize);
    geo->VertexBuffer->Unmap(0, nullptr);

    geo->VertexBufferView.BufferLocation = geo->VertexBuffer->GetGPUVirtualAddress();
    geo->VertexBufferView.StrideInBytes = sizeof(Vertex);
    geo->VertexBufferView.SizeInBytes = vbByteSize;

    // �ε��� ���� �����
    geo->IndexCount = (UINT)indices.size();
    const UINT ibByteSize = geo->IndexCount * sizeof(std::int32_t);     // ũ�� �����ֱ�

    heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    desc = CD3DX12_RESOURCE_DESC::Buffer(ibByteSize);

    md3dDevice->CreateCommittedResource(
        &heapProperty,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&geo->IndexBuffer));

    void* indexDataBuffer = nullptr;
    CD3DX12_RANGE indexRange(0, 0);
    geo->IndexBuffer->Map(0, &indexRange, &indexDataBuffer);
    memcpy(indexDataBuffer, indices.data(), ibByteSize);
    geo->IndexBuffer->Unmap(0, nullptr);

    geo->IndexBufferView.BufferLocation = geo->IndexBuffer->GetGPUVirtualAddress();
    geo->IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    geo->IndexBufferView.SizeInBytes = ibByteSize;

    mGeometries[geo->Name] = std::move(geo);        // ������ ������ �ű��
}

void InitDirect3DApp::BuildTextures()
{
    UINT indexCount = 0;

    auto bricksTex = std::make_unique<TextureInfo>();
    bricksTex->Name = "bricks";
    bricksTex->Filename = L"../Textures/bricks.dds";
    bricksTex->DiffuseSrvHeapIndex = indexCount++;
    ThrowIfFailed(CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(),
        bricksTex->Filename.c_str(),
        bricksTex->Resource, bricksTex->UploadHeap));
    mTextures[bricksTex->Name] = std::move(bricksTex);

    auto bricksNoramlTex = std::make_unique<TextureInfo>();
    bricksNoramlTex->Name = "bricksNormal";
    bricksNoramlTex->Filename = L"../Textures/bricks_nmap.dds";
    bricksNoramlTex->DiffuseSrvHeapIndex = indexCount++;
    ThrowIfFailed(CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(),
        bricksNoramlTex->Filename.c_str(),
        bricksNoramlTex->Resource, bricksNoramlTex->UploadHeap));
    mTextures[bricksNoramlTex->Name] = std::move(bricksNoramlTex);

    auto stoneTex = std::make_unique<TextureInfo>();
    stoneTex->Name = "stone";
    stoneTex->Filename = L"../Textures/stone.dds";
    stoneTex->DiffuseSrvHeapIndex = indexCount++;
    ThrowIfFailed(CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(),
        stoneTex->Filename.c_str(),
        stoneTex->Resource, stoneTex->UploadHeap));
    mTextures[stoneTex->Name] = std::move(stoneTex);

    auto tileTex = std::make_unique<TextureInfo>();
    tileTex->Name = "tile";
    tileTex->Filename = L"../Textures/tile.dds";
    tileTex->DiffuseSrvHeapIndex = indexCount++;
    ThrowIfFailed(CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(),
        tileTex->Filename.c_str(),
        tileTex->Resource, tileTex->UploadHeap));
    mTextures[tileTex->Name] = std::move(tileTex);

    auto tileNormalTex = std::make_unique<TextureInfo>();
    tileNormalTex->Name = "tileNormal";
    tileNormalTex->Filename = L"../Textures/tile_nmap.dds";
    tileNormalTex->DiffuseSrvHeapIndex = indexCount++;
    ThrowIfFailed(CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(),
        tileNormalTex->Filename.c_str(),
        tileNormalTex->Resource, tileNormalTex->UploadHeap));
    mTextures[tileNormalTex->Name] = std::move(tileNormalTex);

    auto fenceTex = std::make_unique<TextureInfo>();
    fenceTex->Name = "wirefence";
    fenceTex->Filename = L"../Textures/WireFence.dds";
    fenceTex->DiffuseSrvHeapIndex = indexCount++;
    ThrowIfFailed(CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(),
        fenceTex->Filename.c_str(),
        fenceTex->Resource, fenceTex->UploadHeap));
    mTextures[fenceTex->Name] = std::move(fenceTex);

    auto skyboxTex = std::make_unique<TextureInfo>();
    skyboxTex->Name = "skybox";
    skyboxTex->Filename = L"../Textures/snowcube1024.dds";
    skyboxTex->DiffuseSrvHeapIndex = indexCount++;
    ThrowIfFailed(CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(),
        skyboxTex->Filename.c_str(),
        skyboxTex->Resource, skyboxTex->UploadHeap));
    mTextures[skyboxTex->Name] = std::move(skyboxTex);
}

void InitDirect3DApp::BuildMaterials()
{
    UINT indexCount = 0;

    auto green = std::make_unique<MaterialInfo>();
    green->Name = "Green";
    green->MatCBIndex = indexCount++;
    green->DiffuseAlbedo = XMFLOAT4(Colors::ForestGreen);
    green->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    green->Roughness = 0.1f;
    mMaterials[green->Name] = std::move(green);

    auto blue = std::make_unique<MaterialInfo>();
    blue->Name = "Blue";
    blue->MatCBIndex = indexCount++;
    blue->DiffuseAlbedo = XMFLOAT4(Colors::LightSteelBlue);
    blue->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
    blue->Roughness = 0.3f;
    mMaterials[blue->Name] = std::move(blue);

    auto gray = std::make_unique<MaterialInfo>();
    gray->Name = "Gray";
    gray->MatCBIndex = indexCount++;
    gray->DiffuseAlbedo = XMFLOAT4(Colors::LightGray);
    gray->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    gray->Roughness = 0.2f;
    mMaterials[gray->Name] = std::move(gray);

    auto skull = std::make_unique<MaterialInfo>();
    skull->Name = "Skull";
    skull->MatCBIndex = indexCount++;
    skull->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
    skull->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
    skull->Roughness = 0.3f;
    mMaterials[skull->Name] = std::move(skull);

    auto bricks = std::make_unique<MaterialInfo>();
    bricks->Name = "Bricks";
    bricks->MatCBIndex = indexCount++;
    bricks->DiffuseSrvHeapIndex = mTextures["bricks"]->DiffuseSrvHeapIndex;
    bricks->NormalSrvHeapIndex = mTextures["bricksNormal"]->DiffuseSrvHeapIndex;
    bricks->DiffuseAlbedo = XMFLOAT4(Colors::White);
    bricks->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    bricks->Roughness = 0.1f;
    mMaterials[bricks->Name] = std::move(bricks);

    auto stone = std::make_unique<MaterialInfo>();
    stone->Name = "Stone";
    stone->MatCBIndex = indexCount++;
    stone->DiffuseSrvHeapIndex = mTextures["stone"]->DiffuseSrvHeapIndex;
    stone->DiffuseAlbedo = XMFLOAT4(Colors::White);
    stone->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
    stone->Roughness = 0.3f;
    mMaterials[stone->Name] = std::move(stone);

    auto tile = std::make_unique<MaterialInfo>();
    tile->Name = "Tile";
    tile->MatCBIndex = indexCount++;
    tile->DiffuseSrvHeapIndex = mTextures["tile"]->DiffuseSrvHeapIndex;
    tile->NormalSrvHeapIndex = mTextures["tileNormal"]->DiffuseSrvHeapIndex;
    tile->DiffuseAlbedo = XMFLOAT4(Colors::White);
    tile->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    tile->Roughness = 0.2f;
    mMaterials[tile->Name] = std::move(tile);

    auto wirefence = std::make_unique<MaterialInfo>();
    wirefence->Name = "Wirefence";
    wirefence->MatCBIndex = indexCount++;
    wirefence->DiffuseSrvHeapIndex = mTextures["wirefence"]->DiffuseSrvHeapIndex;
    wirefence->DiffuseAlbedo = XMFLOAT4(Colors::White);
    wirefence->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
    wirefence->Roughness = 0.25f;
    mMaterials[wirefence->Name] = std::move(wirefence);

    auto skybox = std::make_unique<MaterialInfo>();
    skybox->Name = "Skybox";
    skybox->MatCBIndex = indexCount++;
    skybox->DiffuseSrvHeapIndex = mTextures["skybox"]->DiffuseSrvHeapIndex;
    skybox->DiffuseAlbedo = XMFLOAT4(Colors::White);
    skybox->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
    skybox->Roughness = 1.0f;
    mMaterials[skybox->Name] = std::move(skybox);

    auto mirror = std::make_unique<MaterialInfo>();
    mirror->Name = "Mirror";
    mirror->MatCBIndex = indexCount++;
    mirror->DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    mirror->FresnelR0 = XMFLOAT3(0.98f, 0.97f, 0.95f);
    mirror->Roughness = 0.1f;
    mMaterials[mirror->Name] = std::move(mirror);
}

void InitDirect3DApp::BuildRenderItem()
{
    UINT objCBIndex = 0;

    // ��ī�̹ڽ�
    auto skyItem = std::make_unique<RenderItem>();
    skyItem->ObjCBIndex = objCBIndex++;
    skyItem->World = MathHelper::Identity4x4();
    XMStoreFloat4x4(&skyItem->TexTransform, XMMatrixScaling(5000.0f, 5000.0f, 5000.0f));
    skyItem->Geo = mGeometries["Box"].get();
    skyItem->Mat = mMaterials["Skybox"].get();
    skyItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    skyItem->IndexCount = skyItem->Geo->IndexCount;
    mRenderItemLayer[(int)RenderLayer::Skybox].push_back(skyItem.get());        // �ø��ϸ� �ȉ�. ���ʿ��� ������ �ؿ�.
    mRenderItems.push_back(std::move(skyItem));

    // �ٴ�
    auto gridItem = std::make_unique<RenderItem>();
    gridItem->ObjCBIndex = objCBIndex++;
    gridItem->World = MathHelper::Identity4x4();
    XMStoreFloat4x4(&gridItem->TexTransform, XMMatrixScaling(8.0f, 8.0f, 8.0f));
    gridItem->Geo = mGeometries["Grid"].get();
    gridItem->Mat = mMaterials["Tile"].get();
    gridItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    gridItem->IndexCount = gridItem->Geo->IndexCount;
    mRenderItemLayer[(int)RenderLayer::Opaque].push_back(gridItem.get());
    mRenderItems.push_back(std::move(gridItem));

    // �ڽ�
    auto boxItem = std::make_unique<RenderItem>();
    boxItem->ObjCBIndex = objCBIndex++;
    XMStoreFloat4x4(&boxItem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.51f, 0.0f));      // ��ġ ���� ����. ���� �÷���, 0.01�� ��ħ ���� �ذ��Ϸ���
    XMStoreFloat4x4(&boxItem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
    boxItem->Geo = mGeometries["Box"].get();
    boxItem->Mat = mMaterials["Wirefence"].get();
    boxItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    boxItem->IndexCount = boxItem->Geo->IndexCount;
    mRenderItemLayer[(int)RenderLayer::AlphaTested].push_back(boxItem.get());
    mRenderItems.push_back(std::move(boxItem));

    // �ذ�
    auto skullItem = std::make_unique<RenderItem>();
    skullItem->ObjCBIndex = objCBIndex++;
    XMStoreFloat4x4(&skullItem->World, XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(0.0f, 1.0f, 0.0f));      // ��ġ ���� ����. ���� �÷���
    skullItem->Geo = mGeometries["Skull"].get();
    skullItem->Mat = mMaterials["Skull"].get();
    skullItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    skullItem->IndexCount = skullItem->Geo->IndexCount;
    mRenderItemLayer[(int)RenderLayer::Transparent].push_back(skullItem.get());
    mRenderItems.push_back(std::move(skullItem));

    for (int i = 0; i < 5; ++i) 
    {
        auto leftCylItem = std::make_unique<RenderItem>();
        auto rightCylItem = std::make_unique<RenderItem>();
        auto leftSphereItem = std::make_unique<RenderItem>();
        auto rightSphereItem = std::make_unique<RenderItem>();

        XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
        XMMATRIX rightCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);

        XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
        XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);

        // ���� �Ǹ���
        XMStoreFloat4x4(&leftCylItem->World, leftCylWorld);
        XMStoreFloat4x4(&leftCylItem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
        leftCylItem->ObjCBIndex = objCBIndex++;
        leftCylItem->Geo = mGeometries["Cylinder"].get();
        leftCylItem->Mat = mMaterials["Bricks"].get();
        leftCylItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftCylItem->IndexCount = leftCylItem->Geo->IndexCount;
        mRenderItemLayer[(int)RenderLayer::Opaque].push_back(leftCylItem.get());
        mRenderItems.push_back(std::move(leftCylItem));     // �Ű��ֱ�

        // ������ �Ǹ���
        XMStoreFloat4x4(&rightCylItem->World, rightCylWorld);
        XMStoreFloat4x4(&rightCylItem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
        rightCylItem->ObjCBIndex = objCBIndex++;
        rightCylItem->Geo = mGeometries["Cylinder"].get();
        rightCylItem->Mat = mMaterials["Bricks"].get();
        rightCylItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightCylItem->IndexCount = rightCylItem->Geo->IndexCount;
        mRenderItemLayer[(int)RenderLayer::Opaque].push_back(rightCylItem.get());
        mRenderItems.push_back(std::move(rightCylItem));     // �Ű��ֱ�

        // ���� ���Ǿ�
        XMStoreFloat4x4(&leftSphereItem->World, leftSphereWorld);
        XMStoreFloat4x4(&leftSphereItem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
        leftSphereItem->ObjCBIndex = objCBIndex++;
        leftSphereItem->Geo = mGeometries["Sphere"].get();
        leftSphereItem->Mat = mMaterials["Mirror"].get();
        leftSphereItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftSphereItem->IndexCount = leftSphereItem->Geo->IndexCount;
        mRenderItemLayer[(int)RenderLayer::Opaque].push_back(leftSphereItem.get());
        mRenderItems.push_back(std::move(leftSphereItem));     // �Ű��ֱ�

        // ������ ���Ǿ�
        XMStoreFloat4x4(&rightSphereItem->World, rightSphereWorld);
        XMStoreFloat4x4(&rightSphereItem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
        rightSphereItem->ObjCBIndex = objCBIndex++;
        rightSphereItem->Geo = mGeometries["Sphere"].get();
        rightSphereItem->Mat = mMaterials["Mirror"].get();
        rightSphereItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightSphereItem->IndexCount = rightSphereItem->Geo->IndexCount;
        mRenderItemLayer[(int)RenderLayer::Opaque].push_back(rightSphereItem.get());
        mRenderItems.push_back(std::move(rightSphereItem));     // �Ű��ֱ�
    }
}

void InitDirect3DApp::BuildShader()
{
    const D3D_SHADER_MACRO defines[] =
    {
        "FOG", "1",
        NULL, NULL
    };

    const D3D_SHADER_MACRO alphaTestDefines[] =
    {
        "FOG", "1",
        "ALPHA_TEST", "1",
        NULL, NULL
    };

    mShaders["standardVS"] = d3dUtil::CompileShader(L"Color.hlsl", nullptr, "VS", "vs_5_0");        // �ι�° ���ڴ� ��ũ��,
    mShaders["opaquePS"] = d3dUtil::CompileShader(L"Color.hlsl", defines, "PS", "ps_5_0");
    mShaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Color.hlsl", alphaTestDefines, "PS", "ps_5_0");

    mShaders["skyVS"] = d3dUtil::CompileShader(L"Skybox.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["skyPS"] = d3dUtil::CompileShader(L"Skybox.hlsl", nullptr, "PS", "ps_5_1");
}

void InitDirect3DApp::BuildConstantBuffer()
{
    // ���� ������Ʈ ��� ����
    UINT size = sizeof(ObjectConstants);
    mObjectByteSize = ((size + 255) & ~255) * mRenderItems.size();      // ������ ���� ��������.. 256�� ��������� ©����, �׸��� ������Ʈ ������ �����ֱ�

    D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);       // �׻� ����δ�. ���� ������
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(mObjectByteSize);

    md3dDevice->CreateCommittedResource(
        &heapProperty,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&mObjectCB));
    
    mObjectCB->Map(0, nullptr, reinterpret_cast<void**>(&mObjectMappedData));

    // ���� ���� ��� ����
    size = sizeof(MatConstants);
    mMaterialByteSize = ((size + 255) & ~255) * mRenderItems.size();      // ������ ���� ��������.. 256�� ��������� ©����, �׸��� ������Ʈ ������ �����ֱ�

    heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);       // �׻� ����δ�. ���� ������
    desc = CD3DX12_RESOURCE_DESC::Buffer(mMaterialByteSize);

    md3dDevice->CreateCommittedResource(
        &heapProperty,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&mMaterialCB));

    mMaterialCB->Map(0, nullptr, reinterpret_cast<void**>(&mMaterialMappedData));

    // ���� ��� ����
    size = sizeof(PassConstants);
    mPassByteSize = (size + 255) & ~255;            // 256�� �����. 257�̸� 512�� ��.
    heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    desc = CD3DX12_RESOURCE_DESC::Buffer(mPassByteSize);

    md3dDevice->CreateCommittedResource(
        &heapProperty,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&mPassCB));

    mPassCB->Map(0, nullptr, reinterpret_cast<void**>(&mPassMappedData));       // �����͸� �־��ֱ� mPassMappedData�� mPassCB��.
}

void InitDirect3DApp::BuildDescriptorHeaps()
{
    // SRV heap �����
    D3D12_DESCRIPTOR_HEAP_DESC srcHeapDesc = {};
    srcHeapDesc.NumDescriptors = mTextures.size();
    srcHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srcHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;      // �� ���� ������ ��.
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srcHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

    // SRV Heap ä���
    for (auto& item : mTextures)
    {
        TextureInfo* tex = item.second.get();
        CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
        hDescriptor.Offset(tex->DiffuseSrvHeapIndex, mCbvSrvUavDescriptorSize);

        auto texResource = tex->Resource;
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

        if (tex->Name == "skybox")
        {
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = texResource->GetDesc().Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            srvDesc.TextureCube.MostDetailedMip = 0;
            srvDesc.TextureCube.MipLevels = texResource->GetDesc().MipLevels;
            srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

            mSkyTexHeapIndex = tex->DiffuseSrvHeapIndex;
        }
        else 
        {
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = texResource->GetDesc().Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = texResource->GetDesc().MipLevels;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        }
        md3dDevice->CreateShaderResourceView(texResource.Get(), &srvDesc, hDescriptor);
    }
}

void InitDirect3DApp::BuildRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE skyboxTable[] =
    {
        CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0),        // t0
    };

    CD3DX12_DESCRIPTOR_RANGE texTable[] =
    {
        CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1),        // t1
    };

    CD3DX12_DESCRIPTOR_RANGE normalTable[] =
    {
        CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2),        // t2
    };

    CD3DX12_ROOT_PARAMETER param[6];
    param[0].InitAsConstantBufferView(0);       // 0�� ��Ʈ �ñ״��Ŀ� b0 : CBV(���� ������Ʈ ������ۺ�)     �������� ����, �� ���ε�
    param[1].InitAsConstantBufferView(1);       // 1�� -> b1 : ���� ���� CBV;
    param[2].InitAsConstantBufferView(2);       // 2�� -> b1 : ���� CBV
    param[3].InitAsDescriptorTable(_countof(skyboxTable), skyboxTable);     // 3�� -> t0 : ��ī�̹ڽ� �ؽ�ó
    param[4].InitAsDescriptorTable(_countof(texTable), texTable);       // 4�� -> t1 : ������Ʈ �ý���
    param[5].InitAsDescriptorTable(_countof(normalTable), normalTable);       // 5�� -> t1 : �븻 �ý���

    D3D12_STATIC_SAMPLER_DESC samplerDesc = CD3DX12_STATIC_SAMPLER_DESC(0);         // s0 : ���÷�
    D3D12_ROOT_SIGNATURE_DESC sigDesc = CD3DX12_ROOT_SIGNATURE_DESC(_countof(param), param, 1, &samplerDesc);       // ����
    sigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> blobSignature;     // blob �� ������ ���� 
    ComPtr<ID3DBlob> blobError;

    ::D3D12SerializeRootSignature(&sigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blobSignature, &blobError);      // �ñ״��� ���� ��������        // :: -> �����Լ� ȣ�����. ��� �Ȱ���

    md3dDevice->CreateRootSignature(0, blobSignature->GetBufferPointer(), blobSignature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature));       // ��Ʈ �ñ״��� ����.

}

void InitDirect3DApp::BuildPSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

    // opaque objects
    psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS =
    {
        reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
        mShaders["standardVS"]->GetBufferSize()
    };
    psoDesc.PS =
    {
        reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
        mShaders["opaquePS"]->GetBufferSize()
    };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;     // �ﰢ�� ���
    psoDesc.NumRenderTargets = 1;       // ���̰��ϸ� 2���� ��.
    psoDesc.RTVFormats[0] = mBackBufferFormat;
    psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    psoDesc.SampleDesc.Quality = m4xMsaaQuality ? (m4xMsaaQuality - 1) : 0;
    psoDesc.DSVFormat = mDepthStencilFormat;

    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

    // alpha tested objects
    D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = psoDesc;
    alphaTestedPsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(mShaders["alphaTestedPS"]->GetBufferPointer()),
        mShaders["alphaTestedPS"]->GetBufferSize()
    };
    alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;     // �ڿ��� �������ϱ� �ø� ����

    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs["alphaTested"])));

    // transparent objects;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = psoDesc;
    D3D12_RENDER_TARGET_BLEND_DESC transparentBlendDesc;
    transparentBlendDesc.BlendEnable = true;
    transparentBlendDesc.LogicOpEnable = false;
    transparentBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    transparentBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    transparentBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
    transparentBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
    transparentBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
    transparentBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    transparentBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
    transparentBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    transparentPsoDesc.BlendState.RenderTarget[0] = transparentBlendDesc;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs["transparent"])));

    // skybox
    D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = psoDesc;
    skyPsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(mShaders["skyVS"]->GetBufferPointer()),
        mShaders["skyVS"]->GetBufferSize()
    };
    skyPsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(mShaders["skyPS"]->GetBufferPointer()),
        mShaders["skyPS"]->GetBufferSize()
    };

    skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&mPSOs["skybox"])));
}
