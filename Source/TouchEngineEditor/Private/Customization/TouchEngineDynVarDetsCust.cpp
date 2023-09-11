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

#include "TouchEngineDynVarDetsCust.h"
#include "Blueprint/TouchEngineComponent.h"
#include "TouchEngineDynamicVariableStruct.h"

// #include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Engine/TouchEngineSubsystem.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "IPropertyUtilities.h"
// #include "IStructureDetailsView.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
// #include "SDynamicPropertyEditorNumeric.h"
// #include "SResetToDefaultPropertyEditor.h"
#include "TouchEngineEditorLog.h"
// #include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
// #include "Widgets/Input/SNumericEntryBox.h"
// #include "Widgets/Input/STextComboBox.h"
#include "Engine/TouchVariables.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Colors/SColorThemes.h"

#define LOCTEXT_NAMESPACE "TouchEngineDynamicVariableDetailsCustomization"

FTouchEngineDynamicVariableStructDetailsCustomization::~FTouchEngineDynamicVariableStructDetailsCustomization()
{
	if (TouchEngineComponent.IsValid())
	{
		TouchEngineComponent->GetOnToxLoaded().RemoveAll(this);
		TouchEngineComponent->GetOnToxReset().RemoveAll(this);
		TouchEngineComponent->GetOnToxFailedLoad().RemoveAll(this);
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	DynamicVariablePropertyHandle = StructPropertyHandle;

	TArray<UObject*> CustomizedObjects;
	StructPropertyHandle->GetOuterObjects(CustomizedObjects);
	if (CustomizedObjects.IsEmpty() || CustomizedObjects.Num() > 1)
	{
		return;
	}

	TouchEngineComponent = Cast<UTouchEngineComponentBase>(CustomizedObjects[0]);
	const FTouchEngineDynamicVariableContainer* DynVars = GetDynamicVariables();
	if (!ensure(DynVars) || !TouchEngineComponent.IsValid())
	{
		UE_LOG(LogTouchEngineEditor, Warning, TEXT("FTouchEngineDynamicVariableContainer is designed to be contained by UTouchEngineComponentBase. Skipping customization..."));
		return;
	}
	
	// Note: We no longer call RemoveAll on the delegates here as FTouchEngineComponentCustomization (which also relies on these delegates) will have done it earlier.
	TouchEngineComponent->GetOnToxLoaded().AddSP(this, &FTouchEngineDynamicVariableStructDetailsCustomization::ToxLoaded);
	TouchEngineComponent->GetOnToxReset().AddSP(this, &FTouchEngineDynamicVariableStructDetailsCustomization::ToxReset);
	TouchEngineComponent->GetOnToxFailedLoad().AddSP(this, &FTouchEngineDynamicVariableStructDetailsCustomization::ToxFailedLoad);

	HeaderRow
		.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget(LOCTEXT("ToxParameters", "Component Settings"), LOCTEXT("InputOutput", "Input, Output, Parameter variables as read from the TOX file"))
		];
}

void FTouchEngineDynamicVariableStructDetailsCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	PropUtils = StructCustomizationUtils.GetPropertyUtilities();

	TArray<UObject*> Objs;
	StructPropertyHandle->GetOuterObjects(Objs);
	if (Objs.IsEmpty() || Objs.Num() > 1 || !ensure(TouchEngineComponent.IsValid() && DynamicVariablePropertyHandle))
	{
		StructBuilder.AddCustomRow(LOCTEXT("MultiSelectionInvalid", "Selection is invalid"))
			.ValueContent()
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Text(LOCTEXT("MultiSelectionInvalid_Text", "Tox Parameters does not support editing multiple objects"))
			];
	}
	else
	{
		DynamicVariablesData.Empty();
		GenerateInputVariables(StructPropertyHandle, StructBuilder, LOCTEXT("Parameters", "Parameters"), "p/");
		GenerateInputVariables(StructPropertyHandle, StructBuilder, LOCTEXT("Inputs", "Inputs"), "i/");
		GenerateOutputVariables(StructPropertyHandle, StructBuilder);
	}
}

FTouchEngineDynamicVariableContainer* FTouchEngineDynamicVariableStructDetailsCustomization::GetDynamicVariables() const
{
	if (DynamicVariablePropertyHandle->IsValidHandle())
	{
		TArray<void*> RawData;
		DynamicVariablePropertyHandle->AccessRawData(RawData);
		if (RawData.Num() == 1)
		{
			return static_cast<FTouchEngineDynamicVariableContainer*>(RawData[0]);
		}
	}
	return nullptr;
}

bool FTouchEngineDynamicVariableStructDetailsCustomization::GetDynamicVariableByIdentifier(const FString& Identifier, FTouchEngineDynamicVariableStruct*& DynVar, TSharedPtr<IPropertyHandle>& VarHandle) const
{
	FTouchEngineDynamicVariableContainer* DynVars = GetDynamicVariables();
	if (ensure(DynVars) && TouchEngineComponent.IsValid())
	{
		if (const FDynVarData* Data = DynamicVariablesData.Find(Identifier))
		{
			VarHandle = Data->DynVarHandle;
			DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);
			return VarHandle && DynVar;
		}
	}
	return false;
}

