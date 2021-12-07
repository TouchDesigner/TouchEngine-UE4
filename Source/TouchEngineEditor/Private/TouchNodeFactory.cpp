// Copyright © Derivative Inc. 2021


#include "TouchNodeFactory.h"
#include "TouchMegaK2Node.h"
#include "SGraphNodeK2InOut.h"
#include "SGraphNodeDefault.h"
#include "KismetNodes/SGraphNodeK2Default.h"

FTouchNodeFactory::~FTouchNodeFactory()
{
}

TSharedPtr<class SGraphNode> FTouchNodeFactory::CreateNode(UEdGraphNode* Node) const
{
    if (UTouchMegaK2Node* K2Node = Cast<UTouchMegaK2Node>(Node))
    {
        if (Node->GetClass()->ImplementsInterface(UK2Node_TouchAddPinInterface::StaticClass()))
        {
            return SNew(SGraphNodeK2InOut, CastChecked<UK2Node>(Node));
        }
        else
        {
            return SNew(SGraphNodeK2Default, K2Node);
        }
    }
    else
    {
        return SNew(SGraphNodeDefault)
            .GraphNodeObj(Node);
    }
}
