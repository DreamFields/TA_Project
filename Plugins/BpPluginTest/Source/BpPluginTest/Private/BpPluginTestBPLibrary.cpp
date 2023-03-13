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

//! 把下列代码放入.h文件中会报“重定义”的错误
//! UniformBuffer的声明方法每个引擎的版本都在变，其实如果发现声明方法变了我们可以去看引擎自己是怎么写的就好了
BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FMyUniformStructData, )
SHADER_PARAMETER(FVector4f, Color1)
SHADER_PARAMETER(FVector4f, Color2)
SHADER_PARAMETER(FVector4f, Color3)
SHADER_PARAMETER(FVector4f, Color4)
SHADER_PARAMETER(uint32, ColorIndex)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
// 在Shader中直接使用FMyUniform  
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FMyUniformStructData, "FMyUniform");

TGlobalResource<FTextureVertexDeclaration> GTextureVertexDeclaration;
TGlobalResource<FRectangleVertexBuffer> GRectangleVertexBuffer;
TGlobalResource<FRectangleIndexBuffer> GRectangleIndexBuffer;

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
        // 添加贴图
        FTexture2DRHIRef  MyTexture
    )
    {
        SetShaderValue(RHICmdList, ShaderRHI, SimpleColorVal, MyColor);
        // 此处设置传入的贴图
        SetTextureParameter(RHICmdList, ShaderRHI, TestTextureVal, TestTextureSampler, TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), MyTexture);
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

// 注意，计算着色器直接继承FGlobalShader
class FMyGlobalShaderCS: public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FMyGlobalShaderCS)

        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }

    static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    {
        FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
        OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
    }

public:
    FMyGlobalShaderCS() {}
    FMyGlobalShaderCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FGlobalShader(Initializer)
    {
        OutTexture.Bind(Initializer.ParameterMap, TEXT("OutTexture"));
    }

    //这里使用了FUnorderedAccessViewRHIParamRef（UAV）类型的形参，用于设定RenderTarget(UAC)。
    void SetParameters(FRHICommandList& RHICmdList, FRHIUnorderedAccessView* InOutUAV, FMyShaderStructData UniformStruct)
    {
        FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();
        if (OutTexture.IsBound())
            RHICmdList.SetUAVParameter(ComputeShaderRHI, OutTexture.GetBaseIndex(), InOutUAV);

        FMyUniformStructData ShaderStructData;
        ShaderStructData.Color1 = UniformStruct.Color1;
        ShaderStructData.Color2 = UniformStruct.Color2;
        ShaderStructData.Color3 = UniformStruct.Color3;
        ShaderStructData.Color4 = UniformStruct.Color4;
        ShaderStructData.ColorIndex = UniformStruct.ColorIndex;

        SetUniformBufferParameterImmediate(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<FMyUniformStructData>(), ShaderStructData);
    }

    //将UAV设置为空
    void UnbindBuffers(FRHICommandList& RHICmdList)
    {
        FRHIComputeShader* ComputeShaderRHI = RHICmdList.GetBoundComputeShader();
        if (OutTexture.IsBound())
            RHICmdList.SetUAVParameter(ComputeShaderRHI, OutTexture.GetBaseIndex(), nullptr);
    }
private:
    //用于进行数据交换的RenderTarget(UAC)变量
    LAYOUT_FIELD(FShaderResourceParameter, OutTexture);
};


IMPLEMENT_SHADER_TYPE(, FMyGlobalShaderVS, TEXT("/Plugin/BpPluginTest/Private/MyShader.usf"), TEXT("MainVS"), SF_Vertex)
IMPLEMENT_SHADER_TYPE(, FMyGlobalShaderPS, TEXT("/Plugin/BpPluginTest/Private/MyShader.usf"), TEXT("MainPS"), SF_Pixel)
// 计算着色器的实现
IMPLEMENT_SHADER_TYPE(, FMyGlobalShaderCS, TEXT("/Plugin/BpPluginTest/Private/MyShader.usf"), TEXT("MainCS"), SF_Compute)





