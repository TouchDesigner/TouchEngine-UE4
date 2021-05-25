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

#include "TouchEngineComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TouchEngineSubsystem.h"
#include "TouchEngineInfo.h"
#include "Misc/CoreDelegates.h"
#include "Engine.h"

// Sets default values for this component's properties
UTouchEngineComponentBase::UTouchEngineComponentBase() : Super()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	LoadParameters();
}

UTouchEngineComponentBase::~UTouchEngineComponentBase()
{
	// Remove delegates if they're bound
	if (beginFrameDelHandle.IsValid())
	{
		FCoreDelegates::OnBeginFrame.Remove(beginFrameDelHandle);
	}
	if (endFrameDelHandle.IsValid())
	{
		FCoreDelegates::OnEndFrame.Remove(endFrameDelHandle);
	}

	UnbindDelegates();
}

// Called when the game starts
void UTouchEngineComponentBase::BeginPlay()
{
	Super::BeginPlay();

	if (LoadOnBeginPlay)
	{
		// Create engine instance
		LoadTox();
	}

	// Bind delegates based on cook mode
	switch (cookMode)
	{
	case ETouchEngineCookMode::COOKMODE_DELAYEDSYNCHRONIZED:
	case ETouchEngineCookMode::COOKMODE_SYNCHRONIZED:
		beginFrameDelHandle = FCoreDelegates::OnBeginFrame.AddUObject(this, &UTouchEngineComponentBase::OnBeginFrame);
		endFrameDelHandle = FCoreDelegates::OnEndFrame.AddUObject(this, &UTouchEngineComponentBase::OnEndFrame);
		break;
	case ETouchEngineCookMode::COOKMODE_INDEPENDENT:

		break;
	}

	// without this crash can happen if the details panel accidentally binds to a world object
	dynamicVariables.OnToxLoaded.Clear();
	dynamicVariables.OnToxFailedLoad.Clear();
}

void UTouchEngineComponentBase::OnBeginFrame()
{
	if (!EngineInfo || !EngineInfo->isLoaded())
	{
		// TouchEngine has not been started
		return;
	}


	switch (cookMode)
	{
	case ETouchEngineCookMode::COOKMODE_INDEPENDENT:

		break;
	case ETouchEngineCookMode::COOKMODE_DELAYEDSYNCHRONIZED:
	{
	}
	break;
	case ETouchEngineCookMode::COOKMODE_SYNCHRONIZED:

		// set cook time variables since we don't have delta time
		cookTime = UGameplayStatics::GetRealTimeSeconds(this);

		if (lastCookTime == 0)
			lastCookTime = cookTime;

		// start cook as early as possible
		dynamicVariables.SendInputs(EngineInfo);
		EngineInfo->cookFrame((cookTime - lastCookTime) * 10000);

		lastCookTime = cookTime;

		break;
	}
}

void UTouchEngineComponentBase::OnEndFrame()
{
	if (!EngineInfo || !EngineInfo->isLoaded())
	{
		// TouchEngine has not been started
		return;
	}

	switch (cookMode)
	{
	case ETouchEngineCookMode::COOKMODE_INDEPENDENT:

		break;
	case ETouchEngineCookMode::COOKMODE_DELAYEDSYNCHRONIZED:

		break;
	case ETouchEngineCookMode::COOKMODE_SYNCHRONIZED:

		break;
	}
}

void UTouchEngineComponentBase::OnComponentCreated()
{
	// Ensure we tick as early as possible
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	Super::OnComponentCreated();

	LoadParameters();
}

void UTouchEngineComponentBase::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	// Remove delegates if they're bound
	if (beginFrameDelHandle.IsValid())
	{
		FCoreDelegates::OnBeginFrame.Remove(beginFrameDelHandle);
	}
	if (endFrameDelHandle.IsValid())
	{
		FCoreDelegates::OnEndFrame.Remove(endFrameDelHandle);
	}
	if (paramsLoadedDelHandle.IsValid() && loadFailedDelHandle.IsValid())
	{
		UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		teSubsystem->UnbindDelegates(paramsLoadedDelHandle, loadFailedDelHandle);
	}

	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UTouchEngineComponentBase::OnRegister()
{
	// Ensure we tick as early as possible
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	Super::OnRegister();

	LoadParameters();
}

