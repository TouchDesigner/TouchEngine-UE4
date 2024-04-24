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

public class TouchEngineD3D11RHI : ModuleRules
{
	public TouchEngineD3D11RHI(ReadOnlyTargetRules Target) : base(Target)
	{
		bUseUnity = false;
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] 
			{ 
				"Core",
				"CoreUObject",
				"Engine"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"D3D11RHI",
				"RenderCore",
				"RHI", 
				"TouchEngine",
				"TouchEngineAPI", 
				"TouchEngine"
			}
		);
		
		if (!Target.Platform.IsInGroup(UnrealPlatformGroup.Windows))
		{
			PrecompileForTargets = PrecompileTargetsType.None;
		}
		
		var EngineDir = Path.GetFullPath(Target.RelativeEnginePath);
		PrivateIncludePaths.AddRange(
			new string[] {
				Path.Combine(EngineDir, @"Source\Runtime\Windows\D3D11RHI\Private"),
				Path.Combine(EngineDir, @"Source\Runtime\Windows\D3D11RHI\Private\Windows")
			});
		
		PublicSystemLibraries.AddRange(new string[] {
			"DXGI.lib",
			"d3d11.lib"
		});

		if (Target.Version.MajorVersion == 5 && Target.Version.MinorVersion < 4)
		{
			AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelMetricsDiscovery");
		}
		AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelExtensionsFramework");
		
		AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAftermath");
	}
}
