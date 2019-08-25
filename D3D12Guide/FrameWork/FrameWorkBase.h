#pragma once
#include "Prerequisites.h"

class FrameWorkBase
{
public:
    virtual void Init(HINSTANCE hInstance,int nCmdShow);
    virtual void UnInit();
    virtual int Run();
    virtual void OnUpdate();
    virtual void OnRender();

    virtual LRESULT CALLBACK ExtendMsgHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    virtual ~FrameWorkBase();
public:


protected:
    static FrameWorkBase* GetFrameWork();
    FrameWorkBase();

private:
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    void InitWindow(HINSTANCE hInstance, int nCmdShow);
    void InitDevice();
    void InitFence();
    void InitGraphicsCommand();
    void InitSwapChain();
    void InitDescriptorHeap();
    void InitRenderTargets();
    void InitRootSignature();
    void InitShaderAndPipeLine();
    void InitVertex();

protected:
    void WaitForPreviousFrame();
    void PopulateCommandList();

protected:
    static FrameWorkBase* mpFrameWork;
    HINSTANCE mhMainInstance = nullptr;
    HWND mhMainWind = nullptr;
    UINT mWidth = 0;
    UINT mHeight = 0;
    DXGI_SWAP_CHAIN_DESC1  mSwapChainDesc = {};
    UINT64 mFenceValue = 0;
    HANDLE mFenceEvent = nullptr;
    UINT mFrameIndex = 0;

    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT4 color;
    };
protected:

    D3D12_VIEWPORT mViewport;
    D3D12_RECT mScissorRect;
    UINT mRtvDescriptorSize;
    UINT mDsvDescriptorSize;
    UINT mCbvUavDescriptorSize;
    ComPtr<IDXGIFactory4> mD3DFactory;
    ComPtr<ID3D12Device> mD3DDevice;
    ComPtr<ID3D12Fence> mD3DFence;
    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;
    ComPtr<IDXGISwapChain3> mSwapChain;
    ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    ComPtr<ID3D12DescriptorHeap> mDsvHeap;
    ComPtr<ID3D12Resource> mRenderTargets[BUFFER_COUNT];
    ComPtr<ID3D12RootSignature> mRootSignature;
    ComPtr<ID3D12PipelineState> mPipelineState;
    ComPtr<ID3D12Resource> mVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
};
