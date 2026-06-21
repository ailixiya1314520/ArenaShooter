using UnrealBuildTool;

public class ArenaShooter : ModuleRules
{
    public ArenaShooter(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "AIModule",
            "GameplayTasks",
            "NavigationSystem",
            "EnhancedInput",
            "OnlineSubsystem",
            "OnlineSubsystemUtils",
            "UMG"
        });
    }
}