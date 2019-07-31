#include "FrameWorkBase.h"

FrameWorkBase::FrameWorkBase()
    :mWidth(1366),mHeight(768),
    mRtvDescriptorSize(0), mDsvDescriptorSize(0), mCbvUavDescriptorSize(0),
    mhMainWind(nullptr)
{
}

void FrameWorkBase::Init(HINSTANCE hInstance, int nCmdShow)
{
    InitWindow(hInstance, nCmdShow);
    InitDevice();
    InitFence();
    InitGraphicsCommand();
    InitSwapChain();
    InitDescriptorHeap();
    InitRenderTargets();
}

int FrameWorkBase::Run()
{
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        // Process any messages in the queue.
        if (PeekMessage(&msg, mhMainWind, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return static_cast<char>(msg.wParam);
}

LRESULT FrameWorkBase::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hWnd, message, wParam, lParam);
}

void FrameWorkBase::InitWindow(HINSTANCE hInstance,int nCmdShow)
{
    // Initialize the window class.
    WNDCLASSEX windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = L"D3D12Guide";
    RegisterClassEx(&windowClass);

    RECT windowRect = { 0, 0, static_cast<LONG>(this->mWidth), static_cast<LONG>(this->mHeight) };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // Create the window and store a handle to it.
    mhMainWind = CreateWindow(
        windowClass.lpszClassName,
        L"D3DGuide",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,        // We have no parent window.
        nullptr,        // We aren't using menus.
        hInstance,
        nullptr);

    ShowWindow(mhMainWind, nCmdShow);
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
}

void FrameWorkBase::InitFence()
{
    ThrowIfFailed(mD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mD3DFence)));
    mRtvDescriptorSize = mD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    mDsvDescriptorSize = mD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    mCbvUavDescriptorSize = mD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
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
    mSwapChainDesc.BufferDesc.Width = 1366;
    mSwapChainDesc.BufferDesc.Height = 768;
    mSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    mSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    mSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    mSwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER::DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST;
    mSwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING::DXGI_MODE_SCALING_CENTERED;
    mSwapChainDesc.Windowed = true;
    mSwapChainDesc.OutputWindow = mhMainWind;
    mSwapChainDesc.BufferCount = BUFFER_COUNT;
    mSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    mSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_DISCARD;
    mSwapChainDesc.SampleDesc.Count = 1;
    mSwapChainDesc.SampleDesc.Quality = 0;

    ThrowIfFailed(mD3DFactory->CreateSwapChain(
        mCommandQueue.Get(),       // Swap chain needs the queue so that it can force a flush on it.
        &mSwapChainDesc,
        mSwapChain.GetAddressOf() 
    ));
}

void FrameWorkBase::InitDescriptorHeap()
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


