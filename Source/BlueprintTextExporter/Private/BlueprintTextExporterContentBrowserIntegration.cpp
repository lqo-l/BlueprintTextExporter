// Copyright Epic Games, Inc. All Rights Reserved.

#include "BlueprintTextExporterContentBrowserIntegration.h"

#include "BlueprintTextExportService.h"
#include "MaterialTextExportService.h"
#include "ContentBrowserModule.h"
#include "Engine/Blueprint.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Materials/Material.h"
#include "Materials/MaterialFunctionInterface.h"
#include "Materials/MaterialInterface.h"
#include "Misc/MessageDialog.h"
#include "Styling/AppStyle.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"

#define LOCTEXT_NAMESPACE "BlueprintTextExporterContentBrowserIntegration"

DEFINE_LOG_CATEGORY_STATIC(LogBlueprintTextExporterMenu, Log, All);

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

void FBlueprintTextExporterContentBrowserIntegration::ExportBlueprints(TArray<FAssetData> InSelectedAssets)
{
	int32 SuccessCount = 0;
	int32 FailureCount = 0;
	TArray<FString> FailureMessages;

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

	TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
	if (Notification.IsValid())
	{
		Notification->SetCompletionState(FailureCount == 0 ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
	}

	if (FailureMessages.Num() > 0)
	{
		const FText FailureText = FText::FromString(FString::Join(FailureMessages, TEXT("\n")));
		FMessageDialog::Open(EAppMsgType::Ok, FailureText);
	}
}

void FBlueprintTextExporterContentBrowserIntegration::ExportMaterials(TArray<FAssetData> InSelectedAssets)
{
	int32 SuccessCount = 0;
	int32 FailureCount = 0;
	TArray<FString> FailureMessages;

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

	TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
	if (Notification.IsValid())
	{
		Notification->SetCompletionState(FailureCount == 0 ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
	}

	if (FailureMessages.Num() > 0)
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
						FUIAction(FExecuteAction::CreateStatic(&FBlueprintTextExporterContentBrowserIntegration::ExportBlueprints, InSelectedAssets))
					);
				}

				if (CanExportMaterials(InSelectedAssets))
				{
					InMenuBuilder.AddMenuEntry(
						LOCTEXT("ExportMaterialText", "Export Material Text + JSON"),
						LOCTEXT("ExportMaterialTextTooltip", "Exports the selected material, material instance, or material function graph as readable text and structured JSON."),
						FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Export"),
						FUIAction(FExecuteAction::CreateStatic(&FBlueprintTextExporterContentBrowserIntegration::ExportMaterials, InSelectedAssets))
					);
				}
			}));

	return Extender;
}

#undef LOCTEXT_NAMESPACE
