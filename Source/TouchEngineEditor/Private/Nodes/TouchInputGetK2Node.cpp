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


#include "TouchInputGetK2Node.h"

#include "Blueprint/TouchBlueprintFunctionLibrary.h"
#include "Blueprint/TouchEngineComponent.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "KismetCompiler.h"

#define LOCTEXT_NAMESPACE "TouchGetInputK2Node"

void UTouchInputGetK2Node::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Create out pins

	// Execution Pins
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	// Variable Pins

	// Input
	UEdGraphPin* InObjectPin = CreateTouchComponentPin(LOCTEXT("Inputs_ObjectTooltip", "TouchEngine Component to get input values from"));
	UEdGraphPin* InTextPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_String, FPinNames::InputName);
	K2Schema->SetPinAutogeneratedDefaultValue(InTextPin, FString(""));
	// Output
	UEdGraphPin* OutValuePin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, FPinNames::Value);
	UEdGraphPin* OutValidPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, FPinNames::Result);
	K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(OutValidPin);
}

FText UTouchInputGetK2Node::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("GetTEInput", "Get TouchEngine Input");
}

void UTouchInputGetK2Node::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (!FindPin(FPinNames::TouchEngineComponent)->HasAnyConnections())
	{
		CompilerContext.MessageLog.Error(*(FString("No Target Component connected")), this);
	}

	// Check input pin type to make sure it's a supported type for touchengine
	UEdGraphPin* ValuePin = FindPin(FPinNames::Value);

	if (!CheckPinCategory(ValuePin))
	{
		// pin type is not valid
		ValuePin->BreakAllPinLinks();
		return;
	}

	// get the proper function from the library based on pin category
	UFunction* BlueprintFunction = UTouchBlueprintFunctionLibrary::FindInputGetterByType(
		GetCategoryNameChecked(ValuePin),
		ValuePin->PinType.ContainerType == EPinContainerType::Array,
		ValuePin->PinType.PinSubCategoryObject.IsValid() ? ValuePin->PinType.PinSubCategoryObject->GetFName() : FName("")
	);

	if (BlueprintFunction == nullptr)
	{
		CompilerContext.MessageLog.Error(*FString("The function has not been found."), this);
		return;
	}

	UK2Node_CallFunction* CallFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallFunction->SetFromFunction(BlueprintFunction);
	CallFunction->AllocateDefaultPins();
	CallFunction->FindPinChecked(FPinNames::Prefix)->DefaultValue = "i/";
	CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallFunction, this);

	ValidateLegacyVariableNames(FPinNames::InputName, CompilerContext, "i/");

	//Input
	CompilerContext.MovePinLinksToIntermediate(*FindPin(FPinNames::InputName), *CallFunction->FindPin(TEXT("VarName")));
	CompilerContext.MovePinLinksToIntermediate(*FindPin(FPinNames::TouchEngineComponent), *CallFunction->FindPin(TEXT("Target")));

	//Output
	CompilerContext.MovePinLinksToIntermediate(*FindPin(FPinNames::Value), *CallFunction->FindPin(TEXT("value")));
	CompilerContext.MovePinLinksToIntermediate(*FindPin(FPinNames::Result), *CallFunction->GetReturnValuePin());

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

void UTouchInputGetK2Node::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	Super::GetMenuActions(ActionRegistrar);

	UClass* Action = GetClass();

	if (ActionRegistrar.IsOpenForRegistration(Action)) {
		UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
		check(Spawner != nullptr);

		ActionRegistrar.AddBlueprintAction(Action, Spawner);
	}
}

void UTouchInputGetK2Node::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	Super::ReallocatePinsDuringReconstruction(OldPins);

	// This is necessary to retain type information after pasting or loading from disc
	if (UEdGraphPin* InputPin = FindPin(FPinNames::Value))
	{
		// Only update the output pin if it is currently a wildcard
		if (InputPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
		{
			// Find the matching Old Pin if it exists
			for (UEdGraphPin* OldPin : OldPins)
			{
				if (OldPin->PinName == InputPin->PinName)
				{
					// Update our output pin with the old type information and then propagate it to our input pins
					InputPin->PinType = OldPin->PinType;

					const UEdGraphSchema* Schema = GetSchema();
					Schema->RecombinePin(InputPin);

					break;
				}
			}
		}
	}
}

void UTouchInputGetK2Node::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	if (Pin->PinName == FPinNames::Value)
	{
		if (Pin->HasAnyConnections())
		{
			// Check input pin type to make sure it's a supported type for touchengine
			if (!CheckPinCategory(Pin->LinkedTo[0]))
			{
				// pin type is not valid
				Pin->BreakAllPinLinks();
				return;
			}

			Pin->PinType = Pin->LinkedTo[0]->PinType;
		}
		else
		{
			Pin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
			Pin->PinType.ContainerType = EPinContainerType::None;
		}
	}
}

#undef LOCTEXT_NAMESPACE
