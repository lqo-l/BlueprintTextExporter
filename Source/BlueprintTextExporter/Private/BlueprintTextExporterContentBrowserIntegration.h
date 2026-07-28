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

	/** Exports assets selected in the active Content Browser without opening modal dialogs. */
	static void ExportSelectedAssetsFromConsole();

	/** Exports assets identified by long package names or absolute .uasset file paths without opening modal dialogs. */
	static void ExportAssetPathsFromConsole(const TArray<FString>& InAssetPaths);

private:
	static FDelegateHandle ContentBrowserAssetHandle;

	static bool CanExportBlueprints(const TArray<FAssetData>& InSelectedAssets);

	static void ExportBlueprints(TArray<FAssetData> InSelectedAssets, bool bShowFailureDialog = true);

	static bool CanExportMaterials(const TArray<FAssetData>& InSelectedAssets);

	static void ExportMaterials(TArray<FAssetData> InSelectedAssets, bool bShowFailureDialog = true);

	static void ExportAssets(TArray<FAssetData> InSelectedAssets, bool bShowFailureDialog);

	static TSharedRef<FExtender> OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& InSelectedAssets);
};
