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

    // �ʱ�ȭ ��ɵ�
    BuildInputLayout();
    BuildGeometry();
    BuildMaterials();
    BuildRenderItem();      // �������� �������� ���̴� ������ �߿���. BuildConstantBuffer �̰͵� 2�� �ϰ� �ؾ���.
    BuildShader();
    BuildConstantBuffer();
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
    XMMATRIX proj = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, proj);
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
    // ���� ��ǥ�� ���� ��ǥ�� ��ȯ
    float x = mRadius * sinf(mPhi) * cosf(mTheta);
    float z = mRadius * sinf(mPhi) * sinf(mTheta);
    float y = mRadius * cosf(mPhi);

    // �þ����
    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);      // ī�޶� ���
    XMStoreFloat4x4(&mView, view);
}

void InitDirect3DApp::UpdateObjectCB(const GameTimer& gt)
{
    // ���� ������Ʈ ��� ���� ����
    for (auto& item : mRenderItems)
    {
        XMMATRIX world = XMLoadFloat4x4(&item->World);      // ������ �ֱ�. �ε�.

        // ��� ���� ����
        ObjectConstants objConstants;
        XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));     // world���� �����ͼ� ��������. 

        UINT elementIndex = item->ObjCBIndex;
        UINT elementByteSIze = (sizeof(ObjectConstants) + 255) & ~255;
        memcpy(&mObjectMappedData[elementIndex * elementByteSIze], &objConstants, sizeof(ObjectConstants));       // �޸� ī��
    }
}

void InitDirect3DApp::UpdateMaterialCB(const GameTimer& gt)
{
    for (auto& item : mMaterials) {
        MaterialInfo* mat = item.second.get();

        MaterialsConstants matConstants;
        matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
        matConstants.FresnelIR0 = mat->FresnelR0;
        matConstants.Roughness = mat->Roughness;

        UINT elementIndex = mat->MatCBIndex;
        UINT elementByteSize = (sizeof(MaterialsConstants) + 255) & ~255;
        memcpy(&mMaterialMappedData[elementIndex * elementByteSize], &matConstants, sizeof(matConstants));
    }
}

void InitDirect3DApp::UpdatePassCB(const GameTimer& gt)
{
    // ���� ��� ���� ����
    PassConstants mainPass;
    XMMATRIX view = XMLoadFloat4x4(&mView);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);

    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX viewProj = XMMatrixMultiply(view, proj);

    XMStoreFloat4x4(&mainPass.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&mainPass.InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&mainPass.Proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&mainPass.InvProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&mainPass.ViewProj, XMMatrixTranspose(viewProj));

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
    // �׸��� ����
    mCommandList->SetPipelineState(mPSO.Get());

    // ��Ʈ �ñ״�ó ���ε�
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    // ���� ��� ���� ���ε�
    D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = mPassCB->GetGPUVirtualAddress();
    mCommandList->SetGraphicsRootConstantBufferView(2, passCBAddress);

    DrawRenderItems();
}

