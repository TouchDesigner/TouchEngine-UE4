/* Shared Use License: This file is owned by Derivative Inc. (Derivative)
* and can only be used, and/or modified for use, in conjunction with
* Derivative's TouchDesigner software, and only if you are a licensee who has
* accepted Derivative's TouchDesigner license or assignment agreement
* (which also govern the use of this file). You may share or redistribute
* a modified version of this file provided the following conditions are met:
*
* 1. The shared file or redistribution must retain the information set out
* above and this list of conditions.
* 2. Derivative's name (Derivative Inc.) or its trademarks may not be used
* to endorse or promote products derived from this file without specific
* prior written permission from Derivative.
*/

#include "TouchOutputK2Node.h"

#include "Blueprint/TouchBlueprintFunctionLibrary.h"
#include "Blueprint/TouchEngineComponent.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "KismetCompiler.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "TouchOutputK2Node"

FText UTouchOutputK2Node::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("GetTEOutput", "Get TouchEngine Output");
}

void UTouchOutputK2Node::PostReconstructNode()
{
	Super::PostReconstructNode();

	if (UEdGraphPin* Pin = FindPin(FPinNames::Value))
	{
		NotifyPinConnectionListChanged(Pin); // this handles the renaming of the Pin, avoids code duplication
	}
}

void UTouchOutputK2Node::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	/*Create our pins*/

	// Execution pins
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	// Variable pins

	//Output
	UEdGraphPin* InObjectPin = CreateTouchComponentPin(LOCTEXT("Inputs_ObjectTooltip", "TouchEngine Component to get output from"));
	UEdGraphPin* InTextPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_String, FPinNames::OutputName);
	K2Schema->SetPinAutogeneratedDefaultValue(InTextPin, FString(""));
	UEdGraphPin* OutValuePin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, FPinNames::Value);

	//Output
	UEdGraphPin* OutValidPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, FPinNames::Result);
	K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(OutValidPin);
	UEdGraphPin* OutFrameLastUpdatedPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Int64, FPinNames::OutputFrameLastUpdated);
	K2Schema->SetPinAutogeneratedDefaultValue(OutFrameLastUpdatedPin, TEXT("-1"));
}

void UTouchOutputK2Node::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (!FindPin(FPinNames::TouchEngineComponent)->HasAnyConnections())
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("NoTargetComponent", "No Target Component connected").ToString(), this);
	}

	// Check input pin type to make sure it's a supported type for TouchEngine
	if (!IsPinCategoryValid(FindPin(FPinNames::Value)))
	{
		// pin type is not valid
		FindPin(FPinNames::Value)->BreakAllPinLinks();
		return;
	}

	//This is just a hard reference to the static method that lives in the BlueprintLibrary. Probably not the best of ways.
	const UEdGraphPin* ValuePin = FindPin(FPinNames::Value);

	const UFunction* BlueprintFunction = UTouchBlueprintFunctionLibrary::FindGetterByType(
		GetCategoryNameChecked(ValuePin),
		ValuePin->PinType.ContainerType == EPinContainerType::Array,
		ValuePin->PinType.PinSubCategoryObject.IsValid() ? ValuePin->PinType.PinSubCategoryObject->GetFName() : FName("")
	);

	if (BlueprintFunction == nullptr) {
		CompilerContext.MessageLog.Error(*LOCTEXT("InvalidFunctionName", "The function has not been found.").ToString(), this);
		return;
	}

	UK2Node_CallFunction* CallFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);

	CallFunction->SetFromFunction(BlueprintFunction);
	CallFunction->AllocateDefaultPins();
	CallFunction->FindPinChecked(FPinNames::Prefix)->DefaultValue = "o/";
	CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallFunction, this);

	ValidateLegacyVariableNames(FPinNames::OutputName, CompilerContext, "o/");

	//Input
	CompilerContext.MovePinLinksToIntermediate(*FindPin(FPinNames::OutputName), *CallFunction->FindPin(FFunctionParametersNames::ParameterName));
	CompilerContext.MovePinLinksToIntermediate(*FindPin(FPinNames::TouchEngineComponent), *CallFunction->FindPin(FFunctionParametersNames::TouchEngineComponent));

	//Output
	CompilerContext.MovePinLinksToIntermediate(*FindPin(FPinNames::Value), *CallFunction->FindPin(FFunctionParametersNames::Value));
	CompilerContext.MovePinLinksToIntermediate(*FindPin(FPinNames::Result), *CallFunction->GetReturnValuePin());
	CompilerContext.MovePinLinksToIntermediate(*FindPin(FPinNames::OutputFrameLastUpdated), *CallFunction->FindPin(FFunctionParametersNames::OutputFrameLastUpdated));

	//Exec pins
	UEdGraphPin* NodeExec = GetExecPin();
	UEdGraphPin* NodeThen = FindPin(UEdGraphSchema_K2::PN_Then);

	UEdGraphPin* InternalExec = CallFunction->GetExecPin();
	CompilerContext.MovePinLinksToIntermediate(*NodeExec, *InternalExec);

	UEdGraphPin* InternalThen = CallFunction->GetThenPin();
	CompilerContext.MovePinLinksToIntermediate(*NodeThen, *InternalThen);

	//After we are done we break all links to this node (not the internally created one)
	BreakAllNodeLinks();
}