bool FTouchEngineDynamicVariableStructDetailsCustomization::GetDynamicVariableByIdentifierWeak(const TWeakPtr<FTouchEngineDynamicVariableStructDetailsCustomization>& ThisWeak, const FString& Identifier, FTouchEngineDynamicVariableStruct*& DynVar, TSharedPtr<IPropertyHandle>& VarHandle)
{
	if (const TSharedPtr<FTouchEngineDynamicVariableStructDetailsCustomization> SharedThis = ThisWeak.Pin())
	{
		return SharedThis->GetDynamicVariableByIdentifier(Identifier, DynVar, VarHandle);
	}
	return false;
}

void FTouchEngineDynamicVariableStructDetailsCustomization::GenerateInputVariables(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, const FText& InTitle, const FString& InPrefixFilter)
{
	IDetailGroup& InputGroup = StructBuilder.AddGroup(FName("Inputs"), InTitle);

	// handle input variables
	TSharedPtr<IPropertyHandleArray> InputsHandle = DynamicVariablePropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableContainer, DynVars_Input))->AsArray();
	uint32 NumInputs = 0u;
	InputsHandle->GetNumElements(NumInputs);
	
	for (uint32 i = 0; i < NumInputs; i++)
	{
		TSharedRef<IPropertyHandle> DynVarHandle = InputsHandle->GetElement(i);

		TArray<void*> RawData;
		DynVarHandle->AccessRawData(RawData);
		if (RawData.Num() == 0 || !RawData[0])
		{
			continue;
		}
		
		FTouchEngineDynamicVariableStruct* DynVar = static_cast<FTouchEngineDynamicVariableStruct*>(RawData[0]);
		if (DynVar->VarName == TEXT("ERROR_NAME"))
		{
			continue;
		}
		if (!DynVar->VarName.StartsWith(InPrefixFilter))
		{
			continue; // Since TouchEngine stores both inputs and parameters under a single Input array, this filter is used to separate both for display
		}
		TWeakPtr<FTouchEngineDynamicVariableStructDetailsCustomization> ThisWeak = SharedThis(this);
		FString& Identifier = DynVar->VarIdentifier;
		DynamicVariablesData.Add(Identifier, FDynVarData{Identifier, DynVarHandle});

		FResetToDefaultOverride ResetToDefault = FResetToDefaultOverride::Create(
			TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FTouchEngineDynamicVariableStructDetailsCustomization::IsResetToDefaultVisible, Identifier)),
			FSimpleDelegate::CreateSP(SharedThis(this), &FTouchEngineDynamicVariableStructDetailsCustomization::ResetToDefaultHandler, Identifier)
		);

		switch (DynVar->VarType)
		{
		case EVarType::Bool:
			{
				InputGroup.AddWidgetRow()
				          .NameContent()
					[
						CreateNameWidget(DynVar->VarLabel, DynVar->GetTooltip(), StructPropertyHandle)
					]
					.ValueContent()
					.MaxDesiredWidth(250)
					[
						SNew(SCheckBox)
						.OnCheckStateChanged(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleChecked, DynVar->VarIdentifier)
						.IsChecked_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsCheckState, DynVar->VarIdentifier)
					];
				break;
			}
		case EVarType::Int:
			{
				if (!DynVar->bIsArray)
				{
					if (DynVar->VarIntent != EVarIntent::DropDown)
					{
						FDetailWidgetRow& Row = GenerateNumericInputForProperty<int>(InputGroup, DynVarHandle, DynVar, ResetToDefault);
					}
					else
					{
						FDetailWidgetRow& Row = GenerateDropDownInputForProperty(InputGroup, DynVarHandle, DynVar, ResetToDefault);
					}
				}
				else
				{
					GenerateNumericArrayInputForProperty<int>(InputGroup, DynVarHandle, DynVar, ResetToDefault);
				}
				break;
			}
		case EVarType::Double:
			{
				if (!DynVar->bIsArray)
				{
					FDetailWidgetRow& Row = GenerateNumericInputForProperty<double>(InputGroup, DynVarHandle, DynVar, ResetToDefault);
				}
				else
				{
					switch (DynVar->VarIntent)
					{
					case EVarIntent::Color:
						{
							GenerateColorInputForProperty(InputGroup, DynVarHandle, DynVar, ResetToDefault);
							break;
						}
					default:
						{
							GenerateNumericArrayInputForProperty<double>(InputGroup, DynVarHandle, DynVar, ResetToDefault);
							break;
						}
					}
				}
				break;
			}
		case EVarType::Float:
			{
				if (!DynVar->bIsArray)
				{
					FDetailWidgetRow& Row = GenerateNumericInputForProperty<float>(InputGroup, DynVarHandle, DynVar, ResetToDefault);
				}
			}
		case EVarType::CHOP:
			{
				const TSharedPtr<IPropertyHandle> FloatsHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, FloatBufferProperty));

				const FSimpleDelegate OnValueChanged = FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
				                                                                  DynVar->VarIdentifier,
				                                                                  FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar, const UTouchEngineInfo* EngineInfo) { DynVar.HandleFloatBufferChanged(EngineInfo); })
				);
				FloatsHandle->SetOnPropertyValueChanged(OnValueChanged);
				FloatsHandle->SetOnChildPropertyValueChanged(OnValueChanged);

				InputGroup.AddPropertyRow(FloatsHandle.ToSharedRef())
				          .ToolTip(DynVar->GetTooltip())
				          .DisplayName(FText::FromString(DynVar->VarLabel));
				break;
			}
		case EVarType::String:
			{
				if (!DynVar->bIsArray)
				{
					if (DynVar->VarIntent != EVarIntent::DropDown)
					{
						FDetailWidgetRow& NewRow = InputGroup.AddWidgetRow();
						NewRow.NameContent()
							[
								CreateNameWidget(DynVar->VarLabel, DynVar->GetTooltip(), StructPropertyHandle)
							]
							.ValueContent()
							.MaxDesiredWidth(0.0f)
							.MinDesiredWidth(125.0f)
							[
								SNew(SEditableTextBox)
								.ClearKeyboardFocusOnCommit(false)
								.IsEnabled(true)
								.OnTextCommitted_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextCommitted, DynVar->VarIdentifier)
								.SelectAllTextOnCommit(true)
								.Text_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsFText, DynVar->VarIdentifier)
							];
					}
					else
					{
						FDetailWidgetRow& Row = GenerateDropDownInputForProperty(InputGroup, DynVarHandle, DynVar, ResetToDefault);
					}
				}
				else
				{
					TSharedPtr<IPropertyHandle> StringHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, StringArrayProperty));

					const FSimpleDelegate OnValueChanged = FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
					                                                                  DynVar->VarIdentifier,
					                                                                  FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar, const UTouchEngineInfo* EngineInfo) { DynVar.HandleStringArrayChanged(EngineInfo); })
					);
					StringHandle->SetOnPropertyValueChanged(OnValueChanged);
					StringHandle->SetOnChildPropertyValueChanged(OnValueChanged);

					InputGroup.AddPropertyRow(StringHandle.ToSharedRef())
					          .ToolTip(DynVar->GetTooltip())
					          .DisplayName(FText::FromString(DynVar->VarLabel));
				}
			}
			break;
		case EVarType::Texture:
			{
				FDetailWidgetRow& NewRow = InputGroup.AddWidgetRow();
				TSharedPtr<IPropertyHandle> TextureHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, TextureProperty));
				TextureHandle->SetOnPropertyValueChanged(
					FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
					                           DynVar->VarIdentifier,
					                           FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar, const UTouchEngineInfo* EngineInfo) { DynVar.HandleTextureChanged(EngineInfo); })
					)
				);

				// check for strange state world Property details panel can be in
				if (DynVar->TextureProperty == nullptr && DynVar->Value)
				{
					// value is set but texture Property is empty, set texture Property from value
					DynVar->TextureProperty = DynVar->GetValueAsTexture();
				}

				TSharedPtr<SObjectPropertyEntryBox> TextureSelector = SNew(SObjectPropertyEntryBox)
					.PropertyHandle(TextureHandle)
					.ThumbnailPool(PropUtils.Pin()->GetThumbnailPool())
					.AllowedClass(UTexture::StaticClass())
					.OnShouldFilterAsset(FOnShouldFilterAsset::CreateStatic(&FTouchEngineDynamicVariableStructDetailsCustomization::OnShouldFilterTexture));

				NewRow.NameContent()
					[
						CreateNameWidget(DynVar->VarLabel, DynVar->GetTooltip(), StructPropertyHandle)
					]
					.ValueContent()
					[
						TextureSelector.ToSharedRef()
					];
				break;
			}
		default:
			{
				break;
			}
		}
	}
}

