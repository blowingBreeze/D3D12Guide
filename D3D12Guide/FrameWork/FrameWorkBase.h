#pragma once
#include "Prerequisites.h"

class FrameWorkBase
{
public:
    virtual ~FrameWorkBase();
    virtual void Init();
    virtual void UnInit();
    virtual int Run();
    virtual void OnUpdate() {};
    virtual void OnRender() {};

    virtual void OnMouseMove(WPARAM btnState, int x, int y){}
    virtual void OnMouseDown(WPARAM btnState, int x, int y){}
    virtual void OnMouseUp(WPARAM btnState, int x, int y){}

    virtual LRESULT CALLBACK ExtendMsgHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

protected:
    FrameWorkBase();
    FrameWorkBase(HINSTANCE hInstance);
    static FrameWorkBase* GetFrameWork();
    D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackRtvHandler() const 
    { 
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
        rtvHandle.ptr = mRtvHeap->GetCPUDescriptorHandleForHeapStart().ptr + (INT64)mBackBufferIndex * (UINT64)mRtvDescriptorSize;
        return rtvHandle;
    }
    float AspectRatio()const
    {
        return static_cast<float>(mWidth) / mHeight;
    }

private:
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    void InitWindow();
    void InitDevice();
    void InitFence();
    void InitGraphicsCommand();
    void InitSwapChain();
    void InitRtvAndDsvDescriptorHeaps();
    void InitRenderTargets();
    void InitDepstencil();
protected:
    void WaitForPreviousFrame();
    ID3D12Resource* CurrentBackBuffer() const { return mRenderTargets[mBackBufferIndex].Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const 
    {
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
            mBackBufferIndex, mRtvDescriptorSize);
    }
    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const
    {
        return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
    }
protected:
    static FrameWorkBase* mpFrameWork;
    HINSTANCE mhMainInstance = nullptr;
    HWND mhMainWind = nullptr;
    UINT mWidth = 0;
    UINT mHeight = 0;
    DXGI_SWAP_CHAIN_DESC1  mSwapChainDesc = {};
    UINT64 mFenceValue = 0;
    UINT mBackBufferIndex = 0;

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
    ComPtr<ID3D12DescriptorHeap> mCbvHeap;
    ComPtr<ID3D12Resource> mDepthStencilBuffer;
    ComPtr<ID3D12Resource> mRenderTargets[BUFFER_COUNT];
    ComPtr<ID3D12RootSignature> mRootSignature;
    ComPtr<ID3D12PipelineState> mPipelineState;
    ComPtr<ID3D12Resource> mVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;

    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
};
