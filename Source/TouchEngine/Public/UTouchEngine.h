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
#include "Logging/MessageLog.h"
#include "TouchEngine/TouchEngine.h"
#include "UTouchEngine.generated.h"

class UTexture;
class UTexture2D;
class UTouchEngineInfo;
template <typename T> struct TTouchVar;
struct FTouchEngineDynamicVariableStruct;

namespace UE::TouchEngine
{
	class FTouchEngineResourceProvider;
}

struct FTouchCHOPSingleSample
{
	TArray<float>	ChannelData;
	FString ChannelName;
};

struct FTouchCHOPFull
{
	TArray<FTouchCHOPSingleSample> SampleData;
};

struct FTouchDATFull
{
	TETable* ChannelData = nullptr;
	TArray<FString> RowNames;
	TArray<FString> ColumnNames;
};

struct FTouchTOP
{
	UTexture2D*		Texture = nullptr;
	void* WrappedResource = nullptr;
};


DECLARE_MULTICAST_DELEGATE(FTouchOnLoadComplete);
DECLARE_MULTICAST_DELEGATE_OneParam(FTouchOnLoadFailed, const FString&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTouchOnParametersLoaded, const TArray<FTouchEngineDynamicVariableStruct>&, const TArray<FTouchEngineDynamicVariableStruct>&);
DECLARE_MULTICAST_DELEGATE(FTouchOnCookFinished);

UCLASS()
class TOUCHENGINE_API UTouchEngine : public UObject
{
	friend class UTouchEngineInfo;

	GENERATED_BODY()

public:
	virtual ~UTouchEngine() override;
	virtual void				BeginDestroy() override;

private:
	void						Clear();

public:
	void						Copy(UTouchEngine* Other);

	void						PreLoad();
	void						PreLoad(const FString& ToxPath);
	void						LoadTox(FString ToxPath);
	void						Unload();
	const FString&				GetToxPath() const;

	void						CookFrame(int64 FrameTime_Mill);

	bool						SetCookMode(bool IsIndependent);

	bool						SetFrameRate(int64 FrameRate);

	FTouchCHOPFull				GetCHOPOutputSingleSample(const FString& Identifier);
	void						SetCHOPInputSingleSample(const FString &Identifier, const FTouchCHOPSingleSample& CHOP);
	FTouchCHOPFull				GetCHOPOutputs(const FString& Identifier);
	void						SetCHOPInput(const FString& Identifier, const FTouchCHOPFull& CHOP);

	FTouchTOP					GetTOPOutput(const FString& Identifier);
	void						SetTOPInput(const FString& Identifier, UTexture* Texture);

	TTouchVar<bool>				GetBooleanOutput(const FString& Identifier);
	void						SetBooleanInput(const FString& Identifier, TTouchVar<bool>& Op);
	TTouchVar<double>			GetDoubleOutput(const FString& Identifier);
	void						SetDoubleInput(const FString& Identifier, TTouchVar<TArray<double>>& Op);
	TTouchVar<int32_t>			GetIntegerOutput(const FString& Identifier);
	void						SetIntegerInput(const FString& Identifier, TTouchVar<TArray<int32_t>>& Op);
	TTouchVar<TEString*>		GetStringOutput(const FString& Identifier);
	void						SetStringInput(const FString& Identifier, TTouchVar<char*>& Op);
	FTouchDATFull				GetTableOutput(const FString& Identifier);
	void						SetTableInput(const FString& Identifier, FTouchDATFull& Op);

	void						SetDidLoad();

	bool						GetDidLoad() const { return MyDidLoad; }

	bool						GetIsLoading() const;

	bool						GetFailedLoad() const { return MyFailedLoad; }

	FTouchOnLoadFailed OnLoadFailed;
	FTouchOnParametersLoaded OnParametersLoaded;
	FTouchOnCookFinished OnCookFinished;

	FString FailureMessage;

private:
	enum class EFinalClean
	{
		False,
		True
	};

	/**
	 * This won't call TEInstanceLoad to load the Tox file. It's only a pre-load,
	 * configuring the engine with the Tox file if the path to one is provided.
	 * If there's already an instantiated TouchEngine, only configure it with the
	 * new Tox file path.
	 *
	 * @param ToxPath	Absolute path to the tox file
	 * @param Caller	Name of the function calling this one, for error logging
	 */
	bool			InstantiateEngineWithToxFile(const FString& ToxPath, const char* Caller);

	static void		EventCallback(TEInstance* Instance, TEEvent Event, TEResult Result, int64_t StartTimeValue, int32_t StartTimeScale, int64_t EndTimeValue, int32_t EndTimeScale, void* Info);

	void			AddResult(const FString& Str, TEResult Result);
	void			AddError(const FString& Str);
	void			AddWarning(const FString& Str);

	void			OutputMessages();

	void			OutputResult(const FString& Str, TEResult Result);
	void			OutputError(const FString& Str);
	void			OutputWarning(const FString& Str);

	static void		CleanupTextures(EFinalClean FC);
	static void		LinkValueCallback(TEInstance* Instance, TELinkEvent Event, const char* Identifier, void* Info);
	void			LinkValueCallback(TEInstance* Instance, TELinkEvent Event, const char* Identifier);

	static TSharedPtr<UE::TouchEngine::FTouchEngineResourceProvider> GetResourceProvider();

	UPROPERTY()
	FString									MyToxPath;
	TouchObject<TEInstance>					MyTEInstance = nullptr;
	TSharedPtr<UE::TouchEngine::FTouchEngineResourceProvider> MyResourceProvider = nullptr;;

	TMap<FString, FTouchCHOPSingleSample>	MyCHOPSingleOutputs;
	TMap<FString, FTouchCHOPFull>			MyCHOPFullOutputs;
	FCriticalSection						MyTOPLock;
	TMap<FString, FTouchTOP>				MyTOPOutputs;

	FMessageLog								MyMessageLog = FMessageLog(TEXT("TouchEngine"));
	bool									MyLogOpened = false;
	FCriticalSection						MyMessageLock;
	TArray<FString>							MyErrors;
	TArray<FString>							MyWarnings;


	std::atomic<bool>						MyDidLoad = false;
	bool									MyFailedLoad = false;
	bool									MyCooking = false;
	TETimeMode								MyTimeMode = TETimeMode::TETimeInternal;
	float									MyFrameRate = 60.f;
	int64									MyTime = 0;

	//RHIType									MyRHIType = RHIType::Invalid;

	bool									MyConfiguredWithTox = false;
	bool									MyLoadCalled = false;
	int64									MyNumOutputTexturesQueued = 0, MyNumInputTexturesQueued = 0;

};
