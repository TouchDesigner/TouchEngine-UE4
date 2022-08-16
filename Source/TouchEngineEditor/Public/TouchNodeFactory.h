// Copyright © Derivative Inc. 2021

#pragma once

#include "CoreMinimal.h"

#include "EdGraphUtilities.h"

/**
 *
 */
class TOUCHENGINEEDITOR_API FTouchNodeFactory : public FGraphPanelNodeFactory
{
public:
	virtual ~FTouchNodeFactory() override;
	virtual TSharedPtr<class SGraphNode> CreateNode(class UEdGraphNode* Node) const override;
};