bool FTouchEngineDynamicVariableStructDetailsCustomization::IsResetToDefaultVisible(const FString Identifier) const
{
	FTouchEngineDynamicVariableStruct* DynVar;
	TSharedPtr<IPropertyHandle> DynVarHandle;
	if (GetDynamicVariableByIdentifier(Identifier, DynVar, DynVarHandle))
	{
		return DynVar->CanResetToDefault();
	}
	return false;
}
bool FTouchEngineDynamicVariableStructDetailsCustomization::IsResetToDefaultVisible(const FString Identifier, int Index) const
{
	FTouchEngineDynamicVariableStruct* DynVar;
	TSharedPtr<IPropertyHandle> DynVarHandle;
	if (GetDynamicVariableByIdentifier(Identifier, DynVar, DynVarHandle))
	{
		return DynVar->CanResetToDefault(Index);
	}
	return false;
};

void FTouchEngineDynamicVariableStructDetailsCustomization::ResetToDefaultHandler(const FString Identifier)
{
	FTouchEngineDynamicVariableStruct* DynVar;
	TSharedPtr<IPropertyHandle> DynVarHandle;
	if (GetDynamicVariableByIdentifier(Identifier, DynVar, DynVarHandle))
	{
		if( DynVar->CanResetToDefault()) // needed to ensure we are not re-setting the same value
		{
			GEditor->BeginTransaction(FText::FromString(FString::Printf(TEXT("Reset to Default %s"), *DynVar->VarLabel)));
			DynVarHandle->NotifyPreChange();
			DynVar->ResetToDefault();
			DynVarHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
			GEditor->EndTransaction();
		}
	}
}
void FTouchEngineDynamicVariableStructDetailsCustomization::ResetToDefaultHandler(const FString Identifier, int Index)
{
	FTouchEngineDynamicVariableStruct* DynVar;
	TSharedPtr<IPropertyHandle> DynVarHandle;
	if (GetDynamicVariableByIdentifier(Identifier, DynVar, DynVarHandle))
	{
		if( DynVar->CanResetToDefault(Index)) // needed to ensure we are not re-setting the same value
		{
			GEditor->BeginTransaction(FText::FromString(FString::Printf(TEXT("Reset to Default %s"), *DynVar->VarLabel)));
			DynVarHandle->NotifyPreChange();
			DynVar->ResetToDefault(Index);
			DynVarHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
			GEditor->EndTransaction();
		}
	}
}

