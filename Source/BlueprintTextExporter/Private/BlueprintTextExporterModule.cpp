// Copyright Epic Games, Inc. All Rights Reserved.

#include "IBlueprintTextExporter.h"
#include "BlueprintTextExporterContentBrowserIntegration.h"
#include "HAL/IConsoleManager.h"

class FBlueprintTextExporterModule : public IBlueprintTextExporter
{
public:
	virtual void StartupModule() override
	{
		FBlueprintTextExporterContentBrowserIntegration::Integrate();

		// Moon Add: Registers the active-Editor export entry without launching UnrealEditor-Cmd.
		ExportConsoleCommand = MakeUnique<FAutoConsoleCommand>(
			TEXT("BlueprintTextExport.Export"),
			TEXT("Exports selected Content Browser assets, or comma-separated asset paths. Example: BlueprintTextExport.Export /Game/Foo/BP_Test,/Game/Foo/M_Test"),
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FBlueprintTextExporterModule::ExportFromConsole));
	}

	virtual void ShutdownModule() override
	{
		// Moon Add: Unregister the Editor console command before the module unloads.
		ExportConsoleCommand.Reset();
		FBlueprintTextExporterContentBrowserIntegration::Disintegrate();
	}

private:
	void ExportFromConsole(const TArray<FString>& InArguments)
	{
		if (InArguments.IsEmpty())
		{
			FBlueprintTextExporterContentBrowserIntegration::ExportSelectedAssetsFromConsole();
			return;
		}

		TArray<FString> AssetPaths;
		FString::Join(InArguments, TEXT(",")).ParseIntoArray(AssetPaths, TEXT(","), true);
		FBlueprintTextExporterContentBrowserIntegration::ExportAssetPathsFromConsole(AssetPaths);
	}

private:
	TUniquePtr<FAutoConsoleCommand> ExportConsoleCommand;
};

IMPLEMENT_MODULE(FBlueprintTextExporterModule, BlueprintTextExporter)
