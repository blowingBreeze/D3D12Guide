#include "FrameWorkBase.h"
#include <Windowsx.h>
#include <string>

FrameWorkBase::FrameWorkBase()
    :mWidth(1366), mHeight(768),
    mRtvDescriptorSize(0), mDsvDescriptorSize(0), mCbvUavDescriptorSize(0),
    mhMainWind(nullptr),
    mViewport(), mScissorRect(),
    mBackBufferIndex(0)
{
}

FrameWorkBase::FrameWorkBase(HINSTANCE hInstance)
    :mWidth(1366), mHeight(768),
    mRtvDescriptorSize(0), mDsvDescriptorSize(0), mCbvUavDescriptorSize(0),
    mhMainWind(nullptr),
    mViewport(), mScissorRect(),
    mBackBufferIndex(0),
    mhMainInstance(hInstance)
{
    mpFrameWork = this;
}

//static
FrameWorkBase* FrameWorkBase::GetFrameWork()
{
    if (!mpFrameWork)
    {
        mpFrameWork = new FrameWorkBase;
    }
    return mpFrameWork;
}

void FrameWorkBase::Init()
{
    InitWindow();
    InitDevice();
    InitFence();
    InitGraphicsCommand();

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
    
    InitSwapChain();
    InitRtvAndDsvDescriptorHeaps();
    InitRenderTargets();
    InitDepstencil();
}

void FrameWorkBase::UnInit()
{
    WaitForPreviousFrame();
}

int FrameWorkBase::Run()
{
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        // Process any messages in the queue.
        // If there are Window messages then process them.
        if (PeekMessage(&msg, 0,0 ,0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        OnUpdate();
        OnRender();
    }

    UnInit();
    return (int)msg.wParam;
}

LRESULT FrameWorkBase::ExtendMsgHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_ACTIVATE:
    {

    }
    return 0;
    case WM_CREATE:
    {
        // Save the pFrameWork* passed in to CreateWindow.
        LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
    }
    return 0;

    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_MOUSEMOVE:
        OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_PAINT:
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    // Handle any messages the switch statement didn't.
    return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT FrameWorkBase::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return FrameWorkBase::GetFrameWork()->ExtendMsgHandler(hWnd, message, wParam, lParam);
}

FrameWorkBase::~FrameWorkBase()
{
}

void FrameWorkBase::InitWindow()
{
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = mhMainInstance;
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = L"D3D12Guide";
    if (!RegisterClass(&wc))
    {
        MessageBox(0, L"RegisterClass Failed.", 0, 0);
        return;
    }

    RECT windowRect = { 0, 0, static_cast<LONG>(this->mWidth), static_cast<LONG>(this->mHeight) };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // Create the window and store a handle to it.
    mhMainWind = CreateWindow(
        L"D3D12Guide",
        L"D3D12Guide",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, mWidth, mHeight, 0, 0, mhMainInstance, 0);

    if (!mhMainWind)
    {
        MessageBox(0, L"CreateWindow Failed.", 0, 0);
        return;
    }

    ShowWindow(mhMainWind, SW_SHOW);
    UpdateWindow(mhMainWind);
}

void FrameWorkBase::InitDevice()
{
    UINT dxgiFactoryFlags = 0;
    HRESULT hResult = E_FAIL;

#if defined(DEBUG) || defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();

        // Enable additional debug layers.
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    //create d3d device factory object
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mD3DFactory)));
    hResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mD3DDevice));
    if (FAILED(hResult))
    {
        //try to create wrap device when creating hard device failed
        ComPtr<IDXGIAdapter> pWarpAdapter;
        ThrowIfFailed(mD3DFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));//enum all wrap adapter,use the first one
        ThrowIfFailed(D3D12CreateDevice(pWarpAdapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&mD3DDevice)));
    }

    mRtvDescriptorSize = mD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    mDsvDescriptorSize = mD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    mCbvUavDescriptorSize = mD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void FrameWorkBase::InitFence()
{
    ThrowIfFailed(mD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mD3DFence)));

}

void FrameWorkBase::InitGraphicsCommand()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(mD3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));
    ThrowIfFailed(mD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));
    ThrowIfFailed(mD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mDirectCmdListAlloc.Get(), nullptr, IID_PPV_ARGS(mCommandList.GetAddressOf())));

    //we need to reset it commandlist when ref it at fist time,and need to close it before reset
    mCommandList->Close();
}

void FrameWorkBase::InitSwapChain()
{
    // Describe and create the swap chain.
    mSwapChain.Reset();
    mSwapChainDesc.BufferCount = BUFFER_COUNT;
    mSwapChainDesc.Width = mWidth;
    mSwapChainDesc.Height = mHeight;
    mSwapChainDesc.Format = mBackBufferFormat;
    mSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    mSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    mSwapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(mD3DFactory->CreateSwapChainForHwnd(
        mCommandQueue.Get(),       // Swap chain needs the queue so that it can force a flush on it.
        mhMainWind,
        &mSwapChainDesc,
        nullptr,nullptr,
        swapChain.GetAddressOf()
    ));
    ThrowIfFailed(swapChain.As(&mSwapChain));
}

void FrameWorkBase::InitRtvAndDsvDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = mSwapChainDesc.BufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(mD3DDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(mD3DDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void FrameWorkBase::InitRenderTargets()
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());

    // Create a RTV for each frame.
    for (UINT n = 0; n < BUFFER_COUNT; n++)
    {
        ThrowIfFailed(mSwapChain->GetBuffer(n, IID_PPV_ARGS(&mRenderTargets[n])));
        mD3DDevice->CreateRenderTargetView(mRenderTargets[n].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += mRtvDescriptorSize;
    }
}

void FrameWorkBase::InitDepstencil()
{
    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = mWidth;
    depthStencilDesc.Height = mHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;

    depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = mDepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

    ThrowIfFailed(mD3DDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = mDepthStencilFormat;
    dsvDesc.Texture2D.MipSlice = 0;
    // Create descriptor to mip level 0 of entire resource using the
    // format of the resource.
    mD3DDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, DepthStencilView());


    // Transition the resource from its initial state to be used as a depth buffer.
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
}

void FrameWorkBase::WaitForPreviousFrame()
{
    mFenceValue++;

    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // Signal and increment the fence value.
    ThrowIfFailed(mCommandQueue->Signal(mD3DFence.Get(), mFenceValue));

    // Wait until the previous frame is finished.
    if (mD3DFence->GetCompletedValue() < mFenceValue)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(mD3DFence->SetEventOnCompletion(mFenceValue, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

FrameWorkBase* FrameWorkBase::mpFrameWork = nullptr;

