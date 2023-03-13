/*
 * @Author: Meng Tian
 * @Date: 2023-03-10 15:51:09
 * @Descripttion: Do not edit
 */
 // Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "RenderGraph.h"

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ShaderParameterMacros.h"
#include "BpPluginTestBPLibrary.generated.h"

/*
*	Function library class.
*	Each function in it is expected to be static and represents blueprint node that can be called in any blueprint.
*
*	When declaring function you can define metadata for the node. Key function specifiers will be BlueprintPure and BlueprintCallable.
*	BlueprintPure - means the function does not affect the owning object in any way and thus creates a node without Exec pins.
*	BlueprintCallable - makes a function which can be executed in Blueprints - Thus it has Exec pins.
*	DisplayName - full name of the node, shown when you mouse over the node and in the blueprint drop down menu.
*				Its lets you name the node using characters not allowed in C++ function names.
*	CompactNodeTitle - the word(s) that appear on the node.
*	Keywords -	the list of keywords that helps you to find node when you search for it using Blueprint drop-down menu.
*				Good example is "Print String" node which you can find also by using keyword "log".
*	Category -	the category your node will be under in the Blueprint drop-down menu.
*
*	For more info on custom blueprint nodes visit documentation:
*	https://wiki.unrealengine.com/Custom_Blueprint_Node_Creation
*/
// 声明一个结构体用来设置uniform buffer
USTRUCT(BlueprintType, meta = (ScriptName = "FMyShaderStructData"))
struct FMyShaderStructData
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (WorldContext = "WorldContextObject"))
		FLinearColor Color1;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (WorldContext = "WorldContextObject"))
		FLinearColor Color2;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (WorldContext = "WorldContextObject"))
		FLinearColor Color3;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (WorldContext = "WorldContextObject"))
		FLinearColor Color4;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (WorldContext = "WorldContextObject"))
		int32 ColorIndex;
};

UCLASS(MinimalAPI)
class UBpPluginTestBPLibrary: public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

		UFUNCTION(BlueprintCallable, meta = (DisplayName = "UseGlobalShaderDraw", WorldContext = "WorldContextObject", Keywords = "BpPluginTest sample test testing"), Category = "BpPluginTestTesting")
		static void UseGlobalShaderDraw(
			const UObject* WorldContextObject,
			UTextureRenderTarget2D* OutputRenderTarget,
			FLinearColor MyColor,
			UTexture2D* MyTexture,
			FMyShaderStructData MyParameter
		);

	//UFUNCTION(BlueprintCallable, meta = (DisplayName = "UseGlobalShaderDraw", WorldContext = "WorldContextObject", Keywords = "BpPluginTest sample test testing"), Category = "BpPluginTestTesting")
	//	static void UseGlobalShaderCompute(
	//		const UObject* WorldContextObject,
	//		UTextureRenderTarget2D* OutputRenderTarget,
	//		FLinearColor MyColor,
	//		UTexture2D* MyTexture,
	//		FMyShaderStructData MyParameter
	//	);
};



/*
 * 	Common Resource
 */
struct FTextureVertex
{
	FVector4f Position;
	FVector2f UV;
};

class FRectangleVertexBuffer: public FVertexBuffer
{
public:
	/** Initialize the RHI for this rendering resource */
	void InitRHI() override
	{
		TResourceArray<FTextureVertex, VERTEXBUFFER_ALIGNMENT> Vertices;
		Vertices.SetNumUninitialized(6);

		Vertices[0].Position = FVector4f(1, 1, 0, 1);
		Vertices[0].UV = FVector2f(1, 1);

		Vertices[1].Position = FVector4f(-1, 1, 0, 1);
		Vertices[1].UV = FVector2f(0, 1);

		Vertices[2].Position = FVector4f(1, -1, 0, 1);
		Vertices[2].UV = FVector2f(1, 0);

		Vertices[3].Position = FVector4f(-1, -1, 0, 1);
		Vertices[3].UV = FVector2f(0, 0);

		//The final two vertices are used for the triangle optimization (a single triangle spans the entire viewport )
		Vertices[4].Position = FVector4f(-1, 1, 0, 1);
		Vertices[4].UV = FVector2f(-1, 1);

		Vertices[5].Position = FVector4f(1, -1, 0, 1);
		Vertices[5].UV = FVector2f(1, -1);

		// Create vertex buffer. Fill buffer with initial data upon creation
		FRHIResourceCreateInfo CreateInfo(TEXT("FRectangleVertexBuffer"), &Vertices);
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.GetResourceDataSize(), BUF_Static, CreateInfo);
	}
};

class FRectangleIndexBuffer: public FIndexBuffer
{
public:
	/** Initialize the RHI for this rendering resource */
	void InitRHI() override
	{
		// Indices 0 - 5 are used for rendering a quad. Indices 6 - 8 are used for triangle optimization.
		const uint16 Indices[] = { 0, 1, 2, 2, 1, 3, 0, 4, 5 };

		TResourceArray<uint16, INDEXBUFFER_ALIGNMENT> IndexBuffer;
		uint32 NumIndices = UE_ARRAY_COUNT(Indices);
		IndexBuffer.AddUninitialized(NumIndices);
		FMemory::Memcpy(IndexBuffer.GetData(), Indices, NumIndices * sizeof(uint16));

		// Create index buffer. Fill buffer with initial data upon creation
		FRHIResourceCreateInfo CreateInfo(TEXT("FRectangleIndexBuffer"), &IndexBuffer);
		IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint16), IndexBuffer.GetResourceDataSize(), BUF_Static, CreateInfo);
	}
};

class FTextureVertexDeclaration: public FRenderResource
{
public:
	FVertexDeclarationRHIRef VertexDeclarationRHI;
	virtual void InitRHI() override
	{
		FVertexDeclarationElementList Elements;
		uint32 Stride = sizeof(FTextureVertex);
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FTextureVertex, Position), VET_Float2, 0, Stride));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(FTextureVertex, UV), VET_Float2, 1, Stride));
		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}
	virtual void ReleaseRHI() override
	{
		VertexDeclarationRHI.SafeRelease();
	}
};