void InitDirect3DApp::DrawRenderItems()
{
    UINT objCBByteSize = (sizeof(ObjectConstants) + 255) & ~255;
    UINT matCBByteSize = (sizeof(MaterialsConstants) + 255) & ~255;

    for (size_t i = 0; i < mRenderItems.size(); ++i) 
    {
        auto item = mRenderItems[i].get();

        // ���� ������Ʈ ��� ���� �� ����
        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = mObjectCB->GetGPUVirtualAddress();
        objCBAddress += item->ObjCBIndex * objCBByteSize;

        mCommandList->SetGraphicsRootConstantBufferView(0, objCBAddress);

        // ���� ���� ��� ���� �� ����
        D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = mMaterialCB->GetGPUVirtualAddress();
        matCBAddress += item->Mat->MatCBIndex * matCBByteSize;

        mCommandList->SetGraphicsRootConstantBufferView(1, matCBAddress);

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
    if ((btnState & MK_LBUTTON) != 0) 
    {
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

        mTheta += dx;
        mPhi += dy;

        mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
    }
    else if ((btnState & MK_RBUTTON) != 0)
    {
        float dx = 0.2f * static_cast<float>(x - mLastMousePos.x);
        float dy = 0.2f * static_cast<float>(y - mLastMousePos.y);

        mRadius += dx - dy;

        mRadius = MathHelper::Clamp(mRadius, 3.0f, 150.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

void InitDirect3DApp::BuildInputLayout()        // ó�� ������ �� �ʱ�ȭ
{
    mInputLayout =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
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
    skull->DiffuseAlbedo = XMFLOAT4(Colors::White);
    skull->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
    skull->Roughness = 0.3f;
    mMaterials[skull->Name] = std::move(skull);
}

void InitDirect3DApp::BuildRenderItem()
{
    // �ٴ�
    auto gridItem = std::make_unique<RenderItem>();
    gridItem->ObjCBIndex = 0;
    gridItem->World = MathHelper::Identity4x4();
    gridItem->Geo = mGeometries["Grid"].get();
    gridItem->Mat = mMaterials["Gray"].get();
    gridItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    gridItem->IndexCount = gridItem->Geo->IndexCount;
    mRenderItems.push_back(std::move(gridItem));

    // �ڽ�
    auto boxItem = std::make_unique<RenderItem>();
    boxItem->ObjCBIndex = 1;
    XMStoreFloat4x4(&boxItem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));      // ��ġ ���� ����. ���� �÷���
    boxItem->Geo = mGeometries["Box"].get();
    boxItem->Mat = mMaterials["Blue"].get();
    boxItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    boxItem->IndexCount = boxItem->Geo->IndexCount;
    mRenderItems.push_back(std::move(boxItem));

    // �ذ�
    auto skullItem = std::make_unique<RenderItem>();
    skullItem->ObjCBIndex = 2;
    XMStoreFloat4x4(&skullItem->World, XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(0.0f, 1.0f, 0.0f));      // ��ġ ���� ����. ���� �÷���
    skullItem->Geo = mGeometries["Skull"].get();
    skullItem->Mat = mMaterials["Skull"].get();
    skullItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    skullItem->IndexCount = skullItem->Geo->IndexCount;
    mRenderItems.push_back(std::move(skullItem));

    UINT objCBIndex = 3;
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
        leftCylItem->ObjCBIndex = objCBIndex++;
        leftCylItem->Geo = mGeometries["Cylinder"].get();
        leftCylItem->Mat = mMaterials["Green"].get();
        leftCylItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftCylItem->IndexCount = leftCylItem->Geo->IndexCount;
        mRenderItems.push_back(std::move(leftCylItem));     // �Ű��ֱ�

        // ������ �Ǹ���
        XMStoreFloat4x4(&rightCylItem->World, rightCylWorld);
        rightCylItem->ObjCBIndex = objCBIndex++;
        rightCylItem->Geo = mGeometries["Cylinder"].get();
        rightCylItem->Mat = mMaterials["Green"].get();
        rightCylItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightCylItem->IndexCount = rightCylItem->Geo->IndexCount;
        mRenderItems.push_back(std::move(rightCylItem));     // �Ű��ֱ�

        // ���� ���Ǿ�
        XMStoreFloat4x4(&leftSphereItem->World, leftSphereWorld);
        leftSphereItem->ObjCBIndex = objCBIndex++;
        leftSphereItem->Geo = mGeometries["Sphere"].get();
        leftSphereItem->Mat = mMaterials["Blue"].get();
        leftSphereItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftSphereItem->IndexCount = leftSphereItem->Geo->IndexCount;
        mRenderItems.push_back(std::move(leftSphereItem));     // �Ű��ֱ�

        // ������ ���Ǿ�
        XMStoreFloat4x4(&rightSphereItem->World, rightSphereWorld);
        rightSphereItem->ObjCBIndex = objCBIndex++;
        rightSphereItem->Geo = mGeometries["Sphere"].get();
        rightSphereItem->Mat = mMaterials["Blue"].get();
        rightSphereItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightSphereItem->IndexCount = rightSphereItem->Geo->IndexCount;
        mRenderItems.push_back(std::move(rightSphereItem));     // �Ű��ֱ�
    }
}

void InitDirect3DApp::BuildShader()
{
    mVSByteCode = d3dUtil::CompileShader(L"Color.hlsl", nullptr, "VS", "vs_5_0");
    mPSByteCode = d3dUtil::CompileShader(L"Color.hlsl", nullptr, "PS", "ps_5_0");
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
    size = sizeof(MaterialsConstants);
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

void InitDirect3DApp::BuildRootSignature()
{
    CD3DX12_ROOT_PARAMETER param[3];
    param[0].InitAsConstantBufferView(0);       // 0�� ��Ʈ �ñ״��Ŀ� b0 : CBV(���� ������Ʈ ������ۺ�)     �������� ����, �� ���ε�
    param[1].InitAsConstantBufferView(1);       // 1�� -> b1 : ���� ���� CBV;
    param[2].InitAsConstantBufferView(2);       // 2�� -> b1 : ���� CBV

    D3D12_ROOT_SIGNATURE_DESC sigDesc = CD3DX12_ROOT_SIGNATURE_DESC(_countof(param), param);       // ����
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
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;     // �ﰢ�� ���
    psoDesc.NumRenderTargets = 1;       // ���̰��ϸ� 2���� ��.
    psoDesc.RTVFormats[0] = mBackBufferFormat;
    psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    psoDesc.SampleDesc.Quality = m4xMsaaQuality ? (m4xMsaaQuality - 1) : 0;
    psoDesc.DSVFormat = mDepthStencilFormat;

    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}
