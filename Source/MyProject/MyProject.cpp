// Fill out your copyright notice in the Description page of Project Settings.

#include "MyProject.h"
#include "Modules/ModuleManager.h"

void FMyProjectModule::StartupModule()
{
	/// @note ����һ�����ƶ�Ӧ��Shaders �ļ���
	FString ShaderDirectory = FPaths::Combine(FPaths::ProjectDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Project"), ShaderDirectory);

}

void FMyProjectModule::ShutdownModule()
{
}


IMPLEMENT_PRIMARY_GAME_MODULE(FMyProjectModule, MyProject, "MyProject" );
