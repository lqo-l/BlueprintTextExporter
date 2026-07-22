// Copyright Epic Games, Inc. All Rights Reserved.

#include "MaterialTextExportService.h"

#include "Dom/JsonObject.h"
#include "Engine/Font.h"
#include "Engine/Texture.h"
#include "HAL/FileManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionCustomOutput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionReroute.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialFunctionInstance.h"
#include "Materials/MaterialFunctionInterface.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY_STATIC(LogMaterialTextExporter, Log, All);

namespace UE::MaterialTextExporter
{
	struct FExpressionEdge;

	struct FExpressionNode
	{
		FString Id;
		FString Title;
		FString NodeClass;
		FString Details;
		TArray<FExpressionEdge> Inputs;
	};

	struct FExpressionEdge
	{
		FString Label;
		FString Literal;
		TSharedPtr<FExpressionNode> Target;
	};

	struct FRootChain
	{
		FString Label;
		TSharedPtr<FExpressionNode> RootNode;
		FString Literal;
	};

	struct FRootInput
	{
		FString Label;
		const FExpressionInput* Input = nullptr;
	};

	struct FDeclaredParameter
	{
		FString Name;
		FString Type;
		FString Value;
		FString Group;
		int32 SortPriority = 0;
		FString Description;
		FString SourceAssetPath;
	};

	struct FGraphSummary
	{
		FString AssetKind;
		FString AssetPath;
		FString AssetName;
		FString MaterialDomain;
		FString BlendMode;
		bool bTwoSided = false;
		FString ParentPath;
		TArray<FString> DependentFunctions;
		TArray<FString> Notes;
		TArray<FDeclaredParameter> DeclaredParameters;
		TArray<FRootChain> Roots;
	};

	struct FBuildContext
	{
		TSet<FString> VisitingKeys;
	};

	static FString NormalizeWhitespace(FString InValue)
	{
		InValue.ReplaceInline(TEXT("\r"), TEXT(" "));
		InValue.ReplaceInline(TEXT("\n"), TEXT(" "));
		InValue.TrimStartAndEndInline();
		return InValue;
	}

	static FString GetCaptionText(const UMaterialExpression* InExpression)
	{
		if (InExpression == nullptr)
		{
			return TEXT("<null>");
		}

		TArray<FString> Captions;
		InExpression->GetCaption(Captions);
		if (Captions.Num() > 0)
		{
			for (FString& Caption : Captions)
			{
				Caption = NormalizeWhitespace(Caption);
			}
			return FString::Join(Captions, TEXT(" / "));
		}

		return InExpression->GetClass()->GetName();
	}

	static FString GetExpressionKey(const UMaterialExpression* InExpression, int32 InOutputIndex)
	{
		if (InExpression == nullptr)
		{
			return TEXT("InvalidExpression");
		}

		// Engine note: some engine versions expose GetMaterialExpressionId() as non-const.
		FGuid ExpressionId = const_cast<UMaterialExpression*>(InExpression)->GetMaterialExpressionId();
		if (ExpressionId.IsValid())
		{
			return FString::Printf(TEXT("%s:%d"), *ExpressionId.ToString(EGuidFormats::DigitsWithHyphensLower), InOutputIndex);
		}

		return FString::Printf(TEXT("%s:%d"), *FMD5::HashAnsiString(*InExpression->GetPathName()), InOutputIndex);
	}

	static FString GetExpressionDetails(const UMaterialExpression* InExpression)
	{
		if (InExpression == nullptr)
		{
			return FString();
		}

		TArray<FString> Parts;

		if (InExpression->HasAParameterName())
		{
			const FName ParameterName = InExpression->GetParameterName();
			if (!ParameterName.IsNone())
			{
				Parts.Add(FString::Printf(TEXT("Parameter=%s"), *ParameterName.ToString()));
			}
		}

		if (UObject* ReferencedTexture = InExpression->GetReferencedTexture())
		{
			Parts.Add(FString::Printf(TEXT("Texture=%s"), *ReferencedTexture->GetPathName()));
		}

		if (const UMaterialExpressionMaterialFunctionCall* FunctionCall = Cast<UMaterialExpressionMaterialFunctionCall>(InExpression))
		{
			if (FunctionCall->MaterialFunction != nullptr)
			{
				Parts.Add(FString::Printf(TEXT("Function=%s"), *FunctionCall->MaterialFunction->GetPathName()));
			}
		}

		if (const UMaterialExpressionCustom* CustomExpression = Cast<UMaterialExpressionCustom>(InExpression))
		{
			if (!CustomExpression->Description.IsEmpty())
			{
				Parts.Add(FString::Printf(TEXT("Description=%s"), *NormalizeWhitespace(CustomExpression->Description)));
			}

			const FString Code = NormalizeWhitespace(CustomExpression->Code);
			if (!Code.IsEmpty())
			{
				const FString Snippet = Code.Len() > 80 ? Code.Left(80) + TEXT("...") : Code;
				Parts.Add(FString::Printf(TEXT("Code=%s"), *Snippet));
			}
		}

		if (!InExpression->Desc.IsEmpty())
		{
			Parts.Add(FString::Printf(TEXT("Note=%s"), *NormalizeWhitespace(InExpression->Desc)));
		}

		return FString::Join(Parts, TEXT(", "));
	}

	static FString GetInputDefaultLiteral(UMaterialExpression* InExpression, int32 InInputIndex)
	{
		if (InExpression == nullptr)
		{
			return FString();
		}

		return NormalizeWhitespace(InExpression->GetInputPinDefaultValue(InInputIndex));
	}

	static TSharedPtr<FExpressionNode> BuildExpressionNode(UMaterialExpression* InExpression, int32 InOutputIndex, FBuildContext& InContext);

