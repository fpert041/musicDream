//
// Copyright 2015 Adam Horvath - WWW.UNREAL4AR.COM - info@unreal4ar.com - All Rights Reserved.
//
using UnrealBuildTool;
using System.IO;
using System;

namespace UnrealBuildTool.Rules
{
	public class ARToolkitPlugin : ModuleRules
	{
		public ARToolkitPlugin(TargetInfo Target)
		{
			PublicIncludePaths.AddRange(
				new string[] {
                    "Developer/ARToolkitPlugin/Public",
					// ... add public include paths required here ...
                    "Runtime/Core/Public/Android",
                    "Runtime/Launch/Private/Android",
                    "Runtime/Launch/Public/Android"
                }
				);

			PrivateIncludePaths.AddRange(
				new string[] {
					"Developer/ARToolkitPlugin/Private",

                    
					// ... add other private include paths required here ...
                    
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core","Engine","ARToolkit","CoreUObject","RHI","RenderCore",
				}
				);

			
                string SDKDIR = Utils.ResolveEnvironmentVariable("%NVPACK_ROOT%");
                SDKDIR = SDKDIR.Replace("\\", "/");

                if (Target.Platform == UnrealTargetPlatform.Android)
                {
						string pluginPath = Utils.MakePathRelativeTo(ModuleDirectory, BuildConfiguration.RelativeEnginePath);
						AdditionalPropertiesForReceipt.Add(new ReceiptProperty("AndroidPlugin", Path.Combine(pluginPath, "ARToolkitPlugin_APL.xml")));

                        Console.WriteLine((Path.Combine(pluginPath, "ARToolkitPlugin_APL.xml")));
                }
		}
	}
}