// 顶点、像素着色器流程
static void UseGlobalShaderDraw_RenderThread(
    FRHICommandListImmediate& RHICmdList,
    FTexture2DRHIRef RenderTargetRHI,
    ERHIFeatureLevel::Type FeatureLevel,
    const FName& TextureRenderTargetName,
    FLinearColor MyColor,
    // 添加贴图
    FTexture2DRHIRef  MyTexture,
    // 添加Uniform buffer
    FMyShaderStructData MyParameter
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

    //FRHITexture2D* RenderTargetTexture = OutTextureRenderTargetResource->GetRenderTargetTexture();

    RHICmdList.Transition(FRHITransitionInfo(RenderTargetRHI, ERHIAccess::SRVMask, ERHIAccess::RTV));

    FRHIRenderPassInfo RPInfo(RenderTargetRHI, ERenderTargetActions::DontLoad_Store);
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
        FMyUniformStructData UniformData;
        UniformData.Color1 = MyParameter.Color1;
        UniformData.Color2 = MyParameter.Color2;
        UniformData.Color3 = MyParameter.Color3;
        UniformData.Color4 = MyParameter.Color4;
        UniformData.ColorIndex = MyParameter.ColorIndex;
        // 新添加的变量在这里设置
        //SetUniformBufferParameterImmediate(RHICmdList, PixelShader.GetPixelShader(), PixelShader->GetUniformBufferParameter<FMyUniformStructData>(), UniformData);
        SetUniformBufferParameterImmediate(RHICmdList, PixelShader.GetPixelShader(), PixelShader->GetUniformBufferParameter<FMyUniformStructData>(), UniformData);
        PixelShader->SetParameters(RHICmdList, PixelShader.GetPixelShader(), MyColor, MyTexture);

        RHICmdList.SetStreamSource(0, GRectangleVertexBuffer.VertexBufferRHI, 0);
        RHICmdList.DrawIndexedPrimitive(
            GRectangleIndexBuffer.IndexBufferRHI,
            /*BaseVertexIndex=*/ 0,
            /*MinIndex=*/ 0,
            /*NumVertices=*/ 4,
            /*StartIndex=*/ 0,
            /*NumPrimitives=*/ 2,
            /*NumInstances=*/ 1);

    }
    RHICmdList.EndRenderPass();

    //RHICmdList.Transition(FRHITransitionInfo(RenderTargetTexture, ERHIAccess::RTV, ERHIAccess::SRVMask));
}


// 计算着色器流程
//void GlobalShaderCompute_RenderThread(
//    FRHICommandListImmediate& RHICmdList,
//    FTextureRenderTargetResource* OutputRenderTargetResource,
//    ERHIFeatureLevel::Type FeatureLevel,
//    const FName& TextureRenderTargetName,
//    FMyShaderStructData MyParameter)
//{
//    check(IsInRenderingThread());
//
//    //为RHICmdList设置ComputeShader
//    FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
//    TShaderMapRef<FMyGlobalShaderCS> ComputeShader(GlobalShaderMap);
//    //RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());
//    SetComputePipelineState(RHICmdList, ComputeShader.GetComputeShader());
//
//    int32 SizeX = OutputRenderTargetResource->GetSizeX();
//    int32 SizeY = OutputRenderTargetResource->GetSizeY();
//    FRHIResourceCreateInfo CreateInfo(TEXT("GlobalShader_ComputeShader_UAV"));
//
//    //设置RenderTarget格式与属性，并以此创建UAC
//    FTexture* Texture = RHICreateTexture2D(SizeX, SizeY, PF_A32B32G32R32F, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
//    FUnorderedAccessViewRHIRef TextureUAV = RHICreateUnorderedAccessView(Texture);
//
//    //通过自定义函数设置ComputeShader变量=》分发计算任务进行并行运行=》计算完成后解除绑定
//    ComputeShader->SetParameters(RHICmdList, TextureUAV, MyParameter);
//    DispatchComputeShader(RHICmdList, ComputeShader, SizeX / 32, SizeY / 32, 1);
//    ComputeShader->UnbindBuffers(RHICmdList);
//
//
//    //通过PixelShader的渲染函数将计算结果渲染出来
//    UseGlobalShaderDraw_RenderThread(
//        RHICmdList,
//        OutputRenderTargetResource,
//        FeatureLevel,
//        TextureRenderTargetName,
//        FLinearColor(),
//        Texture,
//        MyParameter);
//}