void UTouchOutputK2Node::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	Super::GetMenuActions(ActionRegistrar);

	const UClass* Action = GetClass();

	if (ActionRegistrar.IsOpenForRegistration(Action)) {
		UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
		check(Spawner != nullptr);

		ActionRegistrar.AddBlueprintAction(Action, Spawner);
	}
}

void UTouchOutputK2Node::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	Super::ReallocatePinsDuringReconstruction(OldPins);

	// This is necessary to retain type information after pasting or loading from disc
	if (UEdGraphPin* OutputPin = FindPin(FPinNames::Value))
	{
		// Only update the output pin if it is currently a wildcard
		if (OutputPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
		{
			// Find the matching Old Pin if it exists
			for (const UEdGraphPin* OldPin : OldPins)
			{
				if (OldPin->PinName == OutputPin->PinName)
				{
					// Update our output pin with the old type information
					OutputPin->PinType = OldPin->PinType;

					const UEdGraphSchema* Schema = GetSchema();
					Schema->RecombinePin(OutputPin);

					break;
				}
			}
		}
	}
}

void UTouchOutputK2Node::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	if (Pin->PinName == FPinNames::Value)
	{
		if (Pin->HasAnyConnections())
		{
			if (!IsPinCategoryValid(Pin->LinkedTo[0]))
			{
				Pin->BreakAllPinLinks();
				return;
			}

			Pin->PinType = Pin->LinkedTo[0]->PinType;

			const UFunction* BlueprintFunction = UTouchBlueprintFunctionLibrary::FindGetterByType(
				GetCategoryNameChecked(Pin),
				Pin->PinType.ContainerType == EPinContainerType::Array,
				Pin->PinType.PinSubCategoryObject.IsValid() ? Pin->PinType.PinSubCategoryObject->GetFName() : FName("")
			);
			if (BlueprintFunction)
			{
				for (const FProperty* Property : TFieldRange<FProperty>(BlueprintFunction))
				{
					if (Property->NamePrivate == TEXT("value"))
					{
						Pin->PinFriendlyName = Property->GetDisplayNameText();
						Pin->PinToolTip = Property->GetToolTipText().ToString();
						return;
					}
				}
			}
			
			Pin->PinFriendlyName = FText::FromName(Pin->PinName);
		}
		else
		{
			Pin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
			Pin->PinType.ContainerType = EPinContainerType::None;
			Pin->PinFriendlyName = FText::FromName(Pin->PinName);
		}
	}
}

bool UTouchOutputK2Node::IsPinCategoryValid(UEdGraphPin* Pin) const
{
	const FName PinCategory = Pin->PinType.PinCategory;

	if (PinCategory == UEdGraphSchema_K2::PC_Float ||
		PinCategory == UEdGraphSchema_K2::PC_Double ||
		PinCategory == UEdGraphSchema_K2::PC_String)
	{
		return true;
	}
	else if (PinCategory == UEdGraphSchema_K2::PC_Object)
	{
		if (const UClass* ObjectClass = Cast<UClass>(Pin->PinType.PinSubCategoryObject.Get()))
		{
			if (ObjectClass == UTexture2D::StaticClass() || ObjectClass->IsChildOf<UTexture2D>() || UTexture2D::StaticClass()->IsChildOf(ObjectClass))
			{
				return true;
			}
			if (ObjectClass == UTouchEngineDAT::StaticClass() || ObjectClass->IsChildOf<UTouchEngineDAT>() || UTouchEngineDAT::StaticClass()->IsChildOf(ObjectClass))
			{
				return true;
			}
		}
	}
	else if (PinCategory == UEdGraphSchema_K2::PC_Struct)
	{
		const UStruct* ObjectStruct = Cast<UStruct>(Pin->PinType.PinSubCategoryObject.Get());
		if (ObjectStruct && ObjectStruct == FTouchEngineCHOP::StaticStruct() || FTouchEngineCHOP::StaticStruct()->IsChildOf(ObjectStruct))
		{
			return true;
		}
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