	static TSharedPtr<FExpressionNode> BuildExpressionNodeFromInput(const FExpressionInput* InInput, const FString& InLabel, FBuildContext& InContext, FExpressionEdge& OutEdge)
	{
		OutEdge.Label = InLabel;
		if (InInput == nullptr)
		{
			return nullptr;
		}

		if (InInput->Expression != nullptr)
		{
			OutEdge.Target = BuildExpressionNode(InInput->Expression, InInput->OutputIndex, InContext);
		}

		return OutEdge.Target;
	}

	static TSharedPtr<FExpressionNode> BuildExpressionNode(UMaterialExpression* InExpression, int32 InOutputIndex, FBuildContext& InContext)
	{
		if (InExpression == nullptr)
		{
			return nullptr;
		}

		if (UMaterialExpressionReroute* Reroute = Cast<UMaterialExpressionReroute>(InExpression))
		{
			if (FExpressionInput* Input = Reroute->GetInput(0))
			{
				if (Input->Expression != nullptr)
				{
					return BuildExpressionNode(Input->Expression, Input->OutputIndex, InContext);
				}
			}
		}

		const FString Key = GetExpressionKey(InExpression, InOutputIndex);
		if (InContext.VisitingKeys.Contains(Key))
		{
			TSharedPtr<FExpressionNode> LoopNode = MakeShared<FExpressionNode>();
			LoopNode->Id = Key + TEXT(":loop");
			LoopNode->Title = FString::Printf(TEXT("[Recursive] %s"), *GetCaptionText(InExpression));
			LoopNode->NodeClass = InExpression->GetClass()->GetName();
			return LoopNode;
		}

		InContext.VisitingKeys.Add(Key);

		TSharedPtr<FExpressionNode> Node = MakeShared<FExpressionNode>();
		Node->Id = Key;
		Node->Title = GetCaptionText(InExpression);
		Node->NodeClass = InExpression->GetClass()->GetName();
		Node->Details = GetExpressionDetails(InExpression);

		for (FExpressionInputIterator It(InExpression); It; ++It)
		{
			FExpressionEdge& Edge = Node->Inputs.AddDefaulted_GetRef();
			Edge.Label = NormalizeWhitespace(InExpression->GetInputName(It.Index).ToString());
			if (Edge.Label.IsEmpty())
			{
				Edge.Label = FString::Printf(TEXT("Input%d"), It.Index);
			}

			if (It->Expression != nullptr)
			{
				Edge.Target = BuildExpressionNode(It->Expression, It->OutputIndex, InContext);
			}
			else
			{
				Edge.Literal = GetInputDefaultLiteral(InExpression, It.Index);
			}
		}

		if (UMaterialExpressionMaterialFunctionCall* FunctionCall = Cast<UMaterialExpressionMaterialFunctionCall>(InExpression))
		{
			if (FunctionCall->FunctionOutputs.IsValidIndex(InOutputIndex))
			{
				const FFunctionExpressionOutput& FunctionOutput = FunctionCall->FunctionOutputs[InOutputIndex];
				if (FunctionOutput.ExpressionOutput != nullptr)
				{
					FExpressionEdge& Edge = Node->Inputs.AddDefaulted_GetRef();
					Edge.Label = FString::Printf(TEXT("FunctionOutput:%s"), *FunctionOutput.ExpressionOutput->OutputName.ToString());
					if (FunctionOutput.ExpressionOutput->A.Expression != nullptr)
					{
						Edge.Target = BuildExpressionNode(FunctionOutput.ExpressionOutput->A.Expression, FunctionOutput.ExpressionOutput->A.OutputIndex, InContext);
					}
				}
			}
		}

		InContext.VisitingKeys.Remove(Key);
		return Node;
	}

	static TSharedPtr<FJsonObject> BuildExpressionJson(const TSharedPtr<FExpressionNode>& InNode)
	{
		if (!InNode.IsValid())
		{
			return nullptr;
		}

		TSharedPtr<FJsonObject> NodeObject = MakeShared<FJsonObject>();
		NodeObject->SetStringField(TEXT("id"), InNode->Id);
		NodeObject->SetStringField(TEXT("title"), InNode->Title);
		NodeObject->SetStringField(TEXT("nodeClass"), InNode->NodeClass);
		NodeObject->SetStringField(TEXT("details"), InNode->Details);

		TArray<TSharedPtr<FJsonValue>> InputArray;
		for (const FExpressionEdge& Edge : InNode->Inputs)
		{
			TSharedPtr<FJsonObject> EdgeObject = MakeShared<FJsonObject>();
			EdgeObject->SetStringField(TEXT("label"), Edge.Label);
			EdgeObject->SetStringField(TEXT("literal"), Edge.Literal);
			if (Edge.Target.IsValid())
			{
				EdgeObject->SetObjectField(TEXT("target"), BuildExpressionJson(Edge.Target));
			}
			InputArray.Add(MakeShared<FJsonValueObject>(EdgeObject));
		}
		NodeObject->SetArrayField(TEXT("inputs"), InputArray);

		return NodeObject;
	}

	static void AppendExpressionText(const TSharedPtr<FExpressionNode>& InNode, const FString& InIndent, TArray<FString>& OutLines)
	{
		if (!InNode.IsValid())
		{
			return;
		}

		const FString NodeLine = InNode->Details.IsEmpty()
			? InNode->Title
			: FString::Printf(TEXT("%s [%s]"), *InNode->Title, *InNode->Details);
		OutLines.Add(InIndent + NodeLine);

		for (const FExpressionEdge& Edge : InNode->Inputs)
		{
			if (Edge.Target.IsValid())
			{
				OutLines.Add(FString::Printf(TEXT("%s  %s <-"), *InIndent, *Edge.Label));
				AppendExpressionText(Edge.Target, InIndent + TEXT("    "), OutLines);
			}
			else if (!Edge.Literal.IsEmpty())
			{
				OutLines.Add(FString::Printf(TEXT("%s  %s = %s"), *InIndent, *Edge.Label, *Edge.Literal));
			}
		}
	}