FDetailWidgetRow& FTouchEngineDynamicVariableStructDetailsCustomization::GenerateDropDownInputForProperty(IDetailGroup& DetailGroup, TSharedRef<IPropertyHandle>& VarHandle, FTouchEngineDynamicVariableStruct* DynVar, const FResetToDefaultOverride& ResetToDefault)
{
	TWeakPtr<FTouchEngineDynamicVariableStructDetailsCustomization> ThisWeak = SharedThis(this);
	FDetailWidgetRow& Row = DetailGroup.AddWidgetRow();
	Row.OverrideResetToDefault(ResetToDefault);
	
	Row.NameContent()
		[
			VarHandle->CreatePropertyNameWidget(FText::FromString(DynVar->VarLabel), DynVar->GetTooltip())
		]
		.ValueContent()
		[
			PropertyCustomizationHelpers::MakePropertyComboBox(
				nullptr,
				FOnGetPropertyComboBoxStrings::CreateLambda([ThisWeak, Identifier = DynVar->VarIdentifier](TArray<TSharedPtr<FString>>& OutComboBoxStrings, TArray<TSharedPtr<SToolTip>>& OutToolTips, TArray<bool>& OutRestrictedItems)
				{
					FTouchEngineDynamicVariableStruct* DynVar;
					TSharedPtr<IPropertyHandle> DynVarHandle;
					if (GetDynamicVariableByIdentifierWeak(ThisWeak, Identifier, DynVar, DynVarHandle))
					{
						for (const TPair<FString, int>& Data : DynVar->DropDownData)
						{
							OutComboBoxStrings.Add(MakeShared<FString>(Data.Key));
							OutRestrictedItems.Add(false);
							OutToolTips.Add(SNew(SToolTip).Text(FText::FromString(Data.Key)));
						}
					}
				}),
				FOnGetPropertyComboBoxValue::CreateLambda([ThisWeak, Identifier = DynVar->VarIdentifier]()
				{
					FTouchEngineDynamicVariableStruct* DynVar;
					TSharedPtr<IPropertyHandle> DynVarHandle;
					if (GetDynamicVariableByIdentifierWeak(ThisWeak, Identifier, DynVar, DynVarHandle))
					{
						return DynVar->GetValueAsString();
					}
					return FString();
				}),
				FOnPropertyComboBoxValueSelected::CreateLambda([ThisWeak, Identifier = DynVar->VarIdentifier](const FString& Value)
				{
					FTouchEngineDynamicVariableStruct* DynVar;
					TSharedPtr<IPropertyHandle> DynVarHandle;
					if (GetDynamicVariableByIdentifierWeak(ThisWeak, Identifier, DynVar, DynVarHandle))
					{
						if (DynVar->VarType == EVarType::Int)
						{
							if (const int32* Index = DynVar->DropDownData.Find(Value))
							{
								if (!DynVar->HasSameValueT(*Index))
								{
									const FTouchEngineDynamicVariableStruct PreviousValue = *DynVar;
									GEditor->BeginTransaction(FText::FromString(FString::Printf(TEXT("Edit %s"), *Identifier)));
									DynVarHandle->NotifyPreChange();
									DynVar->HandleValueChanged(*Index, ThisWeak.Pin()->TouchEngineComponent->EngineInfo);
									ThisWeak.Pin()->UpdateDynVarInstances(ThisWeak.Pin()->TouchEngineComponent.Get(), PreviousValue, *DynVar);
									DynVarHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
									GEditor->EndTransaction();
								}
							}
						}
						else if (DynVar->VarType == EVarType::String)
						{
							if (!DynVar->HasSameValueT(Value))
							{
								const FTouchEngineDynamicVariableStruct PreviousValue = *DynVar;
								GEditor->BeginTransaction(FText::FromString(FString::Printf(TEXT("Edit %s"), *Identifier)));
								DynVarHandle->NotifyPreChange();
								DynVar->HandleValueChanged(Value, ThisWeak.Pin()->TouchEngineComponent->EngineInfo);
								ThisWeak.Pin()->UpdateDynVarInstances(ThisWeak.Pin()->TouchEngineComponent.Get(), PreviousValue, *DynVar);
								DynVarHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
								GEditor->EndTransaction();
							}
						}
					}
				})
			)
		];

	return Row;
}

