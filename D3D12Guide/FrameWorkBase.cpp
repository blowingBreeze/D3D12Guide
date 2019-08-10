#include "FrameWorkBase.h"
#include <string>

FrameWorkBase::FrameWorkBase()
    :mWidth(1366), mHeight(768),
    mRtvDescriptorSize(0), mDsvDescriptorSize(0), mCbvUavDescriptorSize(0),
    mhMainWind(nullptr),
    mViewport(), mScissorRect(),
    mFrameIndex(0)
{
    mViewport.TopLeftX = 0;
    mViewport.TopLeftY = 0;
    mViewport.Width = 1366;
    mViewport.Height = 768;
    mViewport.MinDepth = D3D12_MIN_DEPTH;
    mViewport.MaxDepth = D3D12_MAX_DEPTH;

    mScissorRect.left = 0;
    mScissorRect.top = 0;
    mScissorRect.right = 1366;
    mScissorRect.bottom = 768;
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
    InitRootSignature();
    InitShader();
    InitVertex();
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
    FrameWorkBase* pFrameWork = reinterpret_cast<FrameWorkBase*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    switch (message)
    {
    case WM_CREATE:
    {
        // Save the pFrameWork* passed in to CreateWindow.
        LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
    }
    return 0;

    case WM_KEYDOWN:
        return 0;

    case WM_KEYUP:
        return 0;

    case WM_PAINT:
        pFrameWork->OnUpdate();
        pFrameWork->OnRender();
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    // Handle any messages the switch statement didn't.
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
        this);

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

    ComPtr<IDXGISwapChain> swapChain;
    ThrowIfFailed(mD3DFactory->CreateSwapChain(
        mCommandQueue.Get(),       // Swap chain needs the queue so that it can force a flush on it.
        &mSwapChainDesc,
        swapChain.GetAddressOf()
    ));
    ThrowIfFailed(swapChain.As(&mSwapChain));
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

void FrameWorkBase::InitRootSignature()
{
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.NumParameters = 0;
    rootSignatureDesc.NumStaticSamplers = 0;
    rootSignatureDesc.pParameters = nullptr;
    rootSignatureDesc.pStaticSamplers = nullptr;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    ThrowIfFailed(mD3DDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));
}

