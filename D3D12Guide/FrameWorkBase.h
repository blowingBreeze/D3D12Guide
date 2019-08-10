#pragma once


#include <stdexcept>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

using namespace Microsoft::WRL;
using namespace DirectX;

static const int BUFFER_COUNT = 2;
class FrameWorkBase
{
public:
    FrameWorkBase();
    virtual void Init(HINSTANCE hInstance,int nCmdShow);
    virtual int Run();
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
protected:
    void InitWindow(HINSTANCE hInstance, int nCmdShow);
    void InitDevice();
    void InitFence();
    void InitGraphicsCommand();
    void InitSwapChain();
    void InitDescriptorHeap();
    void InitRenderTargets();
    void InitRootSignature();
    void InitShader();
    void InitVertex();

protected:
    void WaitForPreviousFrame();
    void PopulateCommandList();

protected:
    void OnUpdate();
    void OnRender();

protected:
    HWND mhMainWind;
    UINT mWidth;
    UINT mHeight;
    DXGI_SWAP_CHAIN_DESC  mSwapChainDesc;
    UINT64 mFenceValue;
    HANDLE mFenceEvent;
    UINT mFrameIndex;

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

inline std::string HrToString(HRESULT hr)
{
    char s_str[64] = {};
    sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    return std::string(s_str);
}

class HrException : public std::runtime_error
{
public:
    HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
    HRESULT Error() const { return m_hr; }
private:
    const HRESULT m_hr;
};

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw HrException(hr);
    }
}