	static FString EnumToString(const UEnum* InEnum, int64 InValue)
	{
		if (InEnum == nullptr)
		{
			return FString();
		}

		return InEnum->GetNameStringByValue(InValue);
	}

	static FString FormatParameterValue(const FMaterialParameterMetadata& InMetadata)
	{
		switch (InMetadata.Value.Type)
		{
		case EMaterialParameterType::Scalar:
			return FString::Printf(TEXT("%g"), InMetadata.Value.AsScalar());
		case EMaterialParameterType::Vector:
		{
			const FLinearColor Value = InMetadata.Value.AsLinearColor();
			return FString::Printf(TEXT("(R=%g,G=%g,B=%g,A=%g)"), Value.R, Value.G, Value.B, Value.A);
		}
		case EMaterialParameterType::DoubleVector:
		{
			const FVector4d Value = InMetadata.Value.AsVector4d();
			return FString::Printf(TEXT("(X=%g,Y=%g,Z=%g,W=%g)"), Value.X, Value.Y, Value.Z, Value.W);
		}
		case EMaterialParameterType::Texture:
		case EMaterialParameterType::TextureCollection:
		case EMaterialParameterType::RuntimeVirtualTexture:
		case EMaterialParameterType::SparseVolumeTexture:
		case EMaterialParameterType::ParameterCollection:
		{
			if (UObject* TextureObject = InMetadata.Value.AsTextureObject())
			{
				return TextureObject->GetPathName();
			}
			return TEXT("<null>");
		}
		case EMaterialParameterType::Font:
		{
			const UFont* FontValue = InMetadata.Value.Font.Value;
			return FontValue != nullptr
				? FString::Printf(TEXT("%s (Page=%d)"), *FontValue->GetPathName(), InMetadata.Value.Font.Page)
				: FString::Printf(TEXT("<null> (Page=%d)"), InMetadata.Value.Font.Page);
		}
		case EMaterialParameterType::StaticSwitch:
			return InMetadata.Value.AsStaticSwitch() ? TEXT("true") : TEXT("false");
		case EMaterialParameterType::StaticComponentMask:
		{
			const FStaticComponentMaskValue Value = InMetadata.Value.AsStaticComponentMask();
			return FString::Printf(TEXT("(R=%s,G=%s,B=%s,A=%s)"),
				Value.R ? TEXT("true") : TEXT("false"),
				Value.G ? TEXT("true") : TEXT("false"),
				Value.B ? TEXT("true") : TEXT("false"),
				Value.A ? TEXT("true") : TEXT("false"));
		}
		case EMaterialParameterType::None:
		default:
			return FString();
		}
	}

	static FString BuildDeclaredParameterKey(const FDeclaredParameter& InParameter)
	{
		return FString::Printf(TEXT("%s|%s|%s|%s"),
			*InParameter.Type,
			*InParameter.Name,
			*InParameter.Group,
			*InParameter.SourceAssetPath);
	}

	static void AddDeclaredParameter(FGraphSummary& OutSummary, TSet<FString>& InOutSeenKeys, FDeclaredParameter&& InParameter)
	{
		if (InParameter.Name.IsEmpty())
		{
			return;
		}

		const FString Key = BuildDeclaredParameterKey(InParameter);
		if (InOutSeenKeys.Contains(Key))
		{
			return;
		}

		InOutSeenKeys.Add(Key);
		OutSummary.DeclaredParameters.Add(MoveTemp(InParameter));
	}

	static bool TryBuildDeclaredParameterFromExpression(UMaterialExpression* InExpression, FDeclaredParameter& OutParameter)
	{
		if (InExpression == nullptr || !InExpression->HasAParameterName())
		{
			return false;
		}

		const FName ParameterName = InExpression->GetParameterName();
		if (ParameterName.IsNone())
		{
			return false;
		}

		FMaterialParameterMetadata Metadata;
		InExpression->GetParameterValue(Metadata);

		OutParameter.Name = ParameterName.ToString();
		OutParameter.Type = Metadata.Value.Type != EMaterialParameterType::None
			? MaterialParameterTypeToString(Metadata.Value.Type)
			: InExpression->GetClass()->GetName();
		OutParameter.Value = FormatParameterValue(Metadata);
		OutParameter.Group = Metadata.Group.ToString();
		OutParameter.SortPriority = Metadata.SortPriority;
		OutParameter.Description = NormalizeWhitespace(!Metadata.Description.IsEmpty() ? Metadata.Description : InExpression->Desc);
		OutParameter.SourceAssetPath = Metadata.AssetPath;

		if (OutParameter.SourceAssetPath.IsEmpty())
		{
			if (const UObject* Outer = InExpression->GetOuter())
			{
				OutParameter.SourceAssetPath = Outer->GetPathName();
			}
		}

		return true;
	}

	static void CollectDeclaredParametersFromExpressions(TConstArrayView<TObjectPtr<UMaterialExpression>> InExpressions, FGraphSummary& OutSummary, TSet<FString>& InOutSeenKeys)
	{
		for (UMaterialExpression* Expression : InExpressions)
		{
			FDeclaredParameter Parameter;
			if (TryBuildDeclaredParameterFromExpression(Expression, Parameter))
			{
				AddDeclaredParameter(OutSummary, InOutSeenKeys, MoveTemp(Parameter));
			}
		}
	}

