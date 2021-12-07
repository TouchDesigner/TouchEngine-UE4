// Copyright © Derivative Inc. 2021


#include "SGraphNodeK2InOut.h"
#include "EdGraph/EdGraph.h"
#include "Widgets/SBoxPanel.h"
#include "GraphEditorSettings.h"
#include "K2Node_TouchAddPinInterface.h"
#include "K2Node.h"
#include "ScopedTransaction.h"
#include "IDocumentation.h"


void SGraphNodeK2InOut::Construct(const FArguments& InArgs, UK2Node* InNode)
{
	ensure(InNode == nullptr || InNode->GetClass()->ImplementsInterface(UK2Node_TouchAddPinInterface::StaticClass()));
	GraphNode = InNode;

	SetCursor(EMouseCursor::CardinalCross);

	UpdateGraphNode();
}

void SGraphNodeK2InOut::CreateOutputSideAddButton(TSharedPtr<SVerticalBox> OutputBox)
{
	TSharedRef<SWidget> AddPinButton = AddPinButtonContent(
		NSLOCTEXT("SequencerNode", "SequencerNodeAddPinButton", "Add input pin"),
		NSLOCTEXT("SequencerNode", "SequencerNodeAddPinButton_ToolTip", "Add new input pin"));

	TSharedRef<SWidget> AddOutputPinButton = AddPinExtraButtonContext(
		NSLOCTEXT("SequencerNode", "SequencerNodeAddOutputPinButton", "Add output pin"),
		NSLOCTEXT("SequencerNode", "SequencerNodeAddOutputPinButton_ToolTip", "Add new output pin"));

	FMargin AddPinPadding = Settings->GetOutputPinPadding();
	AddPinPadding.Top += 6.0f;

	OutputBox->AddSlot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Right)
		.Padding(AddPinPadding)
		[
			AddPinButton
		];

	OutputBox->AddSlot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Right)
		.Padding(AddPinPadding)
		[
			AddOutputPinButton
		];
}

FReply SGraphNodeK2InOut::OnAddPin()
{
	IK2Node_TouchAddPinInterface* AddPinNode = Cast<IK2Node_TouchAddPinInterface>(GraphNode);
	ensure(AddPinNode);
	if (AddPinNode && AddPinNode->CanAddPin())
	{
		FScopedTransaction Transaction(NSLOCTEXT("SequencerNode", "AddPinTransaction", "Add Pin"));

		AddPinNode->AddInputPin();
		UpdateGraphNode();
		GraphNode->GetGraph()->NotifyGraphChanged();
	}

	return FReply::Handled();
}

EVisibility SGraphNodeK2InOut::IsAddPinButtonVisible() const
{
	IK2Node_TouchAddPinInterface* AddPinNode = Cast<IK2Node_TouchAddPinInterface>(GraphNode);
	ensure(AddPinNode);
	return ((AddPinNode && AddPinNode->CanAddPin()) ? EVisibility::Visible : EVisibility::Collapsed);
}

TSharedRef<SWidget> SGraphNodeK2InOut::AddPinExtraButtonContext(FText PinText, FText PinTooltipText , bool bRightSide, FString DocumentationExcerpt, TSharedPtr<SToolTip> CustomTooltip)
{
	TSharedPtr<SWidget> ButtonContent;
	if (bRightSide)
	{
		SAssignNew(ButtonContent, SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			[
				SNew(STextBlock)
				.Text(PinText)
			.ColorAndOpacity(FLinearColor::White)
			]
		+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(7, 0, 0, 0)
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush(TEXT("PropertyWindow.Button_AddToArray")))
			];
	}
	else
	{
		SAssignNew(ButtonContent, SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 7, 0)
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush(TEXT("PropertyWindow.Button_AddToArray")))
			]
		+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			[
				SNew(STextBlock)
				.Text(PinText)
			.ColorAndOpacity(FLinearColor::White)
			];
	}

	TSharedPtr<SToolTip> Tooltip;

	if (CustomTooltip.IsValid())
	{
		Tooltip = CustomTooltip;
	}
	else if (!DocumentationExcerpt.IsEmpty())
	{
		Tooltip = IDocumentation::Get()->CreateToolTip(PinTooltipText, NULL, GraphNode->GetDocumentationLink(), DocumentationExcerpt);
	}

	TSharedRef<SButton> AddPinButton = SNew(SButton)
		.ContentPadding(0.0f)
		.ButtonStyle(FEditorStyle::Get(), "NoBorder")
		.OnClicked(this, &SGraphNodeK2InOut::OnAddOutpuPin)
		.IsEnabled(this, &SGraphNodeK2InOut::IsNodeEditable)
		.ToolTipText(PinTooltipText)
		.ToolTip(Tooltip)
		.Visibility(this, &SGraphNodeK2InOut::IsAddPinButtonVisible)
		[
			ButtonContent.ToSharedRef()
		];

	AddPinButton->SetCursor(EMouseCursor::Hand);

	return AddPinButton;
}

FReply SGraphNodeK2InOut::OnAddOutpuPin()
{
	IK2Node_TouchAddPinInterface* AddPinNode = Cast<IK2Node_TouchAddPinInterface>(GraphNode);
	ensure(AddPinNode);
	if (AddPinNode && AddPinNode->CanAddPin())
	{
		FScopedTransaction Transaction(NSLOCTEXT("SequencerNode", "AddPinTransaction", "Add Pin"));

		AddPinNode->AddOutputPin();
		UpdateGraphNode();
		GraphNode->GetGraph()->NotifyGraphChanged();
	}

	return FReply::Handled();
}
