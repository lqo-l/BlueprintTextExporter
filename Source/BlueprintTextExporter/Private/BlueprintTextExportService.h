// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UBlueprint;

class FBlueprintTextExportService
{
public:
	static bool ExportBlueprint(UBlueprint* InBlueprint, FString& OutTextPath, FString& OutJsonPath, FString& OutErrorMessage);
};
