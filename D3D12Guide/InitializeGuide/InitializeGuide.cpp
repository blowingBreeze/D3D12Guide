#include "InitializeGuide.h"


InitializeGuide::InitializeGuide()
{
    mpFrameWork = GetFrameWork();
}

void InitializeGuide::Init(HINSTANCE hInstance, int nCmdShow)
{
    mpFrameWork->Init(hInstance, nCmdShow);
}

int InitializeGuide::Run()
{
    return mpFrameWork->Run();
}
