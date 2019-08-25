// D3D12Guide.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <windows.h>
#include "InitializeGuide/InitializeGuide.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    InitializeGuide app;
    app.Init(hInstance, nCmdShow);
    return app.Run();
}
