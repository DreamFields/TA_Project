// Copyright Epic Games, Inc. All Rights Reserved.

#include "OceanTest.h"
#include "Interfaces/IPluginManager.h"
#define LOCTEXT_NAMESPACE "FOceanTestModule"

void FOceanTestModule::StartupModule()
{

}

void FOceanTestModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FOceanTestModule, OceanTest)