void UTouchEngineComponentBase::OnUnregister()
{
	// Remove delegates if they're bound
	if (beginFrameDelHandle.IsValid())
	{
		FCoreDelegates::OnBeginFrame.Remove(beginFrameDelHandle);
	}
	if (endFrameDelHandle.IsValid())
	{
		FCoreDelegates::OnEndFrame.Remove(endFrameDelHandle);
	}
	if (paramsLoadedDelHandle.IsValid() && loadFailedDelHandle.IsValid())
	{
		UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		teSubsystem->UnbindDelegates(paramsLoadedDelHandle, loadFailedDelHandle);
	}

	Super::OnUnregister();
}

#if WITH_EDITORONLY_DATA
void UTouchEngineComponentBase::PostEditChangeProperty(FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e);

	FName PropertyName = (e.Property != NULL) ? e.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UTouchEngineComponentBase, ToxFilePath))
	{
		// unbind delegates if they're already bound
		UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		if (paramsLoadedDelHandle.IsValid() || loadFailedDelHandle.IsValid())
			teSubsystem->UnbindDelegates(paramsLoadedDelHandle, loadFailedDelHandle);
		// Regrab parameters if the ToxFilePath variable has been changed
		LoadParameters();
		// Refresh details panel
		dynamicVariables.OnToxFailedLoad.Broadcast();
	}
}
#endif

void UTouchEngineComponentBase::LoadParameters()
{
	// Make sure dynamic variables parent is set
	dynamicVariables.parent = this;

	//if (ToxFilePath.IsEmpty())
	//{
	//	// No tox path set
	//	return;
	//}

	if (!GEngine)
		return; 

	UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();

	// Attempt to grab parameters list. Send delegates to TouchEngine engine subsystem that will be called when parameters are loaded or fail to load.
	teSubsystem->GetParamsFromTox(
		GetAbsoluteToxPath(),
		FTouchOnParametersLoaded::FDelegate::CreateRaw(&dynamicVariables, &FTouchEngineDynamicVariableContainer::ToxParametersLoaded),
		FSimpleDelegate::CreateRaw(&dynamicVariables, &FTouchEngineDynamicVariableContainer::ToxFailedLoad),
		paramsLoadedDelHandle, loadFailedDelHandle
	);
}

void UTouchEngineComponentBase::LoadTox()
{
	//if (ToxFilePath.IsEmpty())
	//{
	//	// No tox path set
	//	return;
	//}

	// set the parent of the dynamic variable container to this
	dynamicVariables.parent = this;

	if (!EngineInfo)
	{
		// Create TouchEngine instance if we haven't loaded one already
		CreateEngineInfo();
	}
}

FString UTouchEngineComponentBase::GetAbsoluteToxPath()
{
	if (ToxFilePath.IsEmpty())
	{
		// No tox path set
		return FString();
	}

	FString absoluteToxPath;
	absoluteToxPath = FPaths::ProjectContentDir();
	absoluteToxPath.Append(ToxFilePath);
	return absoluteToxPath;
}


// Called every frame
void UTouchEngineComponentBase::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// do nothing if tox file isn't loaded yet
	if (!EngineInfo || !EngineInfo->isLoaded())
	{
		return;
	}


	switch (cookMode)
	{
	case ETouchEngineCookMode::COOKMODE_INDEPENDENT:
	{
		// Tell TouchEngine to run in Independent mode. Sets inputs arbitrarily, get outputs whenever they arrive
		dynamicVariables.SendInputs(EngineInfo);
		EngineInfo->cookFrame((int64)(10000 * DeltaTime));
		dynamicVariables.GetOutputs(EngineInfo);
	}
	break;
	case ETouchEngineCookMode::COOKMODE_SYNCHRONIZED:
	{
		// locked sync mode stalls until we can get that frame's output. Cook is started on begin frame,
		// outputs are read on tick

		// stall until cook is finished
		UTouchEngineInfo* savedEngineInfo = EngineInfo;
		FGenericPlatformProcess::ConditionalSleep([savedEngineInfo]() {return savedEngineInfo->isCookComplete(); }, .0001f);
		// cook is finished
		dynamicVariables.GetOutputs(EngineInfo);
	}
	break;
	case ETouchEngineCookMode::COOKMODE_DELAYEDSYNCHRONIZED:
	{
		// get previous frame output, then set new frame inputs and trigger a new cook.

		// make sure previous frame is done cooking, if it's not stall until it is
		UTouchEngineInfo* savedEngineInfo = EngineInfo;
		FGenericPlatformProcess::ConditionalSleep([savedEngineInfo]() {return savedEngineInfo->isCookComplete(); }, .0001f);
		// cook is finished, get outputs
		dynamicVariables.GetOutputs(EngineInfo);
		// send inputs (cook from last frame has been finished and outputs have been grabbed)
		dynamicVariables.SendInputs(EngineInfo);
		EngineInfo->cookFrame((int64)(10000 * DeltaTime));
	}
	break;
	}
}