	static void CollectDeclaredParametersFromMaterial(UMaterial* InMaterial, FGraphSummary& OutSummary)
	{
		if (InMaterial == nullptr)
		{
			return;
		}

		TSet<FString> SeenKeys;
		CollectDeclaredParametersFromExpressions(InMaterial->GetExpressions(), OutSummary, SeenKeys);

		TArray<UMaterialFunctionInterface*> Functions;
		InMaterial->GetDependentFunctions(Functions);
		for (UMaterialFunctionInterface* Function : Functions)
		{
			if (Function != nullptr)
			{
				CollectDeclaredParametersFromExpressions(Function->GetExpressions(), OutSummary, SeenKeys);
			}
		}
	}

	static void CollectDeclaredParametersFromFunction(UMaterialFunctionInterface* InFunction, FGraphSummary& OutSummary)
	{
		if (InFunction == nullptr)
		{
			return;
		}

		TSet<FString> SeenKeys;
		CollectDeclaredParametersFromExpressions(InFunction->GetExpressions(), OutSummary, SeenKeys);

		TArray<UMaterialFunctionInterface*> Functions;
		InFunction->GetDependentFunctions(Functions);
		for (UMaterialFunctionInterface* Function : Functions)
		{
			if (Function != nullptr)
			{
				CollectDeclaredParametersFromExpressions(Function->GetExpressions(), OutSummary, SeenKeys);
			}
		}
	}

	static void AddInterfaceDeclaredScalarParameters(UMaterialInterface* InMaterialInterface, FGraphSummary& OutSummary, TSet<FString>& InOutSeenKeys)
	{
		TArray<FMaterialParameterInfo> Infos;
		TArray<FGuid> Ids;
		InMaterialInterface->GetAllScalarParameterInfo(Infos, Ids);
		for (const FMaterialParameterInfo& Info : Infos)
		{
			FDeclaredParameter Parameter;
			Parameter.Name = Info.ToString();
			Parameter.Type = MaterialParameterTypeToString(EMaterialParameterType::Scalar);

			float Value = 0.0f;
			if (InMaterialInterface->GetScalarParameterValue(FHashedMaterialParameterInfo(Info), Value))
			{
				Parameter.Value = FString::Printf(TEXT("%g"), Value);
			}

			FName GroupName;
			if (InMaterialInterface->GetGroupName(FHashedMaterialParameterInfo(Info), GroupName))
			{
				Parameter.Group = GroupName.ToString();
			}

			int32 SortPriority = 0;
			if (InMaterialInterface->GetParameterSortPriority(FHashedMaterialParameterInfo(Info), SortPriority))
			{
				Parameter.SortPriority = SortPriority;
			}

			AddDeclaredParameter(OutSummary, InOutSeenKeys, MoveTemp(Parameter));
		}
	}

	static void AddInterfaceDeclaredVectorParameters(UMaterialInterface* InMaterialInterface, FGraphSummary& OutSummary, TSet<FString>& InOutSeenKeys)
	{
		TArray<FMaterialParameterInfo> Infos;
		TArray<FGuid> Ids;
		InMaterialInterface->GetAllVectorParameterInfo(Infos, Ids);
		for (const FMaterialParameterInfo& Info : Infos)
		{
			FDeclaredParameter Parameter;
			Parameter.Name = Info.ToString();
			Parameter.Type = MaterialParameterTypeToString(EMaterialParameterType::Vector);

			FLinearColor Value;
			if (InMaterialInterface->GetVectorParameterValue(FHashedMaterialParameterInfo(Info), Value))
			{
				Parameter.Value = FString::Printf(TEXT("(R=%g,G=%g,B=%g,A=%g)"), Value.R, Value.G, Value.B, Value.A);
			}

			FName GroupName;
			if (InMaterialInterface->GetGroupName(FHashedMaterialParameterInfo(Info), GroupName))
			{
				Parameter.Group = GroupName.ToString();
			}

			int32 SortPriority = 0;
			if (InMaterialInterface->GetParameterSortPriority(FHashedMaterialParameterInfo(Info), SortPriority))
			{
				Parameter.SortPriority = SortPriority;
			}

			AddDeclaredParameter(OutSummary, InOutSeenKeys, MoveTemp(Parameter));
		}
	}

	static void AddInterfaceDeclaredTextureParameters(UMaterialInterface* InMaterialInterface, FGraphSummary& OutSummary, TSet<FString>& InOutSeenKeys)
	{
		TArray<FMaterialParameterInfo> Infos;
		TArray<FGuid> Ids;
		InMaterialInterface->GetAllTextureParameterInfo(Infos, Ids);
		for (const FMaterialParameterInfo& Info : Infos)
		{
			FDeclaredParameter Parameter;
			Parameter.Name = Info.ToString();
			Parameter.Type = MaterialParameterTypeToString(EMaterialParameterType::Texture);

			UTexture* Value = nullptr;
			if (InMaterialInterface->GetTextureParameterValue(FHashedMaterialParameterInfo(Info), Value) && Value != nullptr)
			{
				Parameter.Value = Value->GetPathName();
			}

			FName GroupName;
			if (InMaterialInterface->GetGroupName(FHashedMaterialParameterInfo(Info), GroupName))
			{
				Parameter.Group = GroupName.ToString();
			}

			int32 SortPriority = 0;
			if (InMaterialInterface->GetParameterSortPriority(FHashedMaterialParameterInfo(Info), SortPriority))
			{
				Parameter.SortPriority = SortPriority;
			}

			AddDeclaredParameter(OutSummary, InOutSeenKeys, MoveTemp(Parameter));
		}
	}

