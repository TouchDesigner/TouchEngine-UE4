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

#pragma once

#include "CoreMinimal.h"
#include "TouchEngine/TETable.h"
#include "TouchVariables.generated.h"

class UTexture2D;

USTRUCT(BlueprintType, DisplayName = "Touch Engine CHOP Channel")
struct TOUCHENGINE_API FTouchEngineCHOPChannel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine")
	TArray<float> Values;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine")
	FString Name;

	FString ToString() const;

	bool operator==(const FTouchEngineCHOPChannel& Other) const;
	bool operator!=(const FTouchEngineCHOPChannel& Other) const;
};

USTRUCT(BlueprintType, DisplayName = "Touch Engine CHOP")
struct TOUCHENGINE_API FTouchEngineCHOP
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchEngine")
	TArray<FTouchEngineCHOPChannel> Channels;

	void Clear();

	TArray<float> GetCombinedValues() const;
	TArray<FString> GetChannelNames() const;

	bool GetChannelByName(const FString& InChannelName, FTouchEngineCHOPChannel& OutChannel);

	FString ToString() const;

	/**
	 * @brief An FTouchEngineCHOP is valid when there is at least one channel and all channels have the same number of values.
	 */
	bool IsValid() const;

	void SetChannelNames(TArray<FString> InChannelNames);

	bool operator==(const FTouchEngineCHOP& Other) const;
	bool operator!=(const FTouchEngineCHOP& Other) const;

	static FTouchEngineCHOP FromChannels(float** FullChannel, int InChannelCount, int InChannelCapacity, TArray<FString> InChannelNames);
	
	bool Serialize(FArchive& Ar);
};

struct FTouchDATFull
{
	TETable* ChannelData = nullptr;
	TArray<FString> RowNames;
	TArray<FString> ColumnNames;
};
