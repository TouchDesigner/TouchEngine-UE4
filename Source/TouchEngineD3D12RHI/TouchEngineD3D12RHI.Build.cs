// Copyright © Derivative Inc. 2022

using System.IO;
using UnrealBuildTool;

public class TouchEngineD3D12RHI : ModuleRules
{
	public TouchEngineD3D12RHI(ReadOnlyTargetRules Target) : base(Target)
	{
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
				Path.Combine(EngineDir, @"Source\Runtime\D3D12RHI\Private"),
				Path.Combine(EngineDir, @"Source\Runtime\D3D12RHI\Private\Windows")
			});
		
		PublicSystemLibraries.AddRange(new string[] {
			"DXGI.lib",
			"d3d12.lib"
		});
		
		if (Target.Platform != UnrealTargetPlatform.HoloLens)
		{
			AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelMetricsDiscovery");
			AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelExtensionsFramework");
		}
		
		AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAftermath");
	}
}
