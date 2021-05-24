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

#include "TouchInputK2Node.h"

#include "TouchBlueprintFunctionLibrary.h"

#include "KismetCompiler.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "TouchEngineComponent.h"	
#include "Engine/TextureRenderTarget2D.h"

#define LOCTEXT_NAMESPACE "UTouchInputK2Node"


struct FTEInput_GetPinNames
{
	static const FName& GetPinNameVarName()
	{
		static const FName TextPinName(TEXT("InputName"));
		return TextPinName;
	}
	static const FName& GetPinNameComponent()
	{
		static const FName TextPinName(TEXT("TouchEngineComponent"));
		return TextPinName;
	}
	static const FName& GetPinNameValue()
	{
		static const FName TextPinName(TEXT("Value"));
		return TextPinName;
	}

	static const FName& GetPinNameOutput()
	{
		static const FName OutputPinName(TEXT("Result"));
		return OutputPinName;
	}
};


FText UTouchInputK2Node::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString("Set TouchEngine Input");//LOCTEXT("TouchSetInput_K2Node", "Set TouchEngine Input");
}

void UTouchInputK2Node::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	/*Create our pins*/

	// Execution pins
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	// Variable pins

	//Input
	UEdGraphPin* InTextPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_String, FTEInput_GetPinNames::GetPinNameVarName());
	K2Schema->SetPinAutogeneratedDefaultValue(InTextPin, FString(""));
	UEdGraphPin* InObjectPin = CreatePin(EGPD_Input, FName(TEXT("Object")), UTouchEngineComponentBase::StaticClass(), FTEInput_GetPinNames::GetPinNameComponent());
	UEdGraphPin* InValuePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, FTEInput_GetPinNames::GetPinNameValue());

	//Output
	UEdGraphPin* OutValidPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, FTEInput_GetPinNames::GetPinNameOutput());
	K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(OutValidPin);
}

FText UTouchInputK2Node::GetTooltipText() const
{
	return FText::FromString("Set TouchEngine Input");
}

FText UTouchInputK2Node::GetMenuCategory() const
{
	return LOCTEXT("TouchEnigne_MenuCategory", "TouchEngine");
}

void UTouchInputK2Node::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);


	if (!FindPin(FTEInput_GetPinNames::GetPinNameComponent())->HasAnyConnections())
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("NoTargetComponent", "No Target Component connected").ToString(), this);
	}

	// Check input pin type to make sure it's a supported type for touchengine

	UEdGraphPin* valuePin = FindPin(FTEInput_GetPinNames::GetPinNameValue());

	if (!CheckPinCategory(valuePin))
	{
		// pin type is not valid
		valuePin->BreakAllPinLinks();
		return;
	}

	// get the proper function from the library based on pin category
	UFunction* BlueprintFunction = UTouchBlueprintFunctionLibrary::FindSetterByType(
		valuePin->PinType.PinCategory, 
		valuePin->PinType.ContainerType == EPinContainerType::Array, 
		valuePin->PinType.PinSubCategoryObject.IsValid() ? valuePin->PinType.PinSubCategoryObject->GetFName() : FName("")
	);

	if (BlueprintFunction == NULL) {
		CompilerContext.MessageLog.Error(*LOCTEXT("InvalidFunctionName", "The function has not been found.").ToString(), this);
		return;
	}

	UK2Node_CallFunction* CallFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);

	CallFunction->SetFromFunction(BlueprintFunction);
	CallFunction->AllocateDefaultPins();
	CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallFunction, this);

	//Input
	CompilerContext.MovePinLinksToIntermediate(*FindPin(FTEInput_GetPinNames::GetPinNameVarName()), *CallFunction->FindPin(TEXT("VarName")));
	CompilerContext.MovePinLinksToIntermediate(*FindPin(FTEInput_GetPinNames::GetPinNameComponent()), *CallFunction->FindPin(TEXT("Target")));
	CompilerContext.MovePinLinksToIntermediate(*FindPin(FTEInput_GetPinNames::GetPinNameValue()), *CallFunction->FindPin(TEXT("value")));

	//Output
	CompilerContext.MovePinLinksToIntermediate(*FindPin(FTEInput_GetPinNames::GetPinNameOutput()), *CallFunction->GetReturnValuePin());

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

void UTouchInputK2Node::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	Super::GetMenuActions(ActionRegistrar);

	UClass* Action = GetClass();

	if (ActionRegistrar.IsOpenForRegistration(Action)) {
		UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
		check(Spawner != nullptr);

		ActionRegistrar.AddBlueprintAction(Action, Spawner);
	}
}

void UTouchInputK2Node::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	Super::ReallocatePinsDuringReconstruction(OldPins);

	// This is necessary to retain type information after pasting or loading from disc
	if (UEdGraphPin* InputPin = FindPin(FTEInput_GetPinNames::GetPinNameValue()))
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

					auto Schema = GetSchema();
					Schema->RecombinePin(InputPin);

					break;
				}
			}
		}
	}
}

void UTouchInputK2Node::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	if (Pin->PinName == FTEInput_GetPinNames::GetPinNameValue())
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

bool UTouchInputK2Node::CheckPinCategory(UEdGraphPin* Pin)
{
	FName PinCategory = Pin->PinType.PinCategory;

	if (PinCategory == TEXT("float"))
	{
		return true;
	}
	else if (PinCategory == TEXT("int"))
	{
		return true;
	}
	else if (PinCategory == TEXT("int64"))
	{
		return true;
	}
	else if (PinCategory == TEXT("bool"))
	{
		return true;
	}
	else if (PinCategory == TEXT("name"))
	{
		return true;
	}
	else if (PinCategory == TEXT("object"))
	{
		if (Cast<UClass>(Pin->PinType.PinSubCategoryObject.Get())->IsChildOf<UTexture>() || UTexture::StaticClass()->IsChildOf(Cast<UClass>(Pin->PinType.PinSubCategoryObject.Get())))
		{
			return true;
		}

		return false;
	}
	else if (PinCategory == TEXT("class"))
	{
		return false;
	}
	else if (PinCategory == TEXT("byte"))
	{
		return true;
	}
	else if (PinCategory == TEXT("string"))
	{
		return true;
	}
	else if (PinCategory == TEXT("text"))
	{
		return true;
	}
	else if (PinCategory == TEXT("struct"))
	{
		if (Pin->PinType.PinSubCategoryObject.Get()->GetFName() == "Vector")
		{
			return true;
		}
		if (Pin->PinType.PinSubCategoryObject.Get()->GetFName() == "Vector4")
		{
			return true;
		}
		if (Pin->PinType.PinSubCategoryObject.Get()->GetFName() == "Color")
		{
			return true;
		}
	}
	else if (PinCategory == TEXT("enum"))
	{
		return true;
	}

	return false;
}

#undef LOCTEXT_NAMESPACE