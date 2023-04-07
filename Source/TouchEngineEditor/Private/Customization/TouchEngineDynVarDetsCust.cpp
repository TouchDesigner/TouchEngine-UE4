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

#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Engine/TouchEngineSubsystem.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "IPropertyUtilities.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "TouchEngineEditorLog.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/STextComboBox.h"
#include "Engine/TouchVariables.h"

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
	FTouchEngineDynamicVariableContainer* DynVars = GetDynamicVariables();
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

void FTouchEngineDynamicVariableStructDetailsCustomization::GenerateInputVariables(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, const FText InTitle, const FString InPrefixFilter)
{
	IDetailGroup& InputGroup = StructBuilder.AddGroup(FName("Inputs"), InTitle);

	// handle input variables
	TSharedPtr<IPropertyHandleArray> InputsHandle = DynamicVariablePropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableContainer, DynVars_Input))->AsArray();
	uint32 NumInputs = 0u;
	InputsHandle->GetNumElements(NumInputs);

	for (uint32 i = 0; i < NumInputs; i++)
	{
		TSharedRef<IPropertyHandle> DynVarHandle = InputsHandle->GetElement(i);
		FTouchEngineDynamicVariableStruct* DynVar;

		TArray<void*> RawData;
		DynVarHandle->AccessRawData(RawData);

		if (RawData.Num() == 0 || !RawData[0])
		{
			continue;
		}
		DynVar = static_cast<FTouchEngineDynamicVariableStruct*>(RawData[0]);
		if (DynVar->VarName == TEXT("ERROR_NAME"))
		{
			continue;
		}

		if (!DynVar->VarName.StartsWith(InPrefixFilter))
		{
			continue; // Since TouchEngine stores both inputs and parameters under a single Input array, this filter is used to separate both for display
		}

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
						.OnCheckStateChanged_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleChecked, DynVar->VarIdentifier, DynVarHandle)
						.IsChecked_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsCheckState, DynVar->VarIdentifier)
					];
				break;
			}
		case EVarType::Int:
			{
				if (DynVar->Count == 1) //todo: Use of Count might be problematic if there is an array of one value, why not bIsArray?
				{
					FDetailWidgetRow& NewRow = InputGroup.AddWidgetRow();

					if (DynVar->VarIntent != EVarIntent::DropDown)
					{
						NewRow.NameContent()
							[
								CreateNameWidget(DynVar->VarLabel, DynVar->GetTooltip(), StructPropertyHandle)
							]
							.ValueContent()
							.MaxDesiredWidth(250)
							[
								SNew(SNumericEntryBox<int32>)
								.OnValueCommitted_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged<int32>, DynVar->VarIdentifier)
								.AllowSpin(false)
								.Value_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalInt, DynVar->VarIdentifier)
							];
					}
					else
					{
						TArray<TSharedPtr<FString>>* DropDownStrings = new TArray<TSharedPtr<FString>>(); //todo: new TArray? Is this ever deleted?

						TArray<FString> Keys;
						DynVar->DropDownData.GetKeys(Keys);
						for (int j = 0; j < Keys.Num(); j++)
						{
							DropDownStrings->Add(MakeShared<FString>(Keys[j]));
						}
						int32 SelectedItem = DynVar->GetValueAsInt(); // The Index

						NewRow.NameContent()
							[
								CreateNameWidget(DynVar->VarLabel, DynVar->GetTooltip(), StructPropertyHandle)
							]
							.ValueContent()
							.MaxDesiredWidth(250)
							[
								SNew(STextComboBox)
								.OptionsSource(DropDownStrings)
								.InitiallySelectedItem(DropDownStrings->IsValidIndex(SelectedItem) ? (*DropDownStrings)[SelectedItem] : TSharedPtr<FString>())
								.OnSelectionChanged_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleDropDownBoxValueChanged, DynVar->VarIdentifier)
							];
					}
				}
				else
				{
					switch (DynVar->Count)
					{
					case 2:
						{
							TSharedPtr<IPropertyHandle> IntVector2DHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, IntPointProperty));
							const FSimpleDelegate OnChangedDelegate = FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
							                                                                     DynVar->VarIdentifier,
							                                                                     FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar) { DynVar.HandleIntVector2Changed(); })
							);
							IntVector2DHandle->SetOnPropertyValueChanged(OnChangedDelegate);
							IntVector2DHandle->SetOnChildPropertyValueChanged(OnChangedDelegate);

							InputGroup.AddPropertyRow(IntVector2DHandle.ToSharedRef())
							          .ToolTip(DynVar->GetTooltip())
							          .DisplayName(FText::FromString(DynVar->VarLabel));
							break;
						}
					case 3:
						{
							TSharedPtr<IPropertyHandle> IntVectorHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, IntVectorProperty));
							const FSimpleDelegate OnChangedDelegate = FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
							                                                                     DynVar->VarIdentifier,
							                                                                     FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar) { DynVar.HandleIntVectorChanged(); })
							);
							IntVectorHandle->SetOnPropertyValueChanged(OnChangedDelegate);
							IntVectorHandle->SetOnChildPropertyValueChanged(OnChangedDelegate);

							InputGroup.AddPropertyRow(IntVectorHandle.ToSharedRef())
							          .ToolTip(DynVar->GetTooltip())
							          .DisplayName(FText::FromString(DynVar->VarLabel));
							break;
						}
					case 4:
						{
							TSharedPtr<IPropertyHandle> IntVector4Handle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, IntVector4Property));
							const FSimpleDelegate OnChangedDelegate = FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
							                                                                     DynVar->VarIdentifier,
							                                                                     FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar) { DynVar.HandleIntVector4Changed(); })
							);
							IntVector4Handle->SetOnPropertyValueChanged(OnChangedDelegate);
							IntVector4Handle->SetOnChildPropertyValueChanged(OnChangedDelegate);

							IDetailPropertyRow& Property = InputGroup.AddPropertyRow(IntVector4Handle.ToSharedRef());
							Property.ToolTip(DynVar->GetTooltip());
							Property.DisplayName(FText::FromString(DynVar->VarLabel));

							break;
						}
					default:
						{
							for (int j = 0; j < DynVar->Count; j++)
							{
								FDetailWidgetRow& NewRow = InputGroup.AddWidgetRow();

								NewRow.NameContent()
									[
										CreateNameWidget((FString(DynVar->VarLabel).Append(FString::FromInt(j + 1))), DynVar->GetTooltip(), StructPropertyHandle)
									]
									.ValueContent()
									.MaxDesiredWidth(250)
									[
										SNew(SNumericEntryBox<int>)
										.OnValueCommitted(SNumericEntryBox<int>::FOnValueCommitted::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChangedWithIndex<int>, j, DynVar->VarIdentifier))
										.AllowSpin(false)
										.Value(TAttribute<TOptional<int>>::Create(TAttribute<TOptional<int>>::FGetter::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetIndexedValueAsOptionalInt, j, DynVar->VarIdentifier)))
									];
							}
							break;
						}
					}
				}
				break;
			}
		case EVarType::Double:
			{
				if (DynVar->Count == 1) //todo: Use of Count might be problematic if there is an array of one value, why not bIsArray?
				{
					FDetailWidgetRow& NewRow = InputGroup.AddWidgetRow();

					NewRow.NameContent()
						[
							CreateNameWidget(DynVar->VarLabel, DynVar->GetTooltip(), StructPropertyHandle)
						]
						.ValueContent()
						.MaxDesiredWidth(250)
						[
							SNew(SNumericEntryBox<double>)
							.OnValueCommitted_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged<double>, DynVar->VarIdentifier)
							.AllowSpin(false)
							.Value_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalDouble, DynVar->VarIdentifier)
						];
				}
				else
				{
					switch (DynVar->VarIntent)
					{
					case EVarIntent::Color:
						{
							TSharedPtr<IPropertyHandle> ColorHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, ColorProperty));
							const FSimpleDelegate OnValueChanged = FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
							                                                                  DynVar->VarIdentifier,
							                                                                  FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar) { DynVar.HandleColorChanged(); })
							);
							ColorHandle->SetOnPropertyValueChanged(OnValueChanged);
							ColorHandle->SetOnChildPropertyValueChanged(OnValueChanged);

							InputGroup.AddPropertyRow(ColorHandle.ToSharedRef())
							          .ToolTip(DynVar->GetTooltip())
							          .DisplayName(FText::FromString(DynVar->VarLabel));
							break;
						}
					default:
						{
							switch (DynVar->Count)
							{
							case 2:
								{
									TSharedPtr<IPropertyHandle> Vector2DHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, Vector2DProperty));
									const FSimpleDelegate OnValueChangedDelegate = FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
									                                                                          DynVar->VarIdentifier,
									                                                                          FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar) { DynVar.HandleVector2Changed(); })
									);
									Vector2DHandle->SetOnPropertyValueChanged(OnValueChangedDelegate);
									Vector2DHandle->SetOnChildPropertyValueChanged(OnValueChangedDelegate);

									IDetailPropertyRow& Property = InputGroup.AddPropertyRow(Vector2DHandle.ToSharedRef());
									Property.ToolTip(DynVar->GetTooltip());
									Property.DisplayName(FText::FromString(DynVar->VarLabel));
									break;
								}
							case 3:
								{
									TSharedPtr<IPropertyHandle> VectorHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, VectorProperty));
									const FSimpleDelegate OnValueChangedDelegate = FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
									                                                                          DynVar->VarIdentifier,
									                                                                          FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar) { DynVar.HandleVectorChanged(); })
									);
									VectorHandle->SetOnPropertyValueChanged(OnValueChangedDelegate);
									VectorHandle->SetOnChildPropertyValueChanged(OnValueChangedDelegate);

									IDetailPropertyRow& Property = InputGroup.AddPropertyRow(VectorHandle.ToSharedRef());
									Property.ToolTip(DynVar->GetTooltip());
									Property.DisplayName(FText::FromString(DynVar->VarLabel));
									break;
								}
							case 4:
								{
									TSharedPtr<IPropertyHandle> Vector4Handle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, Vector4Property));
									const FSimpleDelegate OnValueChangedDelegate = FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
									                                                                          DynVar->VarIdentifier,
									                                                                          FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar) { DynVar.HandleVector4Changed(); })
									);
									Vector4Handle->SetOnPropertyValueChanged(OnValueChangedDelegate);
									Vector4Handle->SetOnChildPropertyValueChanged(OnValueChangedDelegate);

									IDetailPropertyRow& Property = InputGroup.AddPropertyRow(Vector4Handle.ToSharedRef());
									Property.ToolTip(DynVar->GetTooltip());
									Property.DisplayName(FText::FromString(DynVar->VarLabel));
									break;
								}

							default:
								{
									for (int j = 0; j < DynVar->Count; j++)
									{
										FDetailWidgetRow& NewRow = InputGroup.AddWidgetRow();

										NewRow.NameContent()
											[
												CreateNameWidget((FString(DynVar->VarLabel).Append(FString::FromInt(j + 1))), DynVar->GetTooltip(), StructPropertyHandle)
											]
											.ValueContent()
											.MaxDesiredWidth(250)
											[
												SNew(SNumericEntryBox<double>)
													.OnValueCommitted(SNumericEntryBox<double>::FOnValueCommitted::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChangedWithIndex, j, DynVar->VarIdentifier))
													.AllowSpin(false)
													.Value(TAttribute<TOptional<double>>::Create(TAttribute<TOptional<double>>::FGetter::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetIndexedValueAsOptionalDouble, j, DynVar->VarIdentifier)))
											];
									}
									break;
								}
							}
							break;
						}
					}
				}
				break;
			}
		case EVarType::Float:
			InputGroup.AddWidgetRow()
			          .NameContent()
				[
					CreateNameWidget(DynVar->VarLabel, DynVar->GetTooltip(), StructPropertyHandle)
				]
				.ValueContent()
				[
					SNew(SNumericEntryBox<float>)
					.OnValueCommitted_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged<float>, DynVar->VarIdentifier)
					.AllowSpin(false)
					.Value_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalFloat, DynVar->VarIdentifier)
				];
			break;
		case EVarType::CHOP:
			{
				const TSharedPtr<IPropertyHandle> FloatsHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, FloatBufferProperty));

				const FSimpleDelegate OnValueChanged = FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
				                                                                  DynVar->VarIdentifier,
				                                                                  FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar) { DynVar.HandleFloatBufferChanged(); })
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
					FDetailWidgetRow& NewRow = InputGroup.AddWidgetRow();

					if (DynVar->VarIntent != EVarIntent::DropDown)
					{
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
								// .ForegroundColor(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxForegroundColor) //todo: what was this for?
								.OnTextCommitted_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextCommitted, DynVar->VarIdentifier)
								.SelectAllTextOnCommit(true)
								.Text_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxText, DynVar->VarIdentifier)
							];
					}
					else
					{
						TArray<TSharedPtr<FString>>* DropDownStrings = new TArray<TSharedPtr<FString>>(); //todo: new TArray? Is this ever deleted?

						TArray<FString> Keys;
						FString SelectedValue = DynVar->GetValueAsString(); // The Index
						TSharedPtr<FString> SelectedItem; // The selected Item

						DynVar->DropDownData.GetKeys(Keys);
						for (int j = 0; j < Keys.Num(); j++)
						{
							const TSharedRef<FString> Item = MakeShared<FString>(Keys[j]);
							DropDownStrings->Add(Item);
							if (Keys[j] == SelectedValue)
							{
								SelectedItem = Item;
							}
						}

						NewRow.NameContent()
							[
								CreateNameWidget(DynVar->VarLabel, DynVar->GetTooltip(), StructPropertyHandle)
							]
							.ValueContent()
							[
								SNew(STextComboBox)
								.OptionsSource(DropDownStrings)
								.InitiallySelectedItem(SelectedItem)
								.OnSelectionChanged_Raw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleDropDownBoxValueChanged, DynVar->VarIdentifier)
							];
					}
				}
				else
				{
					TSharedPtr<IPropertyHandle> StringHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, StringArrayProperty));

					const FSimpleDelegate OnValueChanged = FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
					                                                                  DynVar->VarIdentifier,
					                                                                  FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar) { DynVar.HandleStringArrayChanged(); })
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
					                           FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar) { DynVar.HandleTextureChanged(); })
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
					.AllowedClass(UTexture2D::StaticClass())
					.OnShouldFilterAsset(FOnShouldFilterAsset::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::OnShouldFilterTexture));

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

