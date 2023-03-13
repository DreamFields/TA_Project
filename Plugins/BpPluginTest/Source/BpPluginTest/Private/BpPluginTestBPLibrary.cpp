// Copyright Epic Games, Inc. All Rights Reserved.

#include "BpPluginTestBPLibrary.h"


#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GlobalShader.h"
#include "PipelineStateCache.h"
#include "RHIStaticStates.h"
#include "SceneUtils.h"
#include "SceneInterface.h"
#include "ShaderParameterUtils.h"
#include "Logging/MessageLog.h"
#include "Internationalization/Internationalization.h"
#include "ShaderCompilerCore.h"
#include "StaticBoundShaderState.h"  

#include "RHICommandList.h"  
#include "UniformBuffer.h"  

static const uint32 kGridSubdivisionX = 32;
static const uint32 kGridSubdivisionY = 16;

#define LOCTEXT_NAMESPACE "BpPluginTest"


class FMyGlobalShader: public FGlobalShader
{
    DECLARE_INLINE_TYPE_LAYOUT(FMyGlobalShader, NonVirtual);
public:

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }

    static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    {
        FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
        OutEnvironment.SetDefine(TEXT("GRID_SUBDIVISION_X"), kGridSubdivisionX);
        OutEnvironment.SetDefine(TEXT("GRID_SUBDIVISION_Y"), kGridSubdivisionY);
    }

    FMyGlobalShader() {}

    FMyGlobalShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FGlobalShader(Initializer)
    {
        SimpleColorVal.Bind(Initializer.ParameterMap, TEXT("SimpleColor"));

        // 在构造函数把新的成员变量和shader里的变量绑定
        TestTextureVal.Bind(Initializer.ParameterMap, TEXT("MyTexture"));  
        TestTextureSampler.Bind(Initializer.ParameterMap, TEXT("MyTextureSampler"));

    }

    template<typename TShaderRHIParamRef>
    void SetParameters(
        FRHICommandListImmediate& RHICmdList,
        const TShaderRHIParamRef ShaderRHI,
        const FLinearColor& MyColor,
        // 添加变量
        const FTexture* MyTexture
        )
    {
        SetShaderValue(RHICmdList, ShaderRHI, SimpleColorVal, MyColor);
        /*SetTextureParameter(
            RHICmdList,
            ShaderRHI,
            TestTextureVal,
            TestTextureSampler,
            TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(),
            MyTexture);*/
        SetTextureParameter(RHICmdList, ShaderRHI, TestTextureVal, TestTextureSampler, MyTexture);
    }

    //virtual bool Serialize(FArchive& Ar)
    //{
    //    bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
    //    Ar << SimpleColorVal;
    //    return bShaderHasOutdatedParameters;
    //}

private:
    LAYOUT_FIELD(FShaderParameter, SimpleColorVal);
    // 加入新的成员变量
    LAYOUT_FIELD(FShaderResourceParameter, TestTextureVal);
    LAYOUT_FIELD(FShaderResourceParameter, TestTextureSampler);
};


class FMyGlobalShaderVS: public FMyGlobalShader
{
    DECLARE_SHADER_TYPE(FMyGlobalShaderVS, Global);
public:

    /** Default constructor. */
    FMyGlobalShaderVS() {}

    /** Initialization constructor. */
    FMyGlobalShaderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FMyGlobalShader(Initializer)
    {
    }
};


class FMyGlobalShaderPS: public FMyGlobalShader
{
    DECLARE_SHADER_TYPE(FMyGlobalShaderPS, Global);
public:

    /** Default constructor. */
    FMyGlobalShaderPS() {}

    /** Initialization constructor. */
    FMyGlobalShaderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FMyGlobalShader(Initializer)
    { }
};


IMPLEMENT_SHADER_TYPE(, FMyGlobalShaderVS, TEXT("/Plugin/BpPluginTest/Private/MyShader.usf"), TEXT("MainVS"), SF_Vertex)
IMPLEMENT_SHADER_TYPE(, FMyGlobalShaderPS, TEXT("/Plugin/BpPluginTest/Private/MyShader.usf"), TEXT("MainPS"), SF_Pixel)


//struct FTextureVertex
//{
//public:
//    FVector4 Position;
//    FVector2D UV;
//};

