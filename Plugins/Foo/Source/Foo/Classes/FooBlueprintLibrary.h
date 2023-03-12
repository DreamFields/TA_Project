// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FooAPI.h"
#include "FooBlueprintLibrary.generated.h"


// class UE_DEPRECATED(5.0, "The LensDistortion plugin is deprecated. Please update your project to use the features of the CameraCalibration plugin.") UFooBlueprintLibrary;
// UCLASS(MinimalAPI, meta=(DeprecationMessage = "The LensDistortion plugin is deprecated. Please update your project to use the features of the CameraCalibration plugin.", ScriptName="LensDistortionLibrary"))
UCLASS(MinimalAPI)
class UFooBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()


	//PRAGMA_DISABLE_DEPRECATION_WARNINGS
	/** Returns the overscan factor required for the undistort rendering to avoid unrendered distorted pixels. */
	UFUNCTION(BlueprintPure, Category = "Foo")
	static void GetUndistortOverscanFactor(
		const FFooCameraModel& CameraModel,
		float DistortedHorizontalFOV,
		float DistortedAspectRatio,
		float& UndistortOverscanFactor);
		
	/** Draws UV displacement map within the output render target.
	 * - Red & green channels hold the distortion displacement;
	 * - Blue & alpha channels hold the undistortion displacement.
	 * @param DistortedHorizontalFOV The desired horizontal FOV in the distorted render.
	 * @param DistortedAspectRatio The desired aspect ratio of the distorted render.
	 * @param UndistortOverscanFactor The factor of the overscan for the undistorted render.
	 * @param OutputRenderTarget The render target to draw to. Don't necessarily need to have same resolution or aspect ratio as distorted render.
	 * @param OutputMultiply The multiplication factor applied on the displacement.
	 * @param OutputAdd Value added to the multiplied displacement before storing into the output render target.
	 */
	UFUNCTION(BlueprintCallable, Category = "Foo", meta = (DisplayName = "DrawUVDisplacementToRenderTarget", WorldContext = "WorldContextObject"))
	static void DrawUVDisplacementToRenderTarget(
		const UObject* WorldContextObject,
		const FFooCameraModel& CameraModel,
		float DistortedHorizontalFOV,
		float DistortedAspectRatio,
		float UndistortOverscanFactor,
		class UTextureRenderTarget2D* OutputRenderTarget,
		float OutputMultiply = 0.5,
		float OutputAdd = 0.5
		);

	/* 如果A等于B（A == B）则返回true */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Equal (LensDistortionCameraModel)", CompactNodeTitle = "==", Keywords = "== equal"), Category = "Foo | Lens Distortion")
		static bool EqualEqual_CompareLensDistortionModels(
			const FFooCameraModel& A,
			const FFooCameraModel& B)
	{
		return A == B;
	}

	/* 如A不等于B（A != B），则返回true */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "NotEqual (LensDistortionCameraModel)", CompactNodeTitle = "!=", Keywords = "!= not equal"), Category = "Foo | Lens Distortion")
		static bool NotEqual_CompareLensDistortionModels(
			const FFooCameraModel& A,
			const FFooCameraModel& B)
	{
		return A != B;
	}
};