void UTouchEngineComponentBase::CreateEngineInfo()
{
	if (!EngineInfo)
	{
		// Create TouchEngine instance if we don't have one already
		EngineInfo = NewObject< UTouchEngineInfo>();

		//EngineInfo->getOnLoadCompleteDelegate()->AddRaw(&dynamicVariables, &FTouchEngineDynamicVariableContainer::ToxLoaded);
		loadFailedDelHandle = EngineInfo->getOnLoadFailedDelegate()->AddRaw(&dynamicVariables, &FTouchEngineDynamicVariableContainer::ToxFailedLoad);
		paramsLoadedDelHandle = EngineInfo->getOnParametersLoadedDelegate()->AddRaw(&dynamicVariables, &FTouchEngineDynamicVariableContainer::ToxParametersLoaded);
	}

	// Set variables in the EngineInfo
	EngineInfo->setCookMode(cookMode == ETouchEngineCookMode::COOKMODE_INDEPENDENT);
	EngineInfo->setFrameRate(TEFrameRate);
	// Tell the TouchEngine instance to load the tox file
	EngineInfo->load(GetAbsoluteToxPath());
}

void UTouchEngineComponentBase::ReloadTox()
{
	//if (ToxFilePath.IsEmpty())
	//{
	//	// No tox path set
	//	return;
	//}

	if (EngineInfo)
	{
		// We're in a world object

		// destroy TouchEngine instance
		EngineInfo->clear();
		EngineInfo = nullptr;
		// Reload 
		LoadTox();
	}
	else
	{
		// We're in an editor object, tell TouchEngine engine subsystem to reload the tox file
		UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		teSubsystem->ReloadTox(
			GetAbsoluteToxPath(),
			FTouchOnParametersLoaded::FDelegate::CreateRaw(&dynamicVariables, &FTouchEngineDynamicVariableContainer::ToxParametersLoaded),
			FSimpleDelegate::CreateRaw(&dynamicVariables, &FTouchEngineDynamicVariableContainer::ToxFailedLoad),
			paramsLoadedDelHandle, loadFailedDelHandle);
	}
}

bool UTouchEngineComponentBase::IsLoaded()
{
	if (EngineInfo)
	{
		// if this is a world object that has begun play and has a local touch engine instance
		return EngineInfo->isLoaded();
	}
	else
	{
		// this object has no local touch engine instance, must check the subsystem to see if our tox file has already been loaded
		UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		return teSubsystem->IsLoaded(GetAbsoluteToxPath());
	}
}

bool UTouchEngineComponentBase::HasFailedLoad()
{
	if (EngineInfo)
	{
		// if this is a world object that has begun play and has a local touch engine instance
		return EngineInfo->hasFailedLoad();
	}
	else
	{
		// this object has no local touch engine instance, must check the subsystem to see if our tox file has failed to load
		UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
		return teSubsystem->HasFailedLoad(GetAbsoluteToxPath());
	}
}

void UTouchEngineComponentBase::StartTouchEngine()
{
	LoadTox();
}

void UTouchEngineComponentBase::StopTouchEngine()
{
	if (EngineInfo)
	{
		EngineInfo->destroy();
		EngineInfo = nullptr;
	}
}

void UTouchEngineComponentBase::UnbindDelegates()
{
	if (paramsLoadedDelHandle.IsValid() && loadFailedDelHandle.IsValid())
	{
		if (EngineInfo)
		{
			EngineInfo->getOnLoadFailedDelegate()->Remove(loadFailedDelHandle);
			EngineInfo->getOnParametersLoadedDelegate()->Remove(paramsLoadedDelHandle);
		}
		else
		{
			if (!GEngine)
				return; 

			UTouchEngineSubsystem* teSubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();

			if (teSubsystem)
			{
				teSubsystem->UnbindDelegates(paramsLoadedDelHandle, loadFailedDelHandle);
			}
		}
	}
}