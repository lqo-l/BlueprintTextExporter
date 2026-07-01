// Copyright Epic Games, Inc. All Rights Reserved.

#include "IBlueprintTextExporter.h"
#include "BlueprintTextExporterContentBrowserIntegration.h"

class FBlueprintTextExporterModule : public IBlueprintTextExporter
{
public:
	virtual void StartupModule() override
	{
		FBlueprintTextExporterContentBrowserIntegration::Integrate();
	}

	virtual void ShutdownModule() override
	{
		FBlueprintTextExporterContentBrowserIntegration::Disintegrate();
	}
};

IMPLEMENT_MODULE(FBlueprintTextExporterModule, BlueprintTextExporter)
