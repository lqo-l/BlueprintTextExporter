// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UObject;

class FMaterialTextExportService
{
public:
	static bool ExportMaterialAsset(UObject* InAsset, FString& OutTextPath, FString& OutJsonPath, FString& OutErrorMessage);
};
