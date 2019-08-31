#pragma once
#include "../FrameWork/FrameWorkBase.h"
class InitializeGuide :
    public FrameWorkBase
{
public:
    InitializeGuide(HINSTANCE hInstance)
        :FrameWorkBase(hInstance)
    {

    }

protected:
    virtual void Init();
    virtual void OnRender();

private:
    void BuildRootSignature();
    void BuildTriangles();
    void BuildPSO();
};