	static void AddInterfaceDeclaredSwitchParameters(UMaterialInterface* InMaterialInterface, FGraphSummary& OutSummary, TSet<FString>& InOutSeenKeys)
	{
		TArray<FMaterialParameterInfo> Infos;
		TArray<FGuid> Ids;
		InMaterialInterface->GetAllStaticSwitchParameterInfo(Infos, Ids);
		for (const FMaterialParameterInfo& Info : Infos)
		{
			FDeclaredParameter Parameter;
			Parameter.Name = Info.ToString();
			Parameter.Type = MaterialParameterTypeToString(EMaterialParameterType::StaticSwitch);

			bool bValue = false;
			FGuid ExpressionGuid;
			if (InMaterialInterface->GetStaticSwitchParameterValue(FHashedMaterialParameterInfo(Info), bValue, ExpressionGuid))
			{
				Parameter.Value = bValue ? TEXT("true") : TEXT("false");
			}

			FName GroupName;
			if (InMaterialInterface->GetGroupName(FHashedMaterialParameterInfo(Info), GroupName))
			{
				Parameter.Group = GroupName.ToString();
			}

			int32 SortPriority = 0;
			if (InMaterialInterface->GetParameterSortPriority(FHashedMaterialParameterInfo(Info), SortPriority))
			{
				Parameter.SortPriority = SortPriority;
			}

			AddDeclaredParameter(OutSummary, InOutSeenKeys, MoveTemp(Parameter));
		}
	}

	static void CollectDeclaredParametersFromMaterialInterface(UMaterialInterface* InMaterialInterface, FGraphSummary& OutSummary)
	{
		if (InMaterialInterface == nullptr)
		{
			return;
		}

		TSet<FString> SeenKeys;
		AddInterfaceDeclaredScalarParameters(InMaterialInterface, OutSummary, SeenKeys);
		AddInterfaceDeclaredVectorParameters(InMaterialInterface, OutSummary, SeenKeys);
		AddInterfaceDeclaredTextureParameters(InMaterialInterface, OutSummary, SeenKeys);
		AddInterfaceDeclaredSwitchParameters(InMaterialInterface, OutSummary, SeenKeys);
	}

	static void AddDependentFunctions(const UMaterialInterface* InMaterialInterface, FGraphSummary& OutSummary)
	{
		if (InMaterialInterface == nullptr)
		{
			return;
		}

		TArray<UMaterialFunctionInterface*> Functions;
		InMaterialInterface->GetDependentFunctions(Functions);
		for (UMaterialFunctionInterface* Function : Functions)
		{
			if (Function != nullptr)
			{
				OutSummary.DependentFunctions.AddUnique(Function->GetPathName());
			}
		}
	}

	static void AddDependentFunctions(const UMaterialFunctionInterface* InMaterialFunction, FGraphSummary& OutSummary)
	{
		if (InMaterialFunction == nullptr)
		{
			return;
		}

		TArray<UMaterialFunctionInterface*> Functions;
		InMaterialFunction->GetDependentFunctions(Functions);
		for (UMaterialFunctionInterface* Function : Functions)
		{
			if (Function != nullptr)
			{
				OutSummary.DependentFunctions.AddUnique(Function->GetPathName());
			}
		}
	}

	static TArray<FRootInput> CollectMaterialRootInputs(UMaterial* InMaterial)
	{
		TArray<FRootInput> Roots;
		if (InMaterial == nullptr)
		{
			return Roots;
		}

		struct FPropertyEntry
		{
			EMaterialProperty Property;
			const TCHAR* Label;
		};

		// Engine note: export only properties available in the current engine version.
		static const TArray<FPropertyEntry> PropertyEntries =
		{
			{ MP_MaterialAttributes, TEXT("Material Attributes") },
			{ MP_FrontMaterial, TEXT("Front Material") },
			{ MP_BaseColor, TEXT("Base Color") },
			{ MP_Metallic, TEXT("Metallic") },
			{ MP_Specular, TEXT("Specular") },
			{ MP_Roughness, TEXT("Roughness") },
			{ MP_Anisotropy, TEXT("Anisotropy") },
			{ MP_Normal, TEXT("Normal") },
			{ MP_Tangent, TEXT("Tangent") },
			{ MP_EmissiveColor, TEXT("Emissive Color") },
			{ MP_Opacity, TEXT("Opacity") },
			{ MP_OpacityMask, TEXT("Opacity Mask") },
			{ MP_WorldPositionOffset, TEXT("World Position Offset") },
			{ MP_Displacement, TEXT("Displacement") },
			{ MP_SubsurfaceColor, TEXT("Subsurface Color") },
			{ MP_AmbientOcclusion, TEXT("Ambient Occlusion") },
			{ MP_Refraction, TEXT("Refraction") },
			{ MP_PixelDepthOffset, TEXT("Pixel Depth Offset") },
			{ MP_ShadingModel, TEXT("Shading Model") },
			{ MP_SurfaceThickness, TEXT("Surface Thickness") },
		};

		for (const FPropertyEntry& Entry : PropertyEntries)
		{
			if (!InMaterial->IsPropertySupported(Entry.Property) || !InMaterial->IsPropertyConnected(Entry.Property))
			{
				continue;
			}

			if (FExpressionInput* Input = InMaterial->GetExpressionInputForProperty(Entry.Property))
			{
				FRootInput& Root = Roots.AddDefaulted_GetRef();
				Root.Label = Entry.Label;
				Root.Input = Input;
			}
		}

		return Roots;
	}

