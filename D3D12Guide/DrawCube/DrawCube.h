#pragma once
#include "../FrameWork/FrameWorkBase.h"
#include "../FrameWork/MeshType.h"
#include "../FrameWork/UploadBuffer.h"
#include "../FrameWork/MathHelper.h"

struct ObjectConstants
{
    XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
};

class DrawCube :
    public FrameWorkBase
{
public:
    DrawCube(HINSTANCE hInstance) 
        :FrameWorkBase(hInstance)
    {

    }

private:
    virtual void Init() override;
    virtual void OnUpdate() override;
    virtual void OnRender() override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

private:
    void BuildDescriptorHeaps();
    void BuildConstantBuffers();
    void BuildRootSignature();
    void BuildBoxGeometry();
    void BuildPSO();

private:
    std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;

    XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
    XMFLOAT4X4 mView = MathHelper::Identity4x4();
    XMFLOAT4X4 mProj = MathHelper::Identity4x4();

private:
    POINT mLastMousePos;
    float mTheta = 1.5f * XM_PI;
    float mPhi = XM_PIDIV4;
    float mRadius = 5.0f;
};