TGlobalResource<FTextureVertexDeclaration> GTextureVertexDeclaration;
TGlobalResource<FRectangleVertexBuffer> GRectangleVertexBuffer;
TGlobalResource<FRectangleIndexBuffer> GRectangleIndexBuffer;
static void DrawTestShaderRenderTarget_RenderThread(
    FRHICommandListImmediate& RHICmdList,
    FTextureRenderTargetResource* OutTextureRenderTargetResource,
    ERHIFeatureLevel::Type FeatureLevel,
    const FName& TextureRenderTargetName,
    FLinearColor MyColor,
    // 添加变量
    FTexture* MyTexture
)
{
    check(IsInRenderingThread());

#if WANTS_DRAW_MESH_EVENTS
    FString EventName;
    TextureRenderTargetName.ToString(EventName);
    SCOPED_DRAW_EVENTF(RHICmdList, SceneCapture, TEXT("DrawTestShaderRenderTarget_RenderThread %s"), *EventName);
#else
    SCOPED_DRAW_EVENT(RHICmdList, DrawTestShaderRenderTarget_RenderThread);
#endif

    FRHITexture2D* RenderTargetTexture = OutTextureRenderTargetResource->GetRenderTargetTexture();

    RHICmdList.Transition(FRHITransitionInfo(RenderTargetTexture, ERHIAccess::SRVMask, ERHIAccess::RTV));

    FRHIRenderPassInfo RPInfo(RenderTargetTexture, ERenderTargetActions::DontLoad_Store);
    RHICmdList.BeginRenderPass(RPInfo, TEXT("DrawTestShader"));
    {
        // 获取着色器。
        FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
        TShaderMapRef< FMyGlobalShaderVS > VertexShader(GlobalShaderMap);
        TShaderMapRef< FMyGlobalShaderPS > PixelShader(GlobalShaderMap);

        // FVertexDeclarationElementList Elements;
        // const auto Stride = sizeof(FTextureVertex);
        // Elements.Add(FVertexElement(0, STRUCT_OFFSET(FTextureVertex, Position), VET_Float4, 0, Stride));
        // Elements.Add(FVertexElement(0, STRUCT_OFFSET(FTextureVertex, UV), VET_Float2, 1, Stride));
        // FVertexDeclarationRHIRef VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
        FTextureVertexDeclaration VertexDeclaration;
        VertexDeclaration.InitRHI();


        // 设置图像管线状态。
        FGraphicsPipelineStateInitializer GraphicsPSOInit;
        RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
        GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
        GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
        GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
        GraphicsPSOInit.PrimitiveType = PT_TriangleList;
        //GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetVertexDeclarationFVector4();
        GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = VertexDeclaration.VertexDeclarationRHI;
        GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
        GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
        SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

        // 更新着色器的统一参数。
        // 新添加的变量在这里设置
        PixelShader->SetParameters(RHICmdList, PixelShader.GetPixelShader(), MyColor,MyTexture);

        RHICmdList.SetStreamSource(0, GRectangleVertexBuffer.VertexBufferRHI, 0);
        RHICmdList.DrawIndexedPrimitive(
            GRectangleIndexBuffer.IndexBufferRHI,
            /*BaseVertexIndex=*/ 0,
            /*MinIndex=*/ 0,
            /*NumVertices=*/ 4,
            /*StartIndex=*/ 0,
            /*NumPrimitives=*/ 2,
            /*NumInstances=*/ 1);
        // -------------------------绘制网格-------------------
        // 创建静态顶点缓冲
        // FRHIResourceCreateInfo CreateInfo = FRHIResourceCreateInfo(TEXT("VolumeForVoxelize"));;
        // FVertexBufferRHIRef VertexBufferRHI = RHICreateVertexBuffer(sizeof(FTextureVertex) * 4, BUF_Volatile, CreateInfo);
        // void* VoidPtr = RHILockVertexBuffer(VertexBufferRHI, 0, sizeof(FTextureVertex) * 4, RLM_WriteOnly);

        // FTextureVertex* Vertices = (FTextureVertex*)VoidPtr;
        // Vertices[0].Position = FVector4(-1.0f, 1.0f, 0, 1.0f);
        // Vertices[1].Position = FVector4(1.0f, 1.0f, 0, 1.0f);
        // Vertices[2].Position = FVector4(-1.0f, -1.0f, 0, 1.0f);
        // Vertices[3].Position = FVector4(1.0f, -1.0f, 0, 1.0f);
        // Vertices[0].UV = FVector2D(0, 0);
        // Vertices[1].UV = FVector2D(1, 0);
        // Vertices[2].UV = FVector2D(0, 1);
        // Vertices[3].UV = FVector2D(1, 1);
        // RHIUnlockVertexBuffer(VertexBufferRHI);

        // RHICmdList.SetStreamSource(0, VertexBufferRHI, 0);
        // RHICmdList.DrawPrimitive(0, 2, 1);
    }
    RHICmdList.EndRenderPass();

    //RHICmdList.Transition(FRHITransitionInfo(RenderTargetTexture, ERHIAccess::RTV, ERHIAccess::SRVMask));
}


UBpPluginTestBPLibrary::UBpPluginTestBPLibrary(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{

}


void UBpPluginTestBPLibrary::BpPluginTestSampleFunction(
    const UObject* WorldContextObject,
    UTextureRenderTarget2D* OutputRenderTarget,
    FLinearColor MyColor,
    // 添加变量
    UTexture2D* MyTexture)
{
    check(IsInGameThread());

    if (!OutputRenderTarget)
    {
        FMessageLog("Blueprint").Warning(LOCTEXT("BpPluginTestSampleFunction_OutputTargetRequired", "BpPluginTestSampleFunctionToRenderTarget: Output render target is required."));
        return;
    }

    const FName TextureRenderTargetName = OutputRenderTarget->GetFName();
    FTextureRenderTargetResource* TextureRenderTargetResource = OutputRenderTarget->GameThread_GetRenderTargetResource();
    
    // UWorld* World = Ac->GetWorld();
    UWorld* World = WorldContextObject->GetWorld();
    ERHIFeatureLevel::Type FeatureLevel = World->Scene->GetFeatureLevel();
    // 添加变量
    FTexture* MyTextureRHI = MyTexture->Resource;

    ENQUEUE_RENDER_COMMAND(CaptureCommand)(
        [MyColor, MyTextureRHI, TextureRenderTargetResource, TextureRenderTargetName, FeatureLevel](FRHICommandListImmediate& RHICmdList)
        {
            DrawTestShaderRenderTarget_RenderThread(
                RHICmdList,
                TextureRenderTargetResource,
                FeatureLevel,
                TextureRenderTargetName,
                MyColor,
                // 添加变量
                MyTextureRHI
                );
        }
    );
}
//PRAGMA_ENABLE_DEPRECATION_WARNINGS
#undef LOCTEXT_NAMESPACE
