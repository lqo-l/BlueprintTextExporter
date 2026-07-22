// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Commandlets/Commandlet.h"

#include "BlueprintTextExportCommandlet.generated.h"

/** Exports Blueprint and material graph summaries without starting the full editor UI. */
UCLASS()
class UBlueprintTextExportCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UBlueprintTextExportCommandlet();

	virtual int32 Main(const FString& Params) override;
};
