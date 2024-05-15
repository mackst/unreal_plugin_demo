// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class PointCloudRender : ModuleRules
{

    // vulkan sdk path
    //private static readonly string VulkanSDKPath = "";
    private static readonly string VulkanSDKLibPath = Path.Combine(GetVulkanSDKPath(), "Lib");
    private static readonly string VulkanSDKIncludePath = Path.Combine(GetVulkanSDKPath(), "Include");

    private static string GetVulkanSDKPath()
    {
        string path = Environment.GetEnvironmentVariable("VULKAN_SDK");

        if (string.IsNullOrEmpty(path))
            throw new Exception("VULKAN_SDK not found.");

        return path;
    }

    private static string GetVulkanSDKLibPath() 
    {
        return Path.Combine(GetVulkanSDKPath(), "Lib");
    }

    public PointCloudRender(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
                VulkanSDKIncludePath,
                // ... add public include paths required here ...
            }
            );
                
        
        PrivateIncludePaths.AddRange(
            new string[] {
                //Path.Combine(GetVulkanSDKPath(), "Include"),
                // ... add other private include paths required here ...
            }
            );
            
        
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "ImageWriteQueue",
                // ... add other public dependencies that you statically link with here ...
            }
            );
        
        if (Target.Platform == UnrealTargetPlatform.Win64)
            PublicAdditionalLibraries.Add(
                Path.Combine(VulkanSDKLibPath, "vulkan-1.lib")
                //Path.Combine(VulkanSDKLibPath, "volk.lib")
            );
        
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Projects",
                "InputCore",
                "EditorFramework",
                "UnrealEd",
                "ToolMenus",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                // ... add private dependencies that you statically link with here ...	
            }
            );
        
        
        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
            );
    }
}
