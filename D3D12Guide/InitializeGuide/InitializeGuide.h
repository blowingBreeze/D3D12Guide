#pragma once
#include "../FrameWork/FrameWorkBase.h"
class InitializeGuide :
    private FrameWorkBase
{
public:
    InitializeGuide();
     virtual void Init(HINSTANCE hInstance, int nCmdShow) override;
     virtual int Run() override;
};

