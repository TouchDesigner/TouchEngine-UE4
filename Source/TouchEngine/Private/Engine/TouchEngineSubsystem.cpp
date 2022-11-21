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

#include "Engine/TouchEngineSubsystem.h"

#include "Engine/TouchEngineInfo.h"
#include "Engine/FileParams.h"
#include "Engine/TouchEngine.h"

void UTouchEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	TempEngineInfo = NewObject<UTouchEngineInfo>();
}

void UTouchEngineSubsystem::GetOrLoadParamsFromTox(FString ToxPath, UObject* Owner, FTouchOnParametersLoaded::FDelegate ParamsLoadedDel, FTouchOnFailedLoad::FDelegate LoadFailedDel, FDelegateHandle& ParamsLoadedDelHandle, FDelegateHandle& LoadFailedDelHandle)
{
	if (const TObjectPtr<UFileParams>* PotentialParams = LoadedParams.Find(ToxPath))
	{
		UFileParams* Params = *PotentialParams;
		if (Params->bHasFailedLoad)
		{
			// Attempt to reload
			LoadTox(ToxPath, Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle);
		}
		else // Tox file is either loading or has already loaded
		{
			Params->BindOrCallDelegates(Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle);
		}
	}
	else
	{
		// tox file has not started loading yet
		LoadTox(ToxPath, Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle);
	}
}

bool UTouchEngineSubsystem::ReloadTox(const FString& ToxPath, UObject* Owner, FTouchOnParametersLoaded::FDelegate ParamsLoadedDel, FTouchOnFailedLoad::FDelegate LoadFailedDel, FDelegateHandle& ParamsLoadedDelHandle, FDelegateHandle& LoadFailedDelHandle)
{
	if (const TObjectPtr<UFileParams>* PotentialParams = LoadedParams.Find(ToxPath))
	{
		UFileParams* Params = *PotentialParams;
		const bool bIsStillLoading = !Params->bIsLoaded && !Params->bHasFailedLoad; 
		if (bIsStillLoading)
		{
			return false;
		}

		const bool bQueueForLater = TempEngineInfo->Engine->IsLoading(); 
		if (bQueueForLater)
		{
			CachedToxPaths.Add(ToxPath, FToxDelegateInfo(Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle));
			return true;
		}
		TempEngineInfo->GetSupportedPixelFormats(CachedSupportedPixelFormats);
		
		// Reset currently stored data
		Params->ResetEngine();
		TempEngineInfo->Unload();
		if (ParamsLoadedDelHandle.IsValid())
		{
			UnbindDelegates(ParamsLoadedDelHandle, LoadFailedDelHandle);
		}
		return LoadTox(ToxPath, Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle) != nullptr;
	}

	// tox was never loaded (can hit this if path is empty or invalid)
	return LoadTox(ToxPath, Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle) != nullptr;
}

bool UTouchEngineSubsystem::IsSupportedPixelFormat(EPixelFormat PixelFormat) const
{
	bool bResult = CachedSupportedPixelFormats.Contains(PixelFormat);
	return bResult;
}

TObjectPtr<UFileParams> UTouchEngineSubsystem::GetParamsFromToxIfLoaded(FString ToxPath)
{
	const TObjectPtr<UFileParams>* Value = LoadedParams.Find(ToxPath);
	return Value && Value->Get()
		? Value->Get()
		: nullptr; 
}

bool UTouchEngineSubsystem::IsLoaded(const FString& ToxPath) const
{
	const TObjectPtr<UFileParams>* Value = LoadedParams.Find(ToxPath);
	return Value && Value->Get()
		? Value->Get()->bIsLoaded
		: false; 
}

bool UTouchEngineSubsystem::HasFailedLoad(const FString& ToxPath) const
{
	const TObjectPtr<UFileParams>* Value = LoadedParams.Find(ToxPath);
	return Value && Value->Get()
		? Value->Get()->bHasFailedLoad
		: false; 
}

bool UTouchEngineSubsystem::UnbindDelegates(FDelegateHandle ParamsLoadedDelHandle, FDelegateHandle LoadFailedDelHandle)
{
	for (const TPair<FString, UFileParams*>& Pair : LoadedParams)
	{
		UFileParams* Params = Pair.Value;
		if (Params->ParamsLoadedDelegate.Remove(ParamsLoadedDelHandle))
		{
			return Params->FailedLoadDelegate.Remove(LoadFailedDelHandle);
		}
	}
	return false;
}

UFileParams* UTouchEngineSubsystem::LoadTox(FString ToxPath, UObject* Owner, FTouchOnParametersLoaded::FDelegate ParamsLoadedDel, FTouchOnFailedLoad::FDelegate LoadFailedDel, FDelegateHandle& ParamsLoadedDelHandle, FDelegateHandle& LoadFailedDelHandle)
{
	if (ToxPath.IsEmpty() || !TempEngineInfo || !TempEngineInfo->Engine)
	{
		return nullptr;
	}

	UFileParams* Params = nullptr;
	if (!TempEngineInfo->Engine->HasAttemptedToLoad())
	{
		if (!LoadedParams.Contains(ToxPath))
		{
			// load tox
			Params = LoadedParams.Add(ToxPath, NewObject<UFileParams>());
			Params->bHasFailedLoad = false;
			Params->bIsLoaded = false;

			// bind delegates
			TempEngineInfo->GetOnParametersLoadedDelegate()->AddUObject(Params, &UFileParams::OnParamsLoaded);
			TempEngineInfo->GetOnLoadFailedDelegate()->AddUObject(Params, &UFileParams::OnFailedLoad);
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
			TempEngineInfo->GetOnParametersLoadedDelegate()->AddUObject(Params, &UFileParams::OnParamsLoaded);
			TempEngineInfo->GetOnLoadFailedDelegate()->AddUObject(Params, &UFileParams::OnFailedLoad);
			Params->BindOrCallDelegates(Owner, ParamsLoadedDel, LoadFailedDel, ParamsLoadedDelHandle, LoadFailedDelHandle);

			if (TempEngineInfo->Load(ToxPath))
			{
				// failed load immediately due to probably file path error
				LoadedParams.Remove(ToxPath);
				return nullptr;
			}
		}

		TempEngineInfo->GetSupportedPixelFormats(CachedSupportedPixelFormats);
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
	if (TempEngineInfo && TempEngineInfo->Engine)
	{
		FString JustLoaded = TempEngineInfo->Engine->GetToxPath();
		CachedToxPaths.Remove(JustLoaded);

		TempEngineInfo->Unload();
	}

	if (CachedToxPaths.Num() > 0)
	{
		FString ToxPath = CachedToxPaths.begin().Key();
		FToxDelegateInfo DelegateInfo = CachedToxPaths.begin().Value();

		UFileParams* Params = LoadedParams[ToxPath];

		TempEngineInfo->GetOnParametersLoadedDelegate()->AddUObject(Params, &UFileParams::OnParamsLoaded);
		TempEngineInfo->GetOnLoadFailedDelegate()->AddUObject(Params, &UFileParams::OnFailedLoad);
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
