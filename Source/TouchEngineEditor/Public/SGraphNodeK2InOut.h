// Copyright © Derivative Inc. 2021

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Input/Reply.h"
#include "KismetNodes/SGraphNodeK2Base.h"

class SVerticalBox;
class UK2Node;

class SGraphNodeK2InOut : public SGraphNodeK2Base
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeK2InOut) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UK2Node* InNode);

protected:
	// SGraphNode interface
	virtual void CreateOutputSideAddButton(TSharedPtr<SVerticalBox> OutputBox) override;
	virtual FReply OnAddPin() override;
	virtual EVisibility IsAddPinButtonVisible() const override;
	// End of SGraphNode interface

	TSharedRef<SWidget> AddPinExtraButtonContext(FText PinText, FText PinTooltipText , bool bRightSide = true, FString DocumentationExcerpt = FString(), TSharedPtr<SToolTip> CustomTooltip = NULL);
	virtual FReply OnAddOutputPin();
};
