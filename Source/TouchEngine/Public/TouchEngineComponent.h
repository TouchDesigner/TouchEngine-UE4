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
#include "Components/ActorComponent.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "Delegates/DelegateSignatureImpl.inl"
#include "TouchEngineComponent.generated.h"

class UTouchEngineInfo;


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnToxLoaded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnToxFailedLoad, FString, errorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSetInputs);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGetOutputs);

/*
* The different cook modes the TouchEngine component can run in
*/
UENUM(BlueprintType)
enum class ETouchEngineCookMode : uint8
{
	COOKMODE_SYNCHRONIZED = 0			UMETA(DisplayName = "Synchronized"),
	COOKMODE_DELAYEDSYNCHRONIZED = 1	UMETA(DisplayName = "Delayed Synchronized"),
	COOKMODE_INDEPENDENT = 2			UMETA(DisplayName = "Independent"),
	COOKMODE_MAX
};

/*
* The different times the TouchEngine component will set / get variables from the TouchEngine instance
*/
UENUM(BlueprintType)
enum class ETouchEngineSendMode : uint8
{
	SENDMODE_EVERYFRAME = 0		UMETA(DisplayName = "Every Frame"),
	SENDMODE_ONACCESS = 1		UMETA(DisplayName = "On Access"),
	SENDMODE_MAX
};

/*
* Adds a TouchEngine instance to an object. 
*/
UCLASS(DefaultToInstanced, Blueprintable, meta = (DisplayName = "TouchEngine Component"))
class TOUCHENGINE_API UTouchEngineComponentBase : public UActorComponent
{
	GENERATED_BODY()

	friend class TouchEngineDynamicVariableStructDetailsCustomization;

public:	
	UTouchEngineComponentBase();
	~UTouchEngineComponentBase();

	// Our TouchEngine Info
	UPROPERTY()
	UTouchEngineInfo* EngineInfo;
	// Path to the Tox File to load
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "ToxFile"))
	FString ToxFilePath = "";
	// Mode for component to run in
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "ToxFile"))
	ETouchEngineCookMode cookMode = ETouchEngineCookMode::COOKMODE_INDEPENDENT;
	// Mode for the component to set and get variables 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "ToxFile"))
	ETouchEngineSendMode sendMode = ETouchEngineSendMode::SENDMODE_EVERYFRAME;
	// TouchEngine framerate
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Category = "ToxFile", DisplayName = "TE Frame Rate"))
	int64 TEFrameRate = 60;
	// Whether or not to start the TouchEngine immediately on begin play
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "ToxFile"))
	bool LoadOnBeginPlay = true;
	// Container for all dynamic variables
	UPROPERTY(EditAnywhere, meta = (NoResetToDefault, Category = "ToxFile"))
	FTouchEngineDynamicVariableContainer dynamicVariables;
	UPROPERTY()
	FString errorMessage;

protected:

	// delegate handles for the tox file either loading parameters successfully or failing
	FDelegateHandle paramsLoadedDelHandle, loadFailedDelHandle;
	// delegate handles for the call to begin frame and end frame
	FDelegateHandle beginFrameDelHandle, endFrameDelHandle;

	// Used to determine the time since last frame if we're cooking outside of TickComponent
	float cookTime = 0, lastCookTime = 0;

	// Called when the game starts
	virtual void BeginPlay() override;
	// Called at the beginning of a frame.
	void OnBeginFrame();
	// Called at the end of a frame.
	void OnEndFrame();
	// Called when a component is created. This may happen in editor for both blueprints and for world objects.
	virtual void OnComponentCreated() override;
	// Called when a component is destroyed. This may happen in editor for both blueprints and for world objects.
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy);
	// Called when a component is registered, after Scene is set, but before CreateRenderState_Concurrent or OnCreatePhysicsState are called.
	virtual void OnRegister() override;
	// Called when a component is unregistered.
	virtual void OnUnregister() override;
#if WITH_EDITORONLY_DATA
	// Called when a property on this object has been modified externally
	virtual void PostEditChangeProperty(FPropertyChangedEvent& e);
#endif
	// Attemps to grab the parameters from the TouchEngine engine subsytem. Should only be used for objects in blueprint.
	void LoadParameters();
	// Attempts to create an engine instance for this object. Should only be used for in world objects.
	void LoadTox();
	// Returns the absolute path of the stored ToxFilePath.
	FString GetAbsoluteToxPath();
	// Tells the dynamic variables to send their inputs
	void VarsSetInputs();
	// Tells the dynamic variables to get their outputs
	void VarsGetOutputs();

public:	

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	// Creates a TouchEngine instance for this object
	virtual void CreateEngineInfo();
	// Reloads the currently loaded tox file
	UFUNCTION(BlueprintCallable, meta = (Category = "ToxFile"))
	void ReloadTox();
	// Checks whether the component already has a tox file loaded
	bool IsLoaded();
	// Checks whether the component has failed to load a tox file
	bool HasFailedLoad();
	// Starts and creates the TouchEngine instnace
	UFUNCTION(BlueprintCallable)
	void StartTouchEngine();
	// Stops and deletes the TouchEngine instance
	UFUNCTION(BlueprintCallable)
	void StopTouchEngine();

	void UnbindDelegates();

	UFUNCTION(BlueprintCallable)
	bool IsRunning();

	/*
	virtual void OnToxLoaded();
	virtual void OnToxFailedLoad();
	*/

	/** Called when the component has been activated, with parameter indicating if it was from a reset */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation")
		FOnToxLoaded OnToxLoaded;

	/** Called when the component has been deactivated */
	UPROPERTY(BlueprintAssignable, Category = "Components|Activation")
		FOnToxFailedLoad OnToxFailedLoad;

	UPROPERTY(BlueprintAssignable, Category = "Components|Parameters")
		FSetInputs SetInputs;

	UPROPERTY(BlueprintAssignable, Category = "Components|Parameters")
		FGetOutputs GetOutputs;
};