	static void BuildMaterialRoots(UMaterial* InMaterial, FGraphSummary& OutSummary)
	{
		for (const FRootInput& RootInput : CollectMaterialRootInputs(InMaterial))
		{
			FRootChain& Root = OutSummary.Roots.AddDefaulted_GetRef();
			Root.Label = RootInput.Label;
			if (RootInput.Input != nullptr && RootInput.Input->Expression != nullptr)
			{
				FBuildContext BuildContext;
				Root.RootNode = BuildExpressionNode(RootInput.Input->Expression, RootInput.Input->OutputIndex, BuildContext);
			}
		}

		TArray<UMaterialExpressionCustomOutput*> CustomOutputs;
		InMaterial->GetAllCustomOutputExpressions(CustomOutputs);
		for (UMaterialExpressionCustomOutput* CustomOutput : CustomOutputs)
		{
			if (CustomOutput == nullptr)
			{
				continue;
			}

			FRootChain& Root = OutSummary.Roots.AddDefaulted_GetRef();
			Root.Label = FString::Printf(TEXT("Custom Output: %s"), *GetCaptionText(CustomOutput));
			FBuildContext BuildContext;
			Root.RootNode = BuildExpressionNode(CustomOutput, 0, BuildContext);
		}
	}

	static void BuildMaterialFunctionRoots(UMaterialFunctionInterface* InFunction, FGraphSummary& OutSummary)
	{
		TArray<UMaterialExpressionFunctionOutput*> Outputs;
		InFunction->GetAllExpressionsOfType<UMaterialExpressionFunctionOutput>(Outputs, false);
		Outputs.Sort(
			[](const UMaterialExpressionFunctionOutput& A, const UMaterialExpressionFunctionOutput& B)
			{
				return A.SortPriority == B.SortPriority ? A.OutputName.LexicalLess(B.OutputName) : A.SortPriority < B.SortPriority;
			});

		for (UMaterialExpressionFunctionOutput* Output : Outputs)
		{
			if (Output == nullptr)
			{
				continue;
			}

			FRootChain& Root = OutSummary.Roots.AddDefaulted_GetRef();
			Root.Label = Output->OutputName.IsNone() ? TEXT("Function Output") : Output->OutputName.ToString();
			if (Output->A.Expression != nullptr)
			{
				FBuildContext BuildContext;
				Root.RootNode = BuildExpressionNode(Output->A.Expression, Output->A.OutputIndex, BuildContext);
			}
		}
	}

	static void AddInstanceParameterNotes(UMaterialInterface* InMaterialInterface, FGraphSummary& OutSummary)
	{
		TArray<FMaterialParameterInfo> ScalarInfos;
		TArray<FGuid> ScalarIds;
		InMaterialInterface->GetAllScalarParameterInfo(ScalarInfos, ScalarIds);
		for (const FMaterialParameterInfo& Info : ScalarInfos)
		{
			float Value = 0.0f;
			if (InMaterialInterface->GetScalarParameterValue(FHashedMaterialParameterInfo(Info), Value))
			{
				OutSummary.Notes.Add(FString::Printf(TEXT("ScalarParameter %s = %g"), *Info.Name.ToString(), Value));
			}
		}

		TArray<FMaterialParameterInfo> VectorInfos;
		TArray<FGuid> VectorIds;
		InMaterialInterface->GetAllVectorParameterInfo(VectorInfos, VectorIds);
		for (const FMaterialParameterInfo& Info : VectorInfos)
		{
			FLinearColor Value;
			if (InMaterialInterface->GetVectorParameterValue(FHashedMaterialParameterInfo(Info), Value))
			{
				OutSummary.Notes.Add(FString::Printf(TEXT("VectorParameter %s = (R=%g,G=%g,B=%g,A=%g)"), *Info.Name.ToString(), Value.R, Value.G, Value.B, Value.A));
			}
		}

		TArray<FMaterialParameterInfo> TextureInfos;
		TArray<FGuid> TextureIds;
		InMaterialInterface->GetAllTextureParameterInfo(TextureInfos, TextureIds);
		for (const FMaterialParameterInfo& Info : TextureInfos)
		{
			UTexture* Value = nullptr;
			if (InMaterialInterface->GetTextureParameterValue(FHashedMaterialParameterInfo(Info), Value) && Value != nullptr)
			{
				OutSummary.Notes.Add(FString::Printf(TEXT("TextureParameter %s = %s"), *Info.Name.ToString(), *Value->GetPathName()));
			}
		}

		TArray<FMaterialParameterInfo> SwitchInfos;
		TArray<FGuid> SwitchIds;
		InMaterialInterface->GetAllStaticSwitchParameterInfo(SwitchInfos, SwitchIds);
		for (const FMaterialParameterInfo& Info : SwitchInfos)
		{
			bool bValue = false;
			FGuid ExpressionGuid;
			if (InMaterialInterface->GetStaticSwitchParameterValue(FHashedMaterialParameterInfo(Info), bValue, ExpressionGuid))
			{
				OutSummary.Notes.Add(FString::Printf(TEXT("StaticSwitch %s = %s"), *Info.Name.ToString(), bValue ? TEXT("true") : TEXT("false")));
			}
		}
	}