IDetailGroup& FTouchEngineDynamicVariableStructDetailsCustomization::GenerateColorInputForProperty(IDetailGroup& DetailGroup, TSharedRef<IPropertyHandle>& VarHandle, FTouchEngineDynamicVariableStruct* DynVar, const FResetToDefaultOverride& ResetToDefault)
{
	IDetailGroup& ColorGroup = GenerateNumericArrayInputForProperty<double>(DetailGroup, VarHandle, DynVar, ResetToDefault);

	TWeakPtr<FTouchEngineDynamicVariableStructDetailsCustomization> ThisWeak = SharedThis(this);
	auto OnGetColor = [ThisWeak, Identifier = DynVar->VarIdentifier]()
	{
		FTouchEngineDynamicVariableStruct* DynVar;
		TSharedPtr<IPropertyHandle> DynVarHandle;
		if (GetDynamicVariableByIdentifierWeak(ThisWeak, Identifier, DynVar, DynVarHandle))
		{
			return DynVar->GetValueAsLinearColor();
		}
		return FLinearColor();
	};

	TSharedPtr<SBorder> ColorWidgetBackgroundBorder;
	ColorGroup.HeaderRow()
	.NameWidget
	[
		VarHandle->CreatePropertyNameWidget(FText::FromString(DynVar->VarLabel), DynVar->GetTooltip())
	]
	.ValueContent() // copy from FColorStructCustomization and SPropertyEditorColor
	[
		SNew(SBox)
		.Padding(FMargin(0,0,4.0f,0.0f))
		.VAlign(VAlign_Center)
		[
			SAssignNew(ColorWidgetBackgroundBorder, SBorder)
			.Padding(1)
			.BorderImage(FAppStyle::Get().GetBrush("ColorPicker.RoundedSolidBackground"))
			.BorderBackgroundColor(FAppStyle::Get().GetSlateColor("Colors.InputOutline"))
			.VAlign(VAlign_Center)
		]
	]
	.OverrideResetToDefault(ResetToDefault);
	
	// declared after because we need to get a reference to the ColorWidgetBackgroundBorder 
	auto OnMouseDown = [ThisWeak, Identifier = DynVar->VarIdentifier, ParentWidget = ColorWidgetBackgroundBorder](const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		{
			return FReply::Unhandled();
		}
		
		FTouchEngineDynamicVariableStruct* DynVar;
		TSharedPtr<IPropertyHandle> DynVarHandle;
		if (GetDynamicVariableByIdentifierWeak(ThisWeak, Identifier, DynVar, DynVarHandle))
		{
			GEditor->BeginTransaction(FText::Format(LOCTEXT("SetColorProperty", "Edit {0}"), FText::FromString(DynVar->VarLabel)));
			FLinearColor InitialColor = DynVar->GetValueAsLinearColor();
			DynVarHandle->NotifyPreChange();
			
			bool bPreviousIniSRGBEnabled = false;
			if (FPaths::FileExists(GEditorPerProjectIni))
			{
				// this ends up overriding the setting we passed to the Color picker, so we cache it before setting it, and we reset it in OnColorPickerWindowClosed
				GConfig->GetBool(TEXT("ColorPickerUI"), TEXT("bSRGBEnabled"), bPreviousIniSRGBEnabled, GEditorPerProjectIni);
				GConfig->SetBool(TEXT("ColorPickerUI"), TEXT("bSRGBEnabled"), false, GEditorPerProjectIni);
			}

			FColorPickerArgs PickerArgs;
			{
				PickerArgs.bUseAlpha = DynVar->Count > 3;
				PickerArgs.bOnlyRefreshOnMouseUp = false;
				PickerArgs.bOnlyRefreshOnOk = false;
				PickerArgs.sRGBOverride = false;
				PickerArgs.DisplayGamma = TAttribute<float>::Create(TAttribute<float>::FGetter::CreateUObject(GEngine, &UEngine::GetDisplayGamma));
				PickerArgs.OnColorCommitted = FOnLinearColorValueChanged::CreateLambda([ThisWeak, Identifier](FLinearColor NewColor)
				{
					FTouchEngineDynamicVariableStruct* DynVar;
					TSharedPtr<IPropertyHandle> DynVarHandle;
					if (GetDynamicVariableByIdentifierWeak(ThisWeak, Identifier, DynVar, DynVarHandle))
					{
						NewColor = DynVar->GetClampedValue(NewColor);
						DynVar->HandleValueChanged(NewColor, ThisWeak.Pin()->TouchEngineComponent->EngineInfo); // to be sure, but not part of a transaction
					}
				});
				PickerArgs.OnColorPickerCancelled = FOnColorPickerCancelled::CreateLambda([ThisWeak, Identifier](FLinearColor OriginalColor)
				{
					FTouchEngineDynamicVariableStruct* DynVar;
					TSharedPtr<IPropertyHandle> DynVarHandle;
					if (GetDynamicVariableByIdentifierWeak(ThisWeak, Identifier, DynVar, DynVarHandle))
					{
						GEditor->CancelTransaction(0);
						DynVar->SetValue(OriginalColor);
					}
				});
				PickerArgs.OnColorPickerWindowClosed = FOnWindowClosed::CreateLambda([ThisWeak, Identifier, InitialColor, bPreviousIniSRGBEnabled](const TSharedRef<SWindow>& Window)
				{
					FTouchEngineDynamicVariableStruct* DynVar;
					TSharedPtr<IPropertyHandle> DynVarHandle;
					if (GetDynamicVariableByIdentifierWeak(ThisWeak, Identifier, DynVar, DynVarHandle))
					{
						if (DynVar->HasSameValueT(InitialColor))
						{
							GEditor->CancelTransaction(0);
						}
						else
						{
							FTouchEngineDynamicVariableStruct PreviousValue = *DynVar;
							PreviousValue.SetValue(InitialColor);
							ThisWeak.Pin()->UpdateDynVarInstances(ThisWeak.Pin()->TouchEngineComponent.Get(), PreviousValue, *DynVar);
							DynVarHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
							GEditor->EndTransaction();
						}
					}
					if (FPaths::FileExists(GEditorPerProjectIni)) // reset to the original value
					{
						GConfig->SetBool(TEXT("ColorPickerUI"), TEXT("bSRGBEnabled"), bPreviousIniSRGBEnabled, GEditorPerProjectIni);
					}
				});
				PickerArgs.InitialColor = InitialColor;
				PickerArgs.ParentWidget = ParentWidget;
				PickerArgs.OptionalOwningDetailsView = ParentWidget;
				FWidgetPath ParentWidgetPath;
				if (FSlateApplication::Get().FindPathToWidget(ParentWidget.ToSharedRef(), ParentWidgetPath))
				{
					PickerArgs.bOpenAsMenu = FSlateApplication::Get().FindMenuInWidgetPath(ParentWidgetPath).IsValid();
				}
			}
			OpenColorPicker(PickerArgs);
			return FReply::Handled();
		}
		return FReply::Unhandled();
	};

	ColorWidgetBackgroundBorder->SetContent(
		SNew(SHorizontalBox) // copy from SPropertyEditorColor
		+ SHorizontalBox::Slot()
		.FillWidth(1)
		[
			SNew(SColorBlock)
			.AlphaBackgroundBrush(FAppStyle::Get().GetBrush("ColorPicker.RoundedAlphaBackground"))
			.Color(TAttribute<FLinearColor>::CreateLambda(OnGetColor))
			.UseSRGB(false)
			.ShowBackgroundForAlpha(true)
			.AlphaDisplayMode(DynVar->Count < 4 ? EColorBlockAlphaDisplayMode::Ignore : EColorBlockAlphaDisplayMode::Combined)
			.OnMouseButtonDown(FPointerEventHandler::CreateLambda(OnMouseDown))
			.Size(FVector2D(20.0f, 20.0f))
			.CornerRadius(FVector4(4.0f,0.0f,0.0f,4.0f))
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1)
		[
			SNew(SColorBlock)
			.Color(TAttribute<FLinearColor>::CreateLambda(OnGetColor))
			.UseSRGB(false)
			.ShowBackgroundForAlpha(false)
			.AlphaDisplayMode(EColorBlockAlphaDisplayMode::Ignore)
			.OnMouseButtonDown(FPointerEventHandler::CreateLambda(OnMouseDown))
			.Size(FVector2D(20.0f, 20.0f))
			.CornerRadius(FVector4(0.0f,4.0f,4.0f,0.0f))
		]
	);

	return ColorGroup;
}

bool FTouchEngineDynamicVariableStructDetailsCustomization::OnShouldFilterTexture(const FAssetData& AssetData)
{
	const UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
	// Don't filter in case the SubSystem doesn't exist.
	
	if (!TESubsystem)
	{
		return false;
	}

	if (const UTexture2D* Texture = Cast<UTexture2D>(AssetData.GetAsset()))
	{
		return !TESubsystem->IsSupportedPixelFormat(Texture->GetPixelFormat());
	}
	if (const UTexture* Texture = Cast<UTexture>(AssetData.GetAsset()))
	{
		if (const FTextureRHIRef RHI = UE::TouchEngine::FTouchResourceProvider::GetStableRHIFromTexture(Texture))
		{
			return !TESubsystem->IsSupportedPixelFormat(RHI->GetFormat());
		}
	}

	return false;
}

void FTouchEngineDynamicVariableStructDetailsCustomization::GenerateOutputVariables(const TSharedRef<IPropertyHandle>& StructPropertyHandle, IDetailChildrenBuilder& StructBuilder)
{
	IDetailGroup& OutputGroup = StructBuilder.AddGroup(FName("Outputs"), LOCTEXT("Outputs", "Outputs"));

	// handle output variables
	const TSharedPtr<IPropertyHandleArray>& OutputsHandle = DynamicVariablePropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableContainer, DynVars_Output))->AsArray();
	uint32 NumOutputs = 0u;
	OutputsHandle->GetNumElements(NumOutputs);

	for (uint32 i = 0; i < NumOutputs; i++)
	{
		const TSharedRef<IPropertyHandle>& DynVarHandle = OutputsHandle->GetElement(i);
		FTouchEngineDynamicVariableStruct* DynVar;

		{
			TArray<void*> RawData;
			DynamicVariablePropertyHandle->AccessRawData(RawData);
			DynVarHandle->AccessRawData(RawData);
			DynVar = static_cast<FTouchEngineDynamicVariableStruct*>(RawData[0]);
			if (!DynVar)
			{
				// TODO DP: Add warning
				continue;
			}
		}
		
		FString& Identifier = DynVar->VarIdentifier;
		DynamicVariablesData.Add(Identifier, FDynVarData{Identifier, DynVarHandle});

		switch (DynVar->VarType)
		{
		case EVarType::CHOP:
			{
				const TSharedPtr<IPropertyHandle> CHOPHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, CHOPProperty));
				
				TSharedPtr<STextBlock> TextBlock;
				OutputGroup.AddPropertyRow(CHOPHandle.ToSharedRef()).IsEnabled(false)
				.CustomWidget()
				.NameContent()
				[
					CreateNameWidget(DynVar->VarLabel, DynVar->GetTooltip(), CHOPHandle.ToSharedRef())
				]
				.ValueContent()
				.MaxDesiredWidth(250)
				[
					SAssignNew(TextBlock, STextBlock)
					.Text(FText::Format(LOCTEXT("CHOPAtRunTime", "CHOP [{0} Channels, {1} Samples]"),
						DynVar->CHOPProperty.GetNumChannels(),
						DynVar->CHOPProperty.GetNumSamples()))
				];
				// these seem to only work for right click > Copy Display Name if set after creating the PropertyRow.
				CHOPHandle->SetPropertyDisplayName(FText::FromString(DynVar->VarLabel));
				CHOPHandle->SetToolTipText(DynVar->GetTooltip());
				break;
			}
		case EVarType::Texture:
			{
				const TSharedPtr<IPropertyHandle> TextureHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, TextureProperty));
				TextureHandle->SetOnPropertyValueChanged(
					FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
						DynVar->VarIdentifier,
						FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar, const UTouchEngineInfo* EngineInfo){ DynVar.HandleTextureChanged(EngineInfo); })
					)
				);

				const TSharedRef<SObjectPropertyEntryBox> TextureWidget = SNew(SObjectPropertyEntryBox)
					.OnShouldFilterAsset_Lambda([](FAssetData AssetData){ return true; })
					.ThumbnailPool(PropUtils.Pin()->GetThumbnailPool())
					.PropertyHandle(TextureHandle)
					.IsEnabled(false);
				
				OutputGroup.AddWidgetRow().NameContent()
					[
						CreateNameWidget(DynVar->VarLabel, DynVar->GetTooltip(), StructPropertyHandle)
					]
					.ValueContent()
					.MaxDesiredWidth(250)
					[
						TextureWidget
					];
				break;
			}
		// string data
		case EVarType::String:
			{
				OutputGroup.AddWidgetRow().NameContent()
					[
						CreateNameWidget(DynVar->VarLabel, DynVar->GetTooltip(), StructPropertyHandle)
					]
					.ValueContent()
					.MaxDesiredWidth(250)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("DATAtRunTime", "DAT data will be filled at runtime"))
					]
					;
				break;
			}
		default:
			checkNoEntry();
		}
	}
}

TSharedRef<IPropertyTypeCustomization> FTouchEngineDynamicVariableStructDetailsCustomization::MakeInstance()
{
	return MakeShareable(new FTouchEngineDynamicVariableStructDetailsCustomization());
}

void FTouchEngineDynamicVariableStructDetailsCustomization::ToxLoaded()
{
	ForceRefresh();
}

void FTouchEngineDynamicVariableStructDetailsCustomization::ToxReset()
{
	ForceRefresh();
}

void FTouchEngineDynamicVariableStructDetailsCustomization::ToxFailedLoad(const FString& Error)
{
	ErrorMessage = Error;
	if (TouchEngineComponent.IsValid())
	{
		TouchEngineComponent->ErrorMessage = Error;
	}

	ForceRefresh();
}

TSharedRef<SWidget> FTouchEngineDynamicVariableStructDetailsCustomization::CreateNameWidget(const FString& Name, const FText& Tooltip, const TSharedRef<IPropertyHandle>& StructPropertyHandle)
{
	return StructPropertyHandle->CreatePropertyNameWidget(FText::FromString(Name), Tooltip);
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleChecked(ECheckBoxState InState, FString Identifier)
{
	FTouchEngineDynamicVariableStruct* DynVar;
	TSharedPtr<IPropertyHandle> DynVarHandle;
	if (GetDynamicVariableByIdentifier(Identifier, DynVar, DynVarHandle))
	{
		DynVarHandle->NotifyPreChange();

		const FTouchEngineDynamicVariableStruct OldValue = *DynVar;
		DynVar->SetValue(InState == ECheckBoxState::Checked);
		DynVar->SetFrameLastUpdatedFromNextCookFrame(TouchEngineComponent->EngineInfo);
		UpdateDynVarInstances(TouchEngineComponent.Get(), OldValue, *DynVar);

		DynVarHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextCommitted(const FText& NewText, ETextCommit::Type CommitInfo, FString Identifier)
{
	FTouchEngineDynamicVariableStruct* DynVar;
	TSharedPtr<IPropertyHandle> DynVarHandle;
	if (GetDynamicVariableByIdentifier(Identifier, DynVar, DynVarHandle))
	{
		DynVarHandle->NotifyPreChange();

		const FTouchEngineDynamicVariableStruct OldValue = *DynVar;
		DynVar->HandleTextBoxTextCommitted(NewText, TouchEngineComponent->EngineInfo);
		UpdateDynVarInstances(TouchEngineComponent.Get(), OldValue, *DynVar);
		
		DynVarHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged(FString Identifier, FValueChangedCallback UpdateValueFunc)
{
	FTouchEngineDynamicVariableStruct* DynVar;
	TSharedPtr<IPropertyHandle> DynVarHandle;
	if (GetDynamicVariableByIdentifier(Identifier, DynVar, DynVarHandle))
	{
		DynVarHandle->NotifyPreChange(); //todo: the value has already changed here

		const FTouchEngineDynamicVariableStruct OldValue = *DynVar; //todo: the value has already been updated at this point, this code should be reviewed
		UpdateValueFunc.Execute(*DynVar, TouchEngineComponent->EngineInfo);
		UpdateDynVarInstances(TouchEngineComponent.Get(), OldValue, *DynVar);

		DynVarHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	}
}

ECheckBoxState FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsCheckState(FString Identifier) const
{
	FTouchEngineDynamicVariableStruct* DynVar;
	TSharedPtr<IPropertyHandle> DynVarHandle;
	if (GetDynamicVariableByIdentifier(Identifier, DynVar, DynVarHandle))
	{
		return DynVar->GetValueAsBool() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	return ECheckBoxState::Unchecked;
}

FText FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsFText(FString Identifier) const
{
	FTouchEngineDynamicVariableStruct* DynVar;
	TSharedPtr<IPropertyHandle> DynVarHandle;
	if (GetDynamicVariableByIdentifier(Identifier, DynVar, DynVarHandle))
	{
		return FText::FromString(DynVar->GetValueAsString());
	}
	else
	{
		return FText();
	}
}


void FTouchEngineDynamicVariableStructDetailsCustomization::UpdateDynVarInstances(UTouchEngineComponentBase* ParentComponent, const FTouchEngineDynamicVariableStruct& OldVar, const FTouchEngineDynamicVariableStruct& NewVar)
{
	const TArray<UTouchEngineComponentBase*> UpdatedInstances; //todo: not used?

	if (ParentComponent->HasAnyFlags(RF_ArchetypeObject))
	{
		TArray<UObject*> ArchetypeInstances;
		ParentComponent->GetArchetypeInstances(ArchetypeInstances);

		for (int32 InstanceIndex = 0; InstanceIndex < ArchetypeInstances.Num(); ++InstanceIndex)
		{
			UTouchEngineComponentBase* InstancedTEComponent = Cast<UTouchEngineComponentBase>(ArchetypeInstances[InstanceIndex]);
			if (InstancedTEComponent != nullptr && !UpdatedInstances.Contains(InstancedTEComponent))
			{
				if (InstancedTEComponent->GetFilePath() == ParentComponent->GetFilePath())
				{
					// find this variable inside the component
					FTouchEngineDynamicVariableStruct* DynVar = InstancedTEComponent->DynamicVariables.GetDynamicVariableByIdentifier(NewVar.VarIdentifier);

					// didn't find the variable, or had a variable type mismatch.
					// This is an odd case, should probably reload the tox file and try again
					if (!DynVar || DynVar->VarType != NewVar.VarType)
					{
						continue;
					}
					// check if the value is the default value
					if (OldVar.Identical(DynVar, 0))
					{
						// if it is, replace it
						DynVar->SetValue(&NewVar);
					}
				}
			}
		}
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::ForceRefresh()
{
	const TSharedPtr<IPropertyUtilities> PinnedPropertyUtilities = PropUtils.Pin();
	if (PinnedPropertyUtilities.IsValid())
	{
		PinnedPropertyUtilities->EnqueueDeferredAction(FSimpleDelegate::CreateSP(PinnedPropertyUtilities.ToSharedRef(), &IPropertyUtilities::ForceRefresh));
	}
}

#undef LOCTEXT_NAMESPACE