void FrameWorkBase::InitShader()
{
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    ThrowIfFailed(D3DCompileFromFile(std::wstring(L"./Assets/Shaders/shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
    ThrowIfFailed(D3DCompileFromFile(std::wstring(L"./Assets/Shaders/shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_SHADER_BYTECODE vs;
    vs.pShaderBytecode = vertexShader.Get()->GetBufferPointer();
    vs.BytecodeLength = vertexShader.Get()->GetBufferSize();

    D3D12_SHADER_BYTECODE ps;
    ps.pShaderBytecode = pixelShader.Get()->GetBufferPointer();
    ps.BytecodeLength = pixelShader.Get()->GetBufferSize();

    D3D12_RASTERIZER_DESC rasterizerState;
    rasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
    rasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
    rasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    rasterizerState.FrontCounterClockwise = FALSE;
    rasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerState.DepthClipEnable = TRUE;
    rasterizerState.MultisampleEnable = FALSE;
    rasterizerState.AntialiasedLineEnable = FALSE;
    rasterizerState.ForcedSampleCount = 0;
    rasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    D3D12_BLEND_DESC blendDesc;
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
    {
        FALSE,FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        blendDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;

    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS = vs;
    psoDesc.PS = ps;
    psoDesc.RasterizerState = rasterizerState;
    psoDesc.BlendState = blendDesc;
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    ThrowIfFailed(mD3DDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPipelineState)));

    // Create the command list.
    ThrowIfFailed(mD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mDirectCmdListAlloc.Get(), mPipelineState.Get(), IID_PPV_ARGS(&mCommandList)));

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    ThrowIfFailed(mCommandList->Close());
}

void FrameWorkBase::InitVertex()
{
    // Create the vertex buffer.
    {
        // Define the geometry for a triangle.
        Vertex triangleVertices[] =
        {
            { { 0.0f, 0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 0.25f, -0.25f , 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.25f, -0.25f , 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        };

        const UINT vertexBufferSize = sizeof(triangleVertices);

        D3D12_HEAP_PROPERTIES heapPropties;
        heapPropties.Type = D3D12_HEAP_TYPE:: D3D12_HEAP_TYPE_UPLOAD;
        heapPropties.CPUPageProperty =D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapPropties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
        heapPropties.CreationNodeMask = 1;
        heapPropties.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC resourceDesc;
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = vertexBufferSize;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;

        // Note: using upload heaps to transfer static data like vert buffers is not 
        // recommended. Every time the GPU needs it, the upload heap will be marshalled 
        // over. Please read up on Default Heap usage. An upload heap is used here for 
        // code simplicity and because there are very few verts to actually transfer.
        ThrowIfFailed(mD3DDevice->CreateCommittedResource(
            &heapPropties,
            D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mVertexBuffer)));

        // Copy the triangle data to the vertex buffer.
        UINT8* pVertexDataBegin;
        D3D12_RANGE readRange;        // We do not intend to read from this resource on the CPU.
        readRange.Begin = 0;
        readRange.End = 0;

        ThrowIfFailed(mVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
        mVertexBuffer->Unmap(0, nullptr);

        // Initialize the vertex buffer view.
        mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
        mVertexBufferView.StrideInBytes = sizeof(Vertex);
        mVertexBufferView.SizeInBytes = vertexBufferSize;

        ThrowIfFailed(mD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mD3DFence)));
        mFenceValue = 1;

        // Create an event handle to use for frame synchronization.
        mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (mFenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        // Wait for the command list to execute; we are reusing the same command 
        // list in our main loop but for now, we just want to wait for setup to 
        // complete before continuing.
        WaitForPreviousFrame();
    }
}

void FrameWorkBase::WaitForPreviousFrame()
{
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // Signal and increment the fence value.
    const UINT64 fence = mFenceValue;
    ThrowIfFailed(mCommandQueue->Signal(mD3DFence.Get(), fence));
    mFenceValue++;

    // Wait until the previous frame is finished.
    if (mD3DFence->GetCompletedValue() < fence)
    {
        ThrowIfFailed(mD3DFence->SetEventOnCompletion(fence, mFenceEvent));
        WaitForSingleObject(mFenceEvent, INFINITE);
    }

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
}

void FrameWorkBase::OnUpdate()
{
}

void FrameWorkBase::OnRender()
{
    // Record all the commands we need to render the scene into the command list.
    PopulateCommandList();

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame.
    ThrowIfFailed(mSwapChain->Present(1, 0));

    WaitForPreviousFrame();
}

void FrameWorkBase::PopulateCommandList()
{
    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    ThrowIfFailed(mDirectCmdListAlloc->Reset());

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPipelineState.Get()));

    // Set necessary state.
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
    mCommandList->RSSetViewports(1, &mViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    D3D12_RESOURCE_BARRIER resourceBarrier;
    resourceBarrier.Transition.pResource = mRenderTargets[mFrameIndex].Get();
    resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT;
    resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
    resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
    resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    // Indicate that the back buffer will be used as a render target.
    mCommandList->ResourceBarrier(1, &resourceBarrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
    rtvHandle.ptr = mRtvHeap->GetCPUDescriptorHandleForHeapStart().ptr+ (INT64)mFrameIndex*(UINT64)mRtvDescriptorSize;
    mCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    mCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mCommandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
    mCommandList->DrawInstanced(3, 1, 0, 0);

    D3D12_RESOURCE_BARRIER resourceBarrier2;
    resourceBarrier2.Transition.pResource = mRenderTargets[mFrameIndex].Get();
    resourceBarrier2.Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
    resourceBarrier2.Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT;
    resourceBarrier2.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    resourceBarrier2.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
    resourceBarrier2.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    // Indicate that the back buffer will now be used to present.
    mCommandList->ResourceBarrier(1, &resourceBarrier2);

    ThrowIfFailed(mCommandList->Close());
}

