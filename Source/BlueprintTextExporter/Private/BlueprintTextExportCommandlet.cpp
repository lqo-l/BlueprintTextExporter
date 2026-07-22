// Copyright Epic Games, Inc. All Rights Reserved.

#include "BlueprintTextExportCommandlet.h"

#include "BlueprintTextExportService.h"
#include "MaterialTextExportService.h"
#include "Engine/Blueprint.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"

DEFINE_LOG_CATEGORY_STATIC(LogBlueprintTextExportCommandlet, Log, All);

UBlueprintTextExportCommandlet::UBlueprintTextExportCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UBlueprintTextExportCommandlet::Main(const FString& Params)
{
	FString AssetList;
	if (!FParse::Value(*Params, TEXT("Assets="), AssetList) || AssetList.IsEmpty())
	{
		UE_LOG(LogBlueprintTextExportCommandlet, Error, TEXT("Missing -Assets=<asset paths>. Example: -Assets=\"/Game/Foo/BP_Test,/Game/Foo/M_Test\""));
		return 1;
	}

	int32 SuccessCount = 0;
	int32 FailureCount = 0;
	TArray<FString> AssetPaths;
	AssetList.ParseIntoArray(AssetPaths, TEXT(","), true);

	for (FString AssetPath : AssetPaths)
	{
		AssetPath.TrimStartAndEndInline();
		if (FPaths::GetExtension(AssetPath).Equals(TEXT("uasset"), ESearchCase::IgnoreCase))
		{
			FString LongPackageName;
			if (!FPackageName::TryConvertFilenameToLongPackageName(AssetPath, LongPackageName))
			{
				UE_LOG(LogBlueprintTextExportCommandlet, Error, TEXT("Cannot convert file path '%s' to a package path."), *AssetPath);
				++FailureCount;
				continue;
			}
			AssetPath = LongPackageName;
		}

		if (FPackageName::IsValidLongPackageName(AssetPath))
		{
			AssetPath += TEXT(".") + FPackageName::GetLongPackageAssetName(AssetPath);
		}

		UObject* Asset = LoadObject<UObject>(nullptr, *AssetPath);
		if (Asset == nullptr)
		{
			UE_LOG(LogBlueprintTextExportCommandlet, Error, TEXT("Cannot load asset '%s'."), *AssetPath);
			++FailureCount;
			continue;
		}

		FString TextPath;
		FString JsonPath;
		FString ErrorMessage;
		const bool bExported = Asset->IsA<UBlueprint>()
			? FBlueprintTextExportService::ExportBlueprint(CastChecked<UBlueprint>(Asset), TextPath, JsonPath, ErrorMessage)
			: FMaterialTextExportService::ExportMaterialAsset(Asset, TextPath, JsonPath, ErrorMessage);
		if (!bExported)
		{
			UE_LOG(LogBlueprintTextExportCommandlet, Error, TEXT("Failed to export '%s': %s"), *Asset->GetPathName(), *ErrorMessage);
			++FailureCount;
			continue;
		}

		UE_LOG(LogBlueprintTextExportCommandlet, Display, TEXT("Exported '%s' to '%s' and '%s'."), *Asset->GetPathName(), *TextPath, *JsonPath);
		++SuccessCount;
	}

	UE_LOG(LogBlueprintTextExportCommandlet, Display, TEXT("BlueprintTextExport finished. Success: %d, Failed: %d."), SuccessCount, FailureCount);
	return FailureCount == 0 ? 0 : 1;
}
