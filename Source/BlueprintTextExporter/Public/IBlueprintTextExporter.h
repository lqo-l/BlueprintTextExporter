// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class IBlueprintTextExporter : public IModuleInterface
{
public:
	static inline IBlueprintTextExporter& Get()
	{
		return FModuleManager::LoadModuleChecked<IBlueprintTextExporter>("BlueprintTextExporter");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("BlueprintTextExporter");
	}
};