UBpPluginTestBPLibrary::UBpPluginTestBPLibrary(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{

}

void UBpPluginTestBPLibrary::UseGlobalShaderDraw(
    const UObject* WorldContextObject,
    UTextureRenderTarget2D* OutputRenderTarget,
    FLinearColor MyColor,
    // 添加贴图
    UTexture2D* MyTexture,
    FMyShaderStructData MyParameter)
{
    check(IsInGameThread());

    if (!OutputRenderTarget)
    {
        FMessageLog("Blueprint").Warning(LOCTEXT("UseGlobalShaderDraw_OutputTargetRequired", "BpPluginTestSampleFunctionToRenderTarget: Output render target is required."));
        return;
    }

    

    //const FName TextureRenderTargetName = OutputRenderTarget->GetFName();
    //FTextureRenderTargetResource* TextureRenderTargetResource = OutputRenderTarget->GameThread_GetRenderTargetResource();

    //// UWorld* World = Ac->GetWorld();
    //UWorld* World = WorldContextObject->GetWorld();
    //ERHIFeatureLevel::Type FeatureLevel = World->Scene->GetFeatureLevel();
    //// 添加贴图
    //// FTexture2DRHIRef  MyTextureRHI = MyTexture->GetResource()->TextureRHI->GetTexture2D();
    //FTexture* MyTextureRHI = MyTexture->Resource;

    FTexture2DRHIRef TextureRenderTargetResource = OutputRenderTarget->GameThread_GetRenderTargetResource()->GetRenderTargetTexture();
    FTexture2DRHIRef MyTextureRHI = MyTexture->GetResource()->TextureRHI->GetTexture2D();
    const FName TextureRenderTargetName = OutputRenderTarget->GetFName();
    UWorld* World = WorldContextObject->GetWorld();
    ERHIFeatureLevel::Type FeatureLevel = World->Scene->GetFeatureLevel();

    ENQUEUE_RENDER_COMMAND(CaptureCommand)(
        [MyColor, MyTextureRHI, MyParameter, TextureRenderTargetResource, TextureRenderTargetName, FeatureLevel](FRHICommandListImmediate& RHICmdList)
        {
            UseGlobalShaderDraw_RenderThread(
                RHICmdList,
                TextureRenderTargetResource,
                FeatureLevel,
                TextureRenderTargetName,
                MyColor,
                // 添加贴图
                MyTextureRHI,
                // 添加uniform buffer
                MyParameter
            );
        }
    );
}

//void UBpPluginTestBPLibrary::UseGlobalShaderCompute(
//    const UObject* WorldContextObject,
//    UTextureRenderTarget2D* OutputRenderTarget,
//    FLinearColor MyColor,
//    UTexture2D* MyTexture,
//    FMyShaderStructData MyParameter
//)
//{
//    check(IsInGameThread());
//    FTexture2DRHIRef RenderTargetRHI = OutputRenderTarget->GameThread_GetRenderTargetResource()->GetRenderTargetTexture();
//    const FName TextureRenderTargetName = OutputRenderTarget->GetFName();
//    FTextureRenderTargetResource* TextureRenderTargetResource = OutputRenderTarget->GameThread_GetRenderTargetResource();
//
//    UWorld* World = WorldContextObject->GetWorld();
//    ERHIFeatureLevel::Type FeatureLevel = World->Scene->GetFeatureLevel();
//    // 添加贴图
//    FTexture2DRHIRef  MyTextureRHI = MyTexture->GetResource()->TextureRHI->GetTexture2D();
//    ENQUEUE_RENDER_COMMAND(CaptureCommand)(
//        [RenderTargetRHI, MyParameter](FRHICommandListImmediate& RHICmdList) {
//            GlobalShaderCompute_RenderThread(
//                RHICmdList, 
//                TextureRenderTargetResource,
//                FeatureLevel,
//                TextureRenderTargetName,
//                MyParameter);
//        });
//}


//PRAGMA_ENABLE_DEPRECATION_WARNINGS
#undef LOCTEXT_NAMESPACE
