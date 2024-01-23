#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "../Common/d3dUtil.h"
#include "../Common/GameTimer.h"

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")      // 정적 쉐이더 컴파일 라이브러리
#pragma comment(lib, "D3D12.lib")       // 다이렉트 라이브러리
#pragma comment(lib, "dxgi.lib")        // DXGI 라이브러리

// using namespaces
using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Microsoft::WRL;     // COM 객체를 사용하기 위해서

class D3DApp
{
protected:
    D3DApp(HINSTANCE hInstance);
    D3DApp(const D3DApp& rhs) = delete;
    D3DApp& operator=(const D3DApp& rhs) = delete;
    virtual ~D3DApp();

public:
    static D3DApp* GetApp();

    HINSTANCE AppInst()const;
    HWND      MainWnd()const;
    float     AspectRatio()const;

    int Run();

    virtual bool Initialize();
    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);     // 메세지 처리

protected:
    virtual void OnResize();
    virtual void Update(const GameTimer& gt) = 0;       // 추상화 해둔 것.
    virtual void DrawBegin(const GameTimer& gt) = 0;
    virtual void Draw(const GameTimer& gt) = 0;     // 0 붙으면 자식이 꼭 정의해야함. 선언만 가능해짐.
    virtual void DrawEnd(const GameTimer& gt) = 0;

    // Convenience overrides for handling mouse input.
    virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
    virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
    virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

protected:
    bool InitMainWindow();      // 윈도우 처음 설정해줌.
    void CalculateFrameStats();

protected:
    bool InitDirect3D();        // 다이렉트 초기화

    void Create4xMsaaState();       // 멀티샘플링안티애일리어싱
    void CreateCommandObjects();        // 커맨드
    void CreateSwapChain();     // 스왑체인
    void CreateCommandFence();      // 팬스, 기다릴 때
    void CreateDescriptorSize();        // 서술자
    void CreateRtvDescriptorHeaps();        // 랜터타입버퍼 힙에 서술자가 들어가 있음.
    void CreateDsvDescriptorHeaps();        // 뎁스텐실버퍼
    void CreateViewPort();      // 뷰포트 생성

protected:
    void FlushCommandQueue();       // 내보낸다. 보낸 데이터 처리될 때까지 기다리기

public:
    bool Get4xMassState()const;
    void Set4xMsaaState(bool value);

    ID3D12Resource* CurrentBackBuffer()const;

    D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

protected:
    ComPtr<IDXGIFactory4>       mdxgiFactory;
    ComPtr<ID3D12Device>       md3dDevice;

    // 다중 샘플링 관련 변수 - 사용 위치는 백버퍼
    bool m4xMsaaState = false;
    UINT m4xMsaaQuality = 0;

    D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    // 커맨드 관련 변수
    ComPtr<ID3D12CommandQueue>          mCommandQueue;
    ComPtr<ID3D12CommandAllocator>      mCommandListAlloc;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;

    // 스왑체인 변수
    ComPtr<IDXGISwapChain>                      mSwapChain;

    static const int                                           SwapChainBufferCount = 2;
    int                                                              mCurrentBackBuffer = 0;
    ComPtr<ID3D12Resource>                      mSwapChainBuffer[SwapChainBufferCount];

    // 펜스 관련 변수
    ComPtr<ID3D12Fence>                            mFence;
    UINT64                                                      mCurrentFence = 0;

    // 서술자 크기 변수
    UINT                                                          mRtvDescriptorSize = 0;       // 랜더타겟
    UINT                                                          mDsvDescriptorSize = 0;       // 뎁스스텐실
    UINT                                                          mCbvSrvUavDescriptorSize = 0;         // 콘스탄트버퍼 쉐이더리소스 언오덜드엑세스버퍼

    // 랜더타겟뷰(RTv) 변수
    ComPtr<ID3D12DescriptorHeap>            mRtvHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE         mSwapChainView[SwapChainBufferCount] = {};

    // 뎁스스텐실(DSV) 변수
    ComPtr<ID3D12Resource>                      mDepthStencilBuffer;
    ComPtr<ID3D12DescriptorHeap>            mDsvHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE         mDepthStencileView = {};

    // 뷰포트 변수
    D3D12_VIEWPORT                                     mScreenViewport;
    D3D12_RECT                                              mScissorRect;

protected:      // 윈도우 생성, 변수들

    static D3DApp* mApp;

    HINSTANCE mhAppInst = nullptr; // application instance handle
    HWND      mhMainWnd = nullptr; // main window handle
    bool      mAppPaused = false;  // is the application paused?
    bool      mMinimized = false;  // is the application minimized?
    bool      mMaximized = false;  // is the application maximized?
    bool      mResizing = false;   // are the resize bars being dragged?
    bool      mFullscreenState = false;// fullscreen enabled

    std::wstring mMainWndCaption = L"SungEun";

    int mClientWidth = 800;
    int mClientHeight = 600;

    // Used to keep track of the 밺elta-time?and game time (?.4).
    GameTimer mTimer;
};

