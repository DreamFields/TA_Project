// Copyright Epic Games, Inc. All Rights Reserved.

#include "BpPluginTestBPLibrary.h"
#include "BpPluginTest.h"

UBpPluginTestBPLibrary::UBpPluginTestBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

float UBpPluginTestBPLibrary::BpPluginTestSampleFunction(float Param)
{
	return -1;
}

