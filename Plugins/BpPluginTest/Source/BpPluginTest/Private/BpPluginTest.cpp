// Copyright Epic Games, Inc. All Rights Reserved.

#include "BpPluginTest.h"
#include "Interfaces/IPluginManager.h"
#define LOCTEXT_NAMESPACE "FBpPluginTestModule"

void FBpPluginTestModule::StartupModule()
{
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("BpPluginTest"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/BpPluginTest"), PluginShaderDir);
}

void FBpPluginTestModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FBpPluginTestModule, BpPluginTest)