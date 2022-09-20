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

#include "TouchEngineSubsystem.h"
#include "TouchEngineDynamicVariableStruct.h"
#include "TouchEngineInfo.h"
#include "Engine/Engine.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"

void UTouchEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	TempEngineInfo = NewObject<UTouchEngineInfo>();

	if (!IsRunningCommandlet())
	{
		TempEngineInfo->PreLoad();
	}
}

void UTouchEngineSubsystem::GetParamsFromTox(FString ToxPath, UObject* Owner, FTouchOnParametersLoaded::FDelegate ParamsLoadedDel, FTouchOnFailedLoad::FDelegate LoadFailedDel, FDelegateHandle& ParamsLoadedDelHandle, FDelegateHandle& LoadFailedDelHandle)
{
	if (LoadedParams.Contains(ToxPath))
	{
		// tox file has at least started loading

		UFileParams* Params = LoadedParams[ToxPath];

		if (Params->bIsLoaded)
		{
			// tox file has already been loaded
			Params->BindOrCallDelegates(Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle);
		}
		else
		{
			if (Params->bHasFailedLoad)
			{
				// tox file has failed to load, attempt to reload
				LoadTox(ToxPath, Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle);
			}
			else
			{
				// tox file is still loading
				Params->BindOrCallDelegates(Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle);
			}
		}
	}
	else
	{
		// tox file has not started loading yet
		LoadTox(ToxPath, Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle);
	}
}

UFileParams* UTouchEngineSubsystem::GetParamsFromTox(FString ToxPath)
{
	if (LoadedParams.Contains(ToxPath))
	{
		return LoadedParams[ToxPath];
	}
	return nullptr;
}

void UTouchEngineSubsystem::UnbindDelegates(FString ToxPath, FDelegateHandle ParamsLoadedDelHandle, FDelegateHandle LoadFailedDelHandle)
{
	if (LoadedParams.Contains(ToxPath))
	{
		UFileParams* Params = LoadedParams[ToxPath];
		Params->OnParamsLoaded.Remove(ParamsLoadedDelHandle);
		Params->OnFailedLoad.Remove(LoadFailedDelHandle);
	}
}

bool UTouchEngineSubsystem::UnbindDelegates(FDelegateHandle ParamsLoadedDelHandle, FDelegateHandle LoadFailedDelHandle)
{
	for (const TPair<FString, UFileParams*>& pair : LoadedParams)
	{
		UFileParams* Params = pair.Value;
		if (Params->OnParamsLoaded.Remove(ParamsLoadedDelHandle))
		{
			return Params->OnFailedLoad.Remove(LoadFailedDelHandle);
		}
	}

	return false;
}

bool UTouchEngineSubsystem::IsLoaded(FString ToxPath) const
{
	if (LoadedParams.Contains(ToxPath))
	{
		return LoadedParams[ToxPath]->bIsLoaded;
	}

	return false;
}

bool UTouchEngineSubsystem::HasFailedLoad(FString ToxPath) const
{
	if (LoadedParams.Contains(ToxPath))
	{
		return LoadedParams[ToxPath]->bHasFailedLoad;
	}

	return false;
}

bool UTouchEngineSubsystem::ReloadTox(FString ToxPath, UObject* Owner,
	FTouchOnParametersLoaded::FDelegate ParamsLoadedDel, FTouchOnFailedLoad::FDelegate LoadFailedDel,
	FDelegateHandle& ParamsLoadedDelHandle, FDelegateHandle& LoadFailedDelHandle)
{
	UFileParams* Params;

	if (LoadedParams.Contains(ToxPath))
	{
		Params = LoadedParams[ToxPath];

		if (!Params->bIsLoaded && !Params->bHasFailedLoad)
		{
			// tox file is still loading, do nothing
			return false;
		}

		if (!TempEngineInfo->IsLoading())
		{
			// reset currently stored data
			Params->ResetEngine();
			TempEngineInfo->Unload();
			if (ParamsLoadedDelHandle.IsValid())
			{
				UnbindDelegates(ParamsLoadedDelHandle, LoadFailedDelHandle);
			}
			return LoadTox(ToxPath, Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle) != nullptr;
		}
		else
		{
			CachedToxPaths.Add(ToxPath, FToxDelegateInfo(Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle));
			return true;
		}
	}

	// tox was never loaded (can hit this if path is empty or invalid)
	return LoadTox(ToxPath, Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle) != nullptr;
}


