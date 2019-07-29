// D3D12Guide.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//


#include <iostream>
#include <windows.h>
#include <stdexcept>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>

#include <wrl.h>
#include <vector>
#include "FrameWorkBase.h"
using Microsoft::WRL::ComPtr;


void LogOutputDisplayModes(ComPtr<IDXGIOutput> output, DXGI_FORMAT format)
{
    UINT count = 0;
    UINT flags = 0;
    output->GetDisplayModeList(format, flags, &count, nullptr);

    std::vector<DXGI_MODE_DESC> modeList(count);
    output->GetDisplayModeList(format, flags, &count, &modeList[0]);

    for (auto& x : modeList)
    {
        std::wcout << x.RefreshRate.Denominator << '\t' << x.ScanlineOrdering << std::endl;
    }
}

void LogAdapterOutputs(ComPtr<IDXGIAdapter> adapter)
{
    UINT i=0;
    ComPtr<IDXGIOutput> output = nullptr;
    while (adapter->EnumOutputs(i,&output)!=DXGI_ERROR_NOT_FOUND)
    {
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);

        std::wcout << desc.DeviceName << std::endl;
        LogOutputDisplayModes(output,DXGI_FORMAT_B8G8R8A8_UNORM);
        ++i;
    }
}

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    std::cout << "Hello World!\n";

    ComPtr<IDXGIFactory4> factory;
    UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif
    CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));
    UINT i = 0;
   ComPtr<IDXGIAdapter> adapter = nullptr;
    while (factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        std::wcout << desc.Description<<std::endl;
        LogAdapterOutputs(adapter);
        ++i;
    }

    FrameWorkBase frameWork;
    frameWork.Init(hInstance, nCmdShow);
    return frameWork.Run();
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