	static FGraphSummary BuildSummary(UObject* InAsset)
	{
		FGraphSummary Summary;
		Summary.AssetPath = InAsset->GetPathName();
		Summary.AssetName = InAsset->GetName();
		Summary.AssetKind = InAsset->GetClass()->GetName();

		if (UMaterial* Material = Cast<UMaterial>(InAsset))
		{
			Summary.AssetKind = TEXT("Material");
			Summary.MaterialDomain = EnumToString(StaticEnum<EMaterialDomain>(), Material->MaterialDomain);
			// Engine note: avoid depending on a non-exported UMaterial helper from a plugin module.
			Summary.BlendMode = EnumToString(StaticEnum<EBlendMode>(), Material->BlendMode);
			Summary.bTwoSided = Material->TwoSided;
			AddDependentFunctions(Material, Summary);
			CollectDeclaredParametersFromMaterial(Material, Summary);
			BuildMaterialRoots(Material, Summary);
		}
		else if (UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(InAsset))
		{
			Summary.AssetKind = TEXT("MaterialInstance");
			Summary.ParentPath = MaterialInstance->Parent ? MaterialInstance->Parent->GetPathName() : FString();
			if (UMaterial* BaseMaterial = MaterialInstance->GetMaterial())
			{
				Summary.MaterialDomain = EnumToString(StaticEnum<EMaterialDomain>(), BaseMaterial->MaterialDomain);
				// Engine note: avoid depending on a non-exported UMaterial helper from a plugin module.
				Summary.BlendMode = EnumToString(StaticEnum<EBlendMode>(), MaterialInstance->GetBlendMode());
				Summary.bTwoSided = BaseMaterial->TwoSided;
				AddDependentFunctions(MaterialInstance, Summary);
				CollectDeclaredParametersFromMaterialInterface(MaterialInstance, Summary);
				AddInstanceParameterNotes(MaterialInstance, Summary);
				BuildMaterialRoots(BaseMaterial, Summary);
			}
		}
		else if (UMaterialFunctionInterface* MaterialFunction = Cast<UMaterialFunctionInterface>(InAsset))
		{
			Summary.AssetKind = TEXT("MaterialFunction");
			Summary.Notes.Add(FString::Printf(TEXT("FunctionUsage=%d"), static_cast<int32>(MaterialFunction->GetMaterialFunctionUsage())));
			AddDependentFunctions(MaterialFunction, Summary);
			CollectDeclaredParametersFromFunction(MaterialFunction, Summary);
			BuildMaterialFunctionRoots(MaterialFunction, Summary);

			if (const UMaterialFunction* ConcreteFunction = Cast<UMaterialFunction>(MaterialFunction))
			{
				if (!ConcreteFunction->Description.IsEmpty())
				{
					Summary.Notes.Add(FString::Printf(TEXT("Description=%s"), *NormalizeWhitespace(ConcreteFunction->Description)));
				}
				if (!ConcreteFunction->UserExposedCaption.IsEmpty())
				{
					Summary.Notes.Add(FString::Printf(TEXT("Caption=%s"), *NormalizeWhitespace(ConcreteFunction->UserExposedCaption)));
				}
			}
			else if (const UMaterialFunctionInstance* FunctionInstance = Cast<UMaterialFunctionInstance>(MaterialFunction))
			{
				Summary.ParentPath = FunctionInstance->Parent ? FunctionInstance->Parent->GetPathName() : FString();
			}
		}

		return Summary;
	}

	static FString BuildTextOutput(const FGraphSummary& InSummary)
	{
		TArray<FString> Lines;
		Lines.Add(FString::Printf(TEXT("Asset: %s"), *InSummary.AssetPath));
		Lines.Add(FString::Printf(TEXT("AssetKind: %s"), *InSummary.AssetKind));
		if (!InSummary.ParentPath.IsEmpty())
		{
			Lines.Add(FString::Printf(TEXT("Parent: %s"), *InSummary.ParentPath));
		}
		if (!InSummary.MaterialDomain.IsEmpty())
		{
			Lines.Add(FString::Printf(TEXT("MaterialDomain: %s"), *InSummary.MaterialDomain));
		}
		if (!InSummary.BlendMode.IsEmpty())
		{
			Lines.Add(FString::Printf(TEXT("BlendMode: %s"), *InSummary.BlendMode));
		}
		if (InSummary.bTwoSided)
		{
			Lines.Add(TEXT("TwoSided: true"));
		}
		if (InSummary.DependentFunctions.Num() > 0)
		{
			Lines.Add(TEXT("DependentFunctions:"));
			for (const FString& FunctionPath : InSummary.DependentFunctions)
			{
				Lines.Add(TEXT("  ") + FunctionPath);
			}
		}
		if (InSummary.Notes.Num() > 0)
		{
			Lines.Add(TEXT("Notes:"));
			for (const FString& Note : InSummary.Notes)
			{
				Lines.Add(TEXT("  ") + Note);
			}
		}
		if (InSummary.DeclaredParameters.Num() > 0)
		{
			Lines.Add(TEXT("DeclaredParameters:"));
			for (const FDeclaredParameter& Parameter : InSummary.DeclaredParameters)
			{
				FString Line = FString::Printf(TEXT("  %s %s"), *Parameter.Type, *Parameter.Name);
				if (!Parameter.Value.IsEmpty())
				{
					Line += FString::Printf(TEXT(" = %s"), *Parameter.Value);
				}
				if (!Parameter.Group.IsEmpty())
				{
					Line += FString::Printf(TEXT(" [Group=%s]"), *Parameter.Group);
				}
				if (Parameter.SortPriority != 0)
				{
					Line += FString::Printf(TEXT(" [SortPriority=%d]"), Parameter.SortPriority);
				}
				if (!Parameter.SourceAssetPath.IsEmpty())
				{
					Line += FString::Printf(TEXT(" [Source=%s]"), *Parameter.SourceAssetPath);
				}
				if (!Parameter.Description.IsEmpty())
				{
					Line += FString::Printf(TEXT(" [Desc=%s]"), *Parameter.Description);
				}
				Lines.Add(Line);
			}
		}

		Lines.Add(TEXT(""));
		Lines.Add(TEXT("Graph:"));
		for (const FRootChain& Root : InSummary.Roots)
		{
			Lines.Add(TEXT("  [Root] ") + Root.Label);
			if (Root.RootNode.IsValid())
			{
				AppendExpressionText(Root.RootNode, TEXT("    "), Lines);
			}
			else if (!Root.Literal.IsEmpty())
			{
				Lines.Add(TEXT("    ") + Root.Literal);
			}
			Lines.Add(TEXT(""));
		}

		return FString::Join(Lines, TEXT("\r\n"));
	}

