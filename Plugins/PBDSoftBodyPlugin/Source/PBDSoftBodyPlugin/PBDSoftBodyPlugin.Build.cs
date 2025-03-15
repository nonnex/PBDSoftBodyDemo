using UnrealBuildTool;

public class PBDSoftBodyPlugin : ModuleRules
{
    public PBDSoftBodyPlugin(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Projects" }); // Added "Projects"
        PrivateDependencyModuleNames.AddRange(new string[] { "RenderCore", "RHI" });

        PrivateIncludePaths.AddRange(new string[]
        {
            "PBDSoftBodyPlugin/Private/Core",
            "PBDSoftBodyPlugin/Private/Simulation",
            "PBDSoftBodyPlugin/Private/Rendering",
            "PBDSoftBodyPlugin/Private/Animation"
        });

        PublicIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory, "Public"));

        PrivateIncludePathModuleNames.AddRange(new string[] { });
        PublicIncludePathModuleNames.AddRange(new string[] { });

        bEnableExceptions = false;
        bUseUnity = true;
        OptimizeCode = CodeOptimization.InShippingBuildsOnly;
    }
}