// Copyright © Derivative Inc. 2022

using System.IO;
using UnrealBuildTool;

public class TouchEngineD3D11RHI : ModuleRules
{
	public TouchEngineD3D11RHI(ReadOnlyTargetRules Target) : base(Target)
	{
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
				"TouchEngineAPI", "TouchEngine"
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
		
		
		if (Target.Platform != UnrealTargetPlatform.HoloLens)
		{
			AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelMetricsDiscovery");
			AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelExtensionsFramework");
		}
		
		AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAftermath");
	}
}