	static FString BuildJsonOutput(const FGraphSummary& InSummary)
	{
		TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
		RootObject->SetStringField(TEXT("assetPath"), InSummary.AssetPath);
		RootObject->SetStringField(TEXT("assetName"), InSummary.AssetName);
		RootObject->SetStringField(TEXT("assetKind"), InSummary.AssetKind);
		RootObject->SetStringField(TEXT("materialDomain"), InSummary.MaterialDomain);
		RootObject->SetStringField(TEXT("blendMode"), InSummary.BlendMode);
		RootObject->SetBoolField(TEXT("twoSided"), InSummary.bTwoSided);
		RootObject->SetStringField(TEXT("parentPath"), InSummary.ParentPath);

		TArray<TSharedPtr<FJsonValue>> FunctionArray;
		for (const FString& FunctionPath : InSummary.DependentFunctions)
		{
			FunctionArray.Add(MakeShared<FJsonValueString>(FunctionPath));
		}
		RootObject->SetArrayField(TEXT("dependentFunctions"), FunctionArray);

		TArray<TSharedPtr<FJsonValue>> NotesArray;
		for (const FString& Note : InSummary.Notes)
		{
			NotesArray.Add(MakeShared<FJsonValueString>(Note));
		}
		RootObject->SetArrayField(TEXT("notes"), NotesArray);

		TArray<TSharedPtr<FJsonValue>> ParameterArray;
		for (const FDeclaredParameter& Parameter : InSummary.DeclaredParameters)
		{
			TSharedPtr<FJsonObject> ParameterObject = MakeShared<FJsonObject>();
			ParameterObject->SetStringField(TEXT("name"), Parameter.Name);
			ParameterObject->SetStringField(TEXT("type"), Parameter.Type);
			ParameterObject->SetStringField(TEXT("value"), Parameter.Value);
			ParameterObject->SetStringField(TEXT("group"), Parameter.Group);
			ParameterObject->SetNumberField(TEXT("sortPriority"), Parameter.SortPriority);
			ParameterObject->SetStringField(TEXT("description"), Parameter.Description);
			ParameterObject->SetStringField(TEXT("sourceAssetPath"), Parameter.SourceAssetPath);
			ParameterArray.Add(MakeShared<FJsonValueObject>(ParameterObject));
		}
		RootObject->SetArrayField(TEXT("declaredParameters"), ParameterArray);

		TArray<TSharedPtr<FJsonValue>> RootArray;
		for (const FRootChain& Root : InSummary.Roots)
		{
			TSharedPtr<FJsonObject> RootNode = MakeShared<FJsonObject>();
			RootNode->SetStringField(TEXT("label"), Root.Label);
			RootNode->SetStringField(TEXT("literal"), Root.Literal);
			if (Root.RootNode.IsValid())
			{
				RootNode->SetObjectField(TEXT("node"), BuildExpressionJson(Root.RootNode));
			}
			RootArray.Add(MakeShared<FJsonValueObject>(RootNode));
		}
		RootObject->SetArrayField(TEXT("roots"), RootArray);

		FString JsonOutput;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonOutput);
		FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
		return JsonOutput;
	}

	static void BuildExportPaths(UObject* InAsset, FString& OutTextPath, FString& OutJsonPath)
	{
		const FString PackageName = InAsset->GetOutermost()->GetName();
		FString RelativeDirectory = FPackageName::GetLongPackagePath(PackageName);
		RelativeDirectory.RemoveFromStart(TEXT("/"));
		RelativeDirectory.ReplaceInline(TEXT("/"), TEXT("\\"));

		const FString ExportDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("MaterialExports"), RelativeDirectory);
		IFileManager::Get().MakeDirectory(*ExportDirectory, true);

		const FString AssetName = FPackageName::GetLongPackageAssetName(PackageName);
		OutTextPath = FPaths::Combine(ExportDirectory, AssetName + TEXT(".txt"));
		OutJsonPath = FPaths::Combine(ExportDirectory, AssetName + TEXT(".json"));
	}
}

bool FMaterialTextExportService::ExportMaterialAsset(UObject* InAsset, FString& OutTextPath, FString& OutJsonPath, FString& OutErrorMessage)
{
	using namespace UE::MaterialTextExporter;

	if (InAsset == nullptr)
	{
		OutErrorMessage = TEXT("Asset is null.");
		return false;
	}

	if (!InAsset->IsA<UMaterialInterface>() && !InAsset->IsA<UMaterialFunctionInterface>())
	{
		OutErrorMessage = TEXT("Unsupported asset type.");
		return false;
	}

	FGraphSummary Summary = BuildSummary(InAsset);
	// Moon Modified: Material instances may intentionally have no local graph or overrides,
	// while their parent and material settings remain useful export data.

	BuildExportPaths(InAsset, OutTextPath, OutJsonPath);

	const FString TextOutput = BuildTextOutput(Summary);
	const FString JsonOutput = BuildJsonOutput(Summary);

	if (!FFileHelper::SaveStringToFile(TextOutput, *OutTextPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		OutErrorMessage = FString::Printf(TEXT("Failed to save text output to '%s'."), *OutTextPath);
		return false;
	}

	if (!FFileHelper::SaveStringToFile(JsonOutput, *OutJsonPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		OutErrorMessage = FString::Printf(TEXT("Failed to save JSON output to '%s'."), *OutJsonPath);
		return false;
	}

	return true;
}
