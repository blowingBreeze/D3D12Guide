#pragma once


#include<stdexcept>
#include<wrl.h>
#include<d3d12.h>
#include <dxgi1_4.h>

using namespace Microsoft::WRL;
class FrameWorkBase
{
public:
    FrameWorkBase();
    virtual void Init(HINSTANCE hInstance,int nCmdShow);
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
protected:
    void InitWindow(HINSTANCE hInstance, int nCmdShow);
    void InitDevice();
    void InitFence();
    void InitGraphicsCommand();
    void InitSwapChain();
    void InitDescriptorHeap();

protected:
    UINT mWidth;
    UINT mHeight;
    DXGI_SWAP_CHAIN_DESC  mSwapChainDesc;
protected:
    HWND mhMainWind;
    UINT mRtvDescriptorSize;
    UINT mDsvDescriptorSize;
    UINT mCbvUavDescriptorSize;
    ComPtr<IDXGIFactory4> mD3DFactory;
    ComPtr<ID3D12Device> mD3DDevice;
    ComPtr<ID3D12Fence> mD3DFence;
    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;
    ComPtr<IDXGISwapChain> mSwapChain;
    ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    ComPtr<ID3D12DescriptorHeap> mDsvHeap;
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
