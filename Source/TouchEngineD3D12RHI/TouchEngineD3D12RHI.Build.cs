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

using System.IO;
using UnrealBuildTool;

public class TouchEngineD3D12RHI : ModuleRules
{
	public TouchEngineD3D12RHI(ReadOnlyTargetRules Target) : base(Target)
	{
		bUseUnity = false;
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] 
			{ 
				"Core",
				"CoreUObject",
				"Engine"
			});
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"D3D12RHI",
				"RenderCore",
				"RHI", 
				"RHICore",
				"TouchEngine",
				"TouchEngineAPI"
			}
		);
		
		if (!Target.Platform.IsInGroup(UnrealPlatformGroup.Windows))
		{
			PrecompileForTargets = PrecompileTargetsType.None;
		}
		
		var EngineDir = Path.GetFullPath(Target.RelativeEnginePath);
		PrivateIncludePaths.AddRange(
			new string[] {
				Path.Combine(EngineDir, @"Source\Runtime\D3D12RHI\Private"),
				Path.Combine(EngineDir, @"Source\Runtime\D3D12RHI\Private\Windows"),
				Path.Combine(EngineDir, @"Source\ThirdParty\Windows\D3DX12\include")
			});
		
		PublicSystemLibraries.AddRange(new string[] {
			"DXGI.lib",
			"d3d12.lib",
			"dxguid.lib"
		});
		
		if (Target.Platform != UnrealTargetPlatform.HoloLens)
		{
			AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelMetricsDiscovery");
			AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelExtensionsFramework");
		}
		
		AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAftermath");
		AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");
	}
}
