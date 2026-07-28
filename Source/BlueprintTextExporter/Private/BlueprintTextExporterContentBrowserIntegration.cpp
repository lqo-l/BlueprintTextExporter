// Copyright Epic Games, Inc. All Rights Reserved.

#include "BlueprintTextExporterContentBrowserIntegration.h"

#include "BlueprintTextExportService.h"
#include "MaterialTextExportService.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Engine/Blueprint.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/PlatformProcess.h"
#include "Materials/Material.h"
#include "Materials/MaterialFunctionInterface.h"
#include "Materials/MaterialInterface.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Styling/AppStyle.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"

#define LOCTEXT_NAMESPACE "BlueprintTextExporterContentBrowserIntegration"

DEFINE_LOG_CATEGORY_STATIC(LogBlueprintTextExporterMenu, Log, All);

namespace
{
	void ShowConsoleWarning(const FText& InMessage)
	{
		FNotificationInfo Info(InMessage);
		Info.ExpireDuration = 5.0f;
		Info.bUseSuccessFailIcons = true;

		if (TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info))
		{
			Notification->SetCompletionState(SNotificationItem::CS_Fail);
		}
	}

	void OpenExportFolder(const FString InExportFilePath)
	{
		const FString ExportFolder = FPaths::GetPath(InExportFilePath);
		if (!ExportFolder.IsEmpty())
		{
			FPlatformProcess::ExploreFolder(*ExportFolder);
		}
	}
}

FDelegateHandle FBlueprintTextExporterContentBrowserIntegration::ContentBrowserAssetHandle;

void FBlueprintTextExporterContentBrowserIntegration::Integrate()
{
	Disintegrate();

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
	CBMenuExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedAssets::CreateStatic(&FBlueprintTextExporterContentBrowserIntegration::OnExtendContentBrowserAssetSelectionMenu));
	ContentBrowserAssetHandle = CBMenuExtenderDelegates.Last().GetHandle();
}

void FBlueprintTextExporterContentBrowserIntegration::Disintegrate()
{
	if (!ContentBrowserAssetHandle.IsValid())
	{
		return;
	}

	if (FContentBrowserModule* ContentBrowserModule = FModuleManager::GetModulePtr<FContentBrowserModule>("ContentBrowser"))
	{
		TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = ContentBrowserModule->GetAllAssetViewContextMenuExtenders();
		CBMenuExtenderDelegates.RemoveAll(
			[](const FContentBrowserMenuExtender_SelectedAssets& InDelegate)
			{
				return InDelegate.GetHandle() == ContentBrowserAssetHandle;
			});
	}

	ContentBrowserAssetHandle.Reset();
}

bool FBlueprintTextExporterContentBrowserIntegration::CanExportBlueprints(const TArray<FAssetData>& InSelectedAssets)
{
	for (const FAssetData& AssetData : InSelectedAssets)
	{
		if (const UClass* AssetClass = AssetData.GetClass(EResolveClass::Yes))
		{
			if (AssetClass->IsChildOf(UBlueprint::StaticClass()))
			{
				return true;
			}
		}
	}

	return false;
}

bool FBlueprintTextExporterContentBrowserIntegration::CanExportMaterials(const TArray<FAssetData>& InSelectedAssets)
{
	for (const FAssetData& AssetData : InSelectedAssets)
	{
		if (const UClass* AssetClass = AssetData.GetClass(EResolveClass::Yes))
		{
			if (AssetClass->IsChildOf(UMaterialInterface::StaticClass()) || AssetClass->IsChildOf(UMaterialFunctionInterface::StaticClass()))
			{
				return true;
			}
		}
	}

	return false;
}

void FBlueprintTextExporterContentBrowserIntegration::ExportSelectedAssetsFromConsole()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> SelectedAssets;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);
	if (SelectedAssets.IsEmpty())
	{
		UE_LOG(LogBlueprintTextExporterMenu, Warning, TEXT("BlueprintTextExport.Export requires a Content Browser selection or one or more asset paths."));
		ShowConsoleWarning(LOCTEXT("ConsoleNoSelection", "Blueprint Text Export failed. Select supported assets or provide asset paths in Output Log."));
		return;
	}

	ExportAssets(MoveTemp(SelectedAssets), false);
}

void FBlueprintTextExporterContentBrowserIntegration::ExportAssetPathsFromConsole(const TArray<FString>& InAssetPaths)
{
	TArray<FAssetData> AssetsToExport;
	int32 FailureCount = 0;

	for (FString AssetPath : InAssetPaths)
	{
		AssetPath.TrimStartAndEndInline();
		if (FPaths::GetExtension(AssetPath).Equals(TEXT("uasset"), ESearchCase::IgnoreCase))
		{
			FString LongPackageName;
			if (!FPackageName::TryConvertFilenameToLongPackageName(AssetPath, LongPackageName))
			{
				UE_LOG(LogBlueprintTextExporterMenu, Warning, TEXT("Cannot convert file path '%s' to a package path."), *AssetPath);
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
			UE_LOG(LogBlueprintTextExporterMenu, Warning, TEXT("Cannot load asset '%s'."), *AssetPath);
			++FailureCount;
			continue;
		}

		AssetsToExport.Emplace(Asset);
	}

	if (!AssetsToExport.IsEmpty())
	{
		ExportAssets(MoveTemp(AssetsToExport), false);
	}

	if (FailureCount > 0)
	{
		ShowConsoleWarning(FText::Format(LOCTEXT("ConsolePathFailures", "Blueprint Text Export could not load {0} asset(s). See Output Log for details."), FText::AsNumber(FailureCount)));
	}
}

void FBlueprintTextExporterContentBrowserIntegration::ExportAssets(TArray<FAssetData> InSelectedAssets, bool bShowFailureDialog)
{
	TArray<FAssetData> BlueprintAssets;
	TArray<FAssetData> MaterialAssets;
	for (const FAssetData& AssetData : InSelectedAssets)
	{
		if (const UClass* AssetClass = AssetData.GetClass(EResolveClass::Yes))
		{
			if (AssetClass->IsChildOf(UBlueprint::StaticClass()))
			{
				BlueprintAssets.Add(AssetData);
			}
			else if (AssetClass->IsChildOf(UMaterialInterface::StaticClass()) || AssetClass->IsChildOf(UMaterialFunctionInterface::StaticClass()))
			{
				MaterialAssets.Add(AssetData);
			}
		}
	}

	if (BlueprintAssets.IsEmpty() && MaterialAssets.IsEmpty())
	{
		UE_LOG(LogBlueprintTextExporterMenu, Warning, TEXT("No supported Blueprint or material assets were provided for export."));
		ShowConsoleWarning(LOCTEXT("ConsoleUnsupportedAssets", "Blueprint Text Export failed. No supported assets were provided."));
		return;
	}

	if (!BlueprintAssets.IsEmpty())
	{
		ExportBlueprints(MoveTemp(BlueprintAssets), bShowFailureDialog);
	}

	if (!MaterialAssets.IsEmpty())
	{
		ExportMaterials(MoveTemp(MaterialAssets), bShowFailureDialog);
	}
}

void FBlueprintTextExporterContentBrowserIntegration::ExportBlueprints(TArray<FAssetData> InSelectedAssets, bool bShowFailureDialog)
{
	int32 SuccessCount = 0;
	int32 FailureCount = 0;
	TArray<FString> FailureMessages;
	FString FirstExportedTextPath;

	for (const FAssetData& AssetData : InSelectedAssets)
	{
		UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
		if (Blueprint == nullptr)
		{
			continue;
		}

		FString TextPath;
		FString JsonPath;
		FString ErrorMessage;
		if (FBlueprintTextExportService::ExportBlueprint(Blueprint, TextPath, JsonPath, ErrorMessage))
		{
			++SuccessCount;
			if (FirstExportedTextPath.IsEmpty())
			{
				FirstExportedTextPath = TextPath;
			}
			UE_LOG(LogBlueprintTextExporterMenu, Log, TEXT("Exported Blueprint '%s' to '%s' and '%s'."), *Blueprint->GetPathName(), *TextPath, *JsonPath);
		}
		else
		{
			++FailureCount;
			FailureMessages.Add(FString::Printf(TEXT("%s: %s"), *Blueprint->GetPathName(), *ErrorMessage));
			UE_LOG(LogBlueprintTextExporterMenu, Warning, TEXT("Failed to export Blueprint '%s': %s"), *Blueprint->GetPathName(), *ErrorMessage);
		}
	}

	FNotificationInfo Info(
		FText::Format(
			LOCTEXT("ExportResult", "Blueprint Text Export complete. Success: {0}, Failed: {1}"),
			FText::AsNumber(SuccessCount),
			FText::AsNumber(FailureCount)));
	Info.ExpireDuration = 5.0f;
	Info.bUseSuccessFailIcons = true;
	Info.SubText = SuccessCount > 0
		? LOCTEXT("ExportOutputLocation", "Files were saved under Saved/BlueprintExports.")
		: LOCTEXT("ExportOutputLocationFailed", "See Output Log for failure details.");
	if (SuccessCount > 0 && !FirstExportedTextPath.IsEmpty())
	{
		Info.Hyperlink = FSimpleDelegate::CreateLambda([FirstExportedTextPath]() { OpenExportFolder(FirstExportedTextPath); });
		Info.HyperlinkText = LOCTEXT("OpenBlueprintExportFolder", "Open Folder");
	}

	TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
	if (Notification.IsValid())
	{
		Notification->SetCompletionState(FailureCount == 0 ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
	}

	if (bShowFailureDialog && FailureMessages.Num() > 0)
	{
		const FText FailureText = FText::FromString(FString::Join(FailureMessages, TEXT("\n")));
		FMessageDialog::Open(EAppMsgType::Ok, FailureText);
	}
}

void FBlueprintTextExporterContentBrowserIntegration::ExportMaterials(TArray<FAssetData> InSelectedAssets, bool bShowFailureDialog)
{
	int32 SuccessCount = 0;
	int32 FailureCount = 0;
	TArray<FString> FailureMessages;
	FString FirstExportedTextPath;

	for (const FAssetData& AssetData : InSelectedAssets)
	{
		UObject* AssetObject = AssetData.GetAsset();
		if (AssetObject == nullptr)
		{
			continue;
		}

		FString TextPath;
		FString JsonPath;
		FString ErrorMessage;
		if (FMaterialTextExportService::ExportMaterialAsset(AssetObject, TextPath, JsonPath, ErrorMessage))
		{
			++SuccessCount;
			if (FirstExportedTextPath.IsEmpty())
			{
				FirstExportedTextPath = TextPath;
			}
			UE_LOG(LogBlueprintTextExporterMenu, Log, TEXT("Exported Material asset '%s' to '%s' and '%s'."), *AssetObject->GetPathName(), *TextPath, *JsonPath);
		}
		else
		{
			++FailureCount;
			FailureMessages.Add(FString::Printf(TEXT("%s: %s"), *AssetObject->GetPathName(), *ErrorMessage));
			UE_LOG(LogBlueprintTextExporterMenu, Warning, TEXT("Failed to export Material asset '%s': %s"), *AssetObject->GetPathName(), *ErrorMessage);
		}
	}

	FNotificationInfo Info(
		FText::Format(
			LOCTEXT("MaterialExportResult", "Material Text Export complete. Success: {0}, Failed: {1}"),
			FText::AsNumber(SuccessCount),
			FText::AsNumber(FailureCount)));
	Info.ExpireDuration = 5.0f;
	Info.bUseSuccessFailIcons = true;
	Info.SubText = SuccessCount > 0
		? LOCTEXT("MaterialExportOutputLocation", "Files were saved under Saved/MaterialExports.")
		: LOCTEXT("MaterialExportOutputLocationFailed", "See Output Log for failure details.");
	if (SuccessCount > 0 && !FirstExportedTextPath.IsEmpty())
	{
		Info.Hyperlink = FSimpleDelegate::CreateLambda([FirstExportedTextPath]() { OpenExportFolder(FirstExportedTextPath); });
		Info.HyperlinkText = LOCTEXT("OpenMaterialExportFolder", "Open Folder");
	}

	TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
	if (Notification.IsValid())
	{
		Notification->SetCompletionState(FailureCount == 0 ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
	}

	if (bShowFailureDialog && FailureMessages.Num() > 0)
	{
		const FText FailureText = FText::FromString(FString::Join(FailureMessages, TEXT("\n")));
		FMessageDialog::Open(EAppMsgType::Ok, FailureText);
	}
}

TSharedRef<FExtender> FBlueprintTextExporterContentBrowserIntegration::OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& InSelectedAssets)
{
	TSharedRef<FExtender> Extender = MakeShared<FExtender>();

	Extender->AddMenuExtension(
		"GetAssetActions",
		EExtensionHook::After,
		nullptr,
		FMenuExtensionDelegate::CreateLambda(
			[InSelectedAssets](FMenuBuilder& InMenuBuilder)
			{
				if (CanExportBlueprints(InSelectedAssets))
				{
					InMenuBuilder.AddMenuEntry(
						LOCTEXT("ExportBlueprintText", "Export Blueprint Text + JSON"),
						LOCTEXT("ExportBlueprintTextTooltip", "Exports the selected Blueprint execution flow as readable text and structured JSON."),
						FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Export"),
						FUIAction(FExecuteAction::CreateStatic(&FBlueprintTextExporterContentBrowserIntegration::ExportAssets, InSelectedAssets, true))
					);
				}

				if (CanExportMaterials(InSelectedAssets))
				{
					InMenuBuilder.AddMenuEntry(
						LOCTEXT("ExportMaterialText", "Export Material Text + JSON"),
						LOCTEXT("ExportMaterialTextTooltip", "Exports the selected material, material instance, or material function graph as readable text and structured JSON."),
						FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Export"),
						FUIAction(FExecuteAction::CreateStatic(&FBlueprintTextExporterContentBrowserIntegration::ExportAssets, InSelectedAssets, true))
					);
				}
			}));

	return Extender;
}

#undef LOCTEXT_NAMESPACE
