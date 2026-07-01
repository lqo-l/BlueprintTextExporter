// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AssetRegistry/AssetData.h"

class FExtender;
class FMenuBuilder;

class FBlueprintTextExporterContentBrowserIntegration
{
public:
	static void Integrate();

	static void Disintegrate();

private:
	static FDelegateHandle ContentBrowserAssetHandle;

	static bool CanExportBlueprints(const TArray<FAssetData>& InSelectedAssets);

	static void ExportBlueprints(TArray<FAssetData> InSelectedAssets);

	static bool CanExportMaterials(const TArray<FAssetData>& InSelectedAssets);

	static void ExportMaterials(TArray<FAssetData> InSelectedAssets);

	static TSharedRef<FExtender> OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& InSelectedAssets);
};