UFileParams* UTouchEngineSubsystem::LoadTox(FString ToxPath, UObject* Owner,
	FTouchOnParametersLoaded::FDelegate ParamsLoadedDel, FTouchOnFailedLoad::FDelegate LoadFailedDel,
	FDelegateHandle& ParamsLoadedDelHandle, FDelegateHandle& LoadFailedDelHandle)
{
	if (ToxPath.IsEmpty())
		return nullptr;

	UFileParams* Params;

	if (!TempEngineInfo->IsLoading())
	{
		if (!LoadedParams.Contains(ToxPath))
		{
			// load tox
			Params = LoadedParams.Add(ToxPath, NewObject<UFileParams>());
			Params->bHasFailedLoad = false;
			Params->bIsLoaded = false;

			// bind delegates
			TempEngineInfo->GetOnParametersLoadedDelegate()->AddUObject(Params, &UFileParams::ParamsLoaded);
			TempEngineInfo->GetOnLoadFailedDelegate()->AddUObject(Params, &UFileParams::FailedLoad);
			Params->BindOrCallDelegates(Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle);

			if (TempEngineInfo->Load(ToxPath))
			{
				// failed load immediately due to probably file path error
				LoadedParams.Remove(ToxPath);
			}
		}
		else
		{
			// reloading
			Params = LoadedParams[ToxPath];

			// load tox
			Params->bHasFailedLoad = false;
			Params->bIsLoaded = false;

			// bind delegates
			TempEngineInfo->GetOnParametersLoadedDelegate()->AddUObject(Params, &UFileParams::ParamsLoaded);
			TempEngineInfo->GetOnLoadFailedDelegate()->AddUObject(Params, &UFileParams::FailedLoad);
			Params->BindOrCallDelegates(Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle);

			if (TempEngineInfo->Load(ToxPath))
			{
				// failed load immediately due to probably file path error
				LoadedParams.Remove(ToxPath);
				return nullptr;
			}
		}

	}
	else
	{
		if (LoadedParams.Contains(ToxPath))
		{
			// adding to file path either cached to load or currently loading
			Params = LoadedParams[ToxPath];
		}
		else
		{
			// reloading
			Params = LoadedParams.Add(ToxPath, NewObject<UFileParams>());
			Params->bHasFailedLoad = false;
			Params->bIsLoaded = false;
		}

		CachedToxPaths.Add(ToxPath, FToxDelegateInfo(Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle));
	}

	return Params;
}

void UTouchEngineSubsystem::LoadNext()
{
	if (TempEngineInfo)
	{
		FString JustLoaded = TempEngineInfo->GetToxPath();
		CachedToxPaths.Remove(JustLoaded);

		TempEngineInfo->Unload();
	}

	if (CachedToxPaths.Num() > 0)
	{
		UFileParams* Params;
		FString ToxPath = CachedToxPaths.begin().Key();
		FToxDelegateInfo DelegateInfo = CachedToxPaths.begin().Value();

		Params = LoadedParams[ToxPath];

		// bind delegates
		TempEngineInfo->GetOnParametersLoadedDelegate()->AddUObject(Params, &UFileParams::ParamsLoaded);
		TempEngineInfo->GetOnLoadFailedDelegate()->AddUObject(Params, &UFileParams::FailedLoad);
		Params->BindOrCallDelegates(DelegateInfo.Owner, DelegateInfo.ParamsLoadedDelegate, DelegateInfo.FailedLoadDelegate, DelegateInfo.ParamsLoadedDelegateHandle, DelegateInfo.LoadFailedDelegateHandle);

		// Remove now, since Load(ToxPath) might fail, and cause LoadNext to try to load the same path again, crashing Unreal.
		CachedToxPaths.Remove(ToxPath);

		if (TempEngineInfo->Load(ToxPath))
		{
			LoadedParams.Remove(ToxPath);
		}
	}
	else
	{
		TempEngineInfo->Unload();
	}
}

void UFileParams::BindOrCallDelegates(UObject* Owner, FTouchOnParametersLoaded::FDelegate ParamsLoadedDel, FTouchOnFailedLoad::FDelegate failedLoadDel,
	FDelegateHandle& ParamsLoadedDelHandle, FDelegateHandle& LoadFailedDelHandle)
{
	if (OnParamsLoaded.IsBoundToObject(Owner) || OnFailedLoad.IsBoundToObject(Owner))
	{
		return;
	}

	ParamsLoadedDelHandle = OnParamsLoaded.Add(ParamsLoadedDel);
	LoadFailedDelHandle = OnFailedLoad.Add(failedLoadDel);

	if (bIsLoaded)
	{
		ParamsLoadedDel.Execute(Inputs, Outputs);
	}

	if (bHasFailedLoad)
	{
		failedLoadDel.Execute(ErrorString);
	}
}

void UFileParams::ParamsLoaded(const TArray<FTouchEngineDynamicVariableStruct>& InInputs, const TArray<FTouchEngineDynamicVariableStruct>& InOutputs)
{
	// set dynamic variable arrays
	Inputs = InInputs;
	Outputs = InOutputs;
	// set variables
	bIsLoaded = true;
	bHasFailedLoad = false;
	// call delegate for parameters being loaded
	if (OnParamsLoaded.IsBound())
		OnParamsLoaded.Broadcast(Inputs, Outputs);
	// load next
	UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
	TESubsystem->LoadNext();
}

void UFileParams::FailedLoad(const FString& Error)
{
	bIsLoaded = false;
	bHasFailedLoad = true;

	OnFailedLoad.Broadcast(Error);
	ErrorString = Error;

	UTouchEngineSubsystem* TESubsystem = GEngine->GetEngineSubsystem<UTouchEngineSubsystem>();
	TESubsystem->LoadNext();
}

void UFileParams::ResetEngine()
{
	bIsLoaded = false;
	bHasFailedLoad = false;
	Inputs.Empty();
	Outputs.Empty();
}