bool FTouchEngineDynamicVariableStructDetailsCustomization::OnShouldFilterTexture(const FAssetData& AssetData) const
{
	UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
	// Don't filter in case the SubSystem doesn't exist.
	
	if (!TESubsystem)
	{
		return false;
	}
	
	if (const UTexture2D* Texture = Cast<UTexture2D>(AssetData.GetAsset()))
	{
		return !TESubsystem->IsSupportedPixelFormat(Texture->GetPixelFormat());
	}

	return false;
}

void FTouchEngineDynamicVariableStructDetailsCustomization::GenerateOutputVariables(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder)
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

		// FDetailWidgetRow& NewRow = OutputGroup.AddWidgetRow();

		switch (DynVar->VarType)
		{
		case EVarType::CHOP:
			{
				TSharedPtr<STextBlock> TextBlock;
				FDetailWidgetRow& NewRow = OutputGroup.AddWidgetRow();
				NewRow.NameContent()
				[
					CreateNameWidget(DynVar->VarLabel, DynVar->GetTooltip(), StructPropertyHandle)
				]
				.ValueContent()
				.MaxDesiredWidth(250)
				[
					SAssignNew(TextBlock, STextBlock)
					.Text(FText::Format(LOCTEXT("CHOPAtRunTime", "CHOP [{0} Channels, {1} Samples]"),
						DynVar->CHOPProperty.GetNumChannels(),
						DynVar->CHOPProperty.GetNumSamples()))
				];
				
				// const TSharedPtr<IPropertyHandle> CHOPHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, CHOPProperty));
				//
				// CHOPHandle->SetPropertyDisplayName(FText::FromString(DynVar->VarLabel));
				// CHOPHandle->SetToolTipText(DynVar->GetTooltip());
				//
				// OutputGroup.AddPropertyRow(CHOPHandle.ToSharedRef()).IsEnabled(false);
				break;
			}
		case EVarType::Texture:
			{
				const TSharedPtr<IPropertyHandle> TextureHandle = DynVarHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTouchEngineDynamicVariableStruct, TextureProperty));
				TextureHandle->SetOnPropertyValueChanged(
					FSimpleDelegate::CreateRaw(this, &FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged,
						DynVar->VarIdentifier,
						FValueChangedCallback::CreateLambda([](FTouchEngineDynamicVariableStruct& DynVar){ DynVar.HandleTextureChanged(); })
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

FSlateColor FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxForegroundColor() const
{
	const FName InvertedForegroundName("InvertedForeground");
	return FAppStyle::GetSlateColor(InvertedForegroundName);
}

void FTouchEngineDynamicVariableStructDetailsCustomization::OnGenerateArrayChild(TSharedRef<IPropertyHandle> ElementHandle, int32 ChildIndex, IDetailChildrenBuilder& ChildrenBuilder)
{
	DynamicVariablePropertyHandle->NotifyPreChange();

	FDetailWidgetRow& ElementRow = ChildrenBuilder.AddCustomRow(FText::Format(LOCTEXT("RowNum", "Row{i}"), ChildIndex));

	ElementRow
		.NameContent()
		[
			ElementHandle->CreatePropertyNameWidget()
		]
	.ValueContent()
		.MaxDesiredWidth(0.f)
		.MinDesiredWidth(125.f)
		[
			ElementHandle->CreatePropertyValueWidget()
		]
	;

	DynamicVariablePropertyHandle->NotifyPostChange(EPropertyChangeType::ArrayAdd);
}

TSharedRef<SWidget> FTouchEngineDynamicVariableStructDetailsCustomization::CreateNameWidget(const FString& Name, const FText& Tooltip, TSharedRef<IPropertyHandle> StructPropertyHandle)
{
	return StructPropertyHandle->CreatePropertyNameWidget(FText::FromString(Name), Tooltip);
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleChecked(ECheckBoxState InState, FString Identifier, TSharedRef<IPropertyHandle> DynVarHandle)
{
	FTouchEngineDynamicVariableContainer* DynVars = GetDynamicVariables();
	if (!ensure(DynVars) || !TouchEngineComponent.IsValid())
	{
		return;
	}

	if (FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier))
	{
		DynVarHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct OldValue;
		OldValue.Copy(DynVar);
		DynVar->HandleChecked(InState);
		UpdateDynVarInstances(TouchEngineComponent.Get(), OldValue, *DynVar);
		if (TouchEngineComponent->EngineInfo && TouchEngineComponent->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(TouchEngineComponent->EngineInfo);
		}

		DynVarHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxTextCommitted(const FText& NewText, ETextCommit::Type CommitInfo, FString Identifier)
{
	FTouchEngineDynamicVariableContainer* DynVars = GetDynamicVariables();
	if (!ensure(DynVars) || !TouchEngineComponent.IsValid())
	{
		return;
	}

	if (FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier))
	{
		DynamicVariablePropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct OldValue; OldValue.Copy(DynVar);
		DynVar->HandleTextBoxTextCommitted(NewText);
		UpdateDynVarInstances(TouchEngineComponent.Get(), OldValue, *DynVar);

		if (TouchEngineComponent->EngineInfo && TouchEngineComponent->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(TouchEngineComponent->EngineInfo);
		}

		DynamicVariablePropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleValueChanged(FString Identifier, FValueChangedCallback UpdateValueFunc)
{
	FTouchEngineDynamicVariableContainer* DynVars = GetDynamicVariables();
	if (!ensure(DynVars) || !TouchEngineComponent.IsValid())
	{
		return;
	}

	if (FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier))
	{
		DynamicVariablePropertyHandle->NotifyPreChange(); //todo: the value has already changed here

		FTouchEngineDynamicVariableStruct OldValue;
		OldValue.Copy(DynVar); //todo: the value has already been updated at this point, this code should be reviewed
		UpdateValueFunc.Execute(*DynVar);
		UpdateDynVarInstances(TouchEngineComponent.Get(), OldValue, *DynVar);

		DynamicVariablePropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);

		if (TouchEngineComponent->EngineInfo && TouchEngineComponent->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(TouchEngineComponent->EngineInfo);
		}
	}
}

void FTouchEngineDynamicVariableStructDetailsCustomization::HandleDropDownBoxValueChanged(TSharedPtr<FString> Arg, ESelectInfo::Type SelectType, FString Identifier)
{
	FTouchEngineDynamicVariableContainer* DynVars = GetDynamicVariables();
	if (!ensure(DynVars) || !TouchEngineComponent.IsValid())
	{
		return;
	}

	if (FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier))
	{
		DynamicVariablePropertyHandle->NotifyPreChange();

		FTouchEngineDynamicVariableStruct OldValue; OldValue.Copy(DynVar);
		DynVar->HandleDropDownBoxValueChanged(Arg);
		UpdateDynVarInstances(TouchEngineComponent.Get(), OldValue, *DynVar);

		if (TouchEngineComponent->EngineInfo && TouchEngineComponent->SendMode == ETouchEngineSendMode::OnAccess)
		{
			DynVar->SendInput(TouchEngineComponent->EngineInfo);
		}

		DynamicVariablePropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	}
}


ECheckBoxState FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsCheckState(FString Identifier) const
{
	FTouchEngineDynamicVariableContainer* DynVars = GetDynamicVariables();
	if (!ensure(DynVars))
	{
		return ECheckBoxState::Undetermined;
	}

	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);
	if (DynVar && DynVar->Value && DynVar->VarType == EVarType::Bool)
	{
		return DynVar->GetValueAsBool() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	else
	{
		return ECheckBoxState::Unchecked;
	}
}

TOptional<int> FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalInt(FString Identifier) const
{
	FTouchEngineDynamicVariableContainer* DynVars = GetDynamicVariables();
	if (!ensure(DynVars))
	{
		return {};
	}

	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);
	if (DynVar && DynVar->Value && DynVar->VarType == EVarType::Int)
	{
		return TOptional<int>(DynVar->GetValueAsInt());
	}
	else
	{
		return TOptional<int>(0);
	}
}

TOptional<int> FTouchEngineDynamicVariableStructDetailsCustomization::GetIndexedValueAsOptionalInt(int Index, FString Identifier) const
{
	FTouchEngineDynamicVariableContainer* DynVars = GetDynamicVariables();
	if (!ensure(DynVars))
	{
		return {};
	}

	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);
	if (DynVar && DynVar->Value && DynVar->VarType == EVarType::Int)
	{
		return TOptional<int>(DynVar->GetValueAsIntIndexed(Index));
	}
	else
	{
		return TOptional<int>(0);
	}
}

TOptional<double> FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalDouble(FString Identifier) const
{
	FTouchEngineDynamicVariableContainer* DynVars = GetDynamicVariables();
	if (!ensure(DynVars))
	{
		return {};
	}

	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);
	if (DynVar && DynVar->Value && DynVar->VarType == EVarType::Double)
	{
		return TOptional<double>(DynVar->GetValueAsDouble());
	}
	else
	{
		return TOptional<double>(0);
	}
}

TOptional<double> FTouchEngineDynamicVariableStructDetailsCustomization::GetIndexedValueAsOptionalDouble(int Index, FString Identifier) const
{
	FTouchEngineDynamicVariableContainer* DynVars = GetDynamicVariables();
	if (!ensure(DynVars))
	{
		return {};
	}

	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);
	if (DynVar && DynVar->Value && DynVar->VarType == EVarType::Double)
	{
		return TOptional<double>(DynVar->GetValueAsDoubleIndexed(Index));
	}
	else
	{
		return TOptional<double>(0);
	}
}

TOptional<float> FTouchEngineDynamicVariableStructDetailsCustomization::GetValueAsOptionalFloat(FString Identifier) const
{
	FTouchEngineDynamicVariableContainer* DynVars = GetDynamicVariables();
	if (!ensure(DynVars))
	{
		return {};
	}

	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);
	if (DynVar && DynVar->Value && DynVar->VarType == EVarType::Float)
	{
		return TOptional<float>(DynVar->GetValueAsFloat());
	}
	else
	{
		return  TOptional<float>(0);
	}
}

FText FTouchEngineDynamicVariableStructDetailsCustomization::HandleTextBoxText(FString Identifier) const
{
	FTouchEngineDynamicVariableContainer* DynVars = GetDynamicVariables();
	if (!ensure(DynVars))
	{
		return {};
	}

	FTouchEngineDynamicVariableStruct* DynVar = DynVars->GetDynamicVariableByIdentifier(Identifier);
	if (DynVar && DynVar->Value && DynVar->VarType == EVarType::String)
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
	TArray<UObject*> ArchetypeInstances;
	TArray<UTouchEngineComponentBase*> UpdatedInstances;

	if (ParentComponent->HasAnyFlags(RF_ArchetypeObject))
	{
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
	TSharedPtr<IPropertyUtilities> PinnedPropertyUtilities = PropUtils.Pin();
	if (PinnedPropertyUtilities.IsValid())
	{
		PinnedPropertyUtilities->EnqueueDeferredAction(FSimpleDelegate::CreateSP(PinnedPropertyUtilities.ToSharedRef(), &IPropertyUtilities::ForceRefresh));
	}
}

#undef LOCTEXT_NAMESPACE