//
// Copyright 2015 Adam Horvath - WWW.UNREAL4AR.COM - info@unreal4ar.com - All Rights Reserved.
//

using UnrealBuildTool;
using System.IO;
using System;



public class ARToolkit : ModuleRules
{
    public ARToolkit(TargetInfo Target)
	{
		Type = ModuleType.External;

        //string SDKDIR = Path.GetDirectoryName(RulesCompiler.GetModuleFilename(this.GetType().Name));
        string SDKDIR = ModuleDirectory;


        SDKDIR = SDKDIR.Replace("\\", "/");
        if (Target.Platform == UnrealTargetPlatform.Mac){

            PublicIncludePaths.Add(SDKDIR + "/include/OSX/");
            string LibPath = SDKDIR + "/lib/OSX/";

            PublicAdditionalLibraries.Add(LibPath+"macosx-universal/libjpeg.a");
            PublicAdditionalLibraries.Add(LibPath + "libAR.a");
            PublicAdditionalLibraries.Add(LibPath + "libAR2.a");
            PublicAdditionalLibraries.Add(LibPath + "libARICP.a");
            PublicAdditionalLibraries.Add(LibPath + "libKPM.a");
            PublicAdditionalLibraries.Add(LibPath + "libARUtil.a");
            PublicAdditionalLibraries.Add(LibPath + "libARMulti.a");
            PublicAdditionalLibraries.Add(LibPath + "libARvideo.a");
            PublicAdditionalLibraries.Add(LibPath + "libEden.a");
            PublicAdditionalLibraries.Add(LibPath + "libARgsub.a");
            PublicAdditionalLibraries.Add(LibPath + "libARgsub_lite.a");

            PublicFrameworks.AddRange(
                new string[] {
                    "QTKit",
                    "CoreVideo",
                    "Accelerate"
                }
            );

} else  if (Target.Platform == UnrealTargetPlatform.IOS){
	    
        PublicIncludePaths.Add(SDKDIR + "/include/iOS/");
        string LibPath = SDKDIR + "/lib/iOS/";

        PublicAdditionalLibraries.Add(LibPath + "libKPM.a");
        PublicAdditionalLibraries.Add(LibPath+  "ios511/libjpeg.a");
        PublicAdditionalLibraries.Add(LibPath + "libAR.a");
        PublicAdditionalLibraries.Add(LibPath + "libAR2.a");
        PublicAdditionalLibraries.Add(LibPath + "libARICP.a");
        PublicAdditionalLibraries.Add(LibPath + "libARUtil.a");
        PublicAdditionalLibraries.Add(LibPath + "libARMulti.a");
        PublicAdditionalLibraries.Add(LibPath + "libARvideo.a");
        PublicAdditionalLibraries.Add(LibPath + "libEden.a");
        PublicAdditionalLibraries.Add(LibPath + "libARgsub_es.a");
        PublicAdditionalLibraries.Add(LibPath + "libARgsub_es2.a");
        PublicAdditionalLibraries.Add(LibPath + "libc++.dylib"); //Need this from IOS SDK
        
        PublicFrameworks.AddRange(
            new string[] {
                "CoreVideo",
                "Accelerate",
                "AVFoundation"

            }
    );
}else if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
		{

            PublicIncludePaths.Add(SDKDIR+"/include/Windows/");

            string LibPath;

            switch (WindowsPlatform.Compiler)
            {
                case WindowsCompiler.VisualStudio2015:
                    LibPath = SDKDIR + "/lib/Win64/vs2015";
                    break;

                case WindowsCompiler.VisualStudio2013:
                    LibPath = SDKDIR + "/lib/Win64/vs2013";
                    break;

                default:
                    throw new BuildException("Unexpected compiler version on Visual Studio 2013 / 2015 is supported!");
            }

            
            PublicLibraryPaths.Add(LibPath);
            PublicAdditionalLibraries.AddRange(
                new string[] {
                    "libjpeg.lib",
                    "pthreadVC2.lib",
                    "ARUtil.lib",
                    "AR.lib",
                    "ARMulti.lib",
                    "AR2.lib",
                    "KPM.lib",
                    "ARvideo.lib",
                    "ARICP.lib"
                }
            );

            Console.WriteLine("Sdk dir: " + SDKDIR);

            PublicDelayLoadDLLs.AddRange(new string[] { "DSVL.dll", "ARvideo.dll", "pthreadVC2.dll" });
            RuntimeDependencies.Add(new RuntimeDependency(SDKDIR+ "/../../../../../Binaries/Win64/DSVL.dll"));
            RuntimeDependencies.Add(new RuntimeDependency(SDKDIR+ "/../../../../../Binaries/Win64/ARvideo.dll"));
            RuntimeDependencies.Add(new RuntimeDependency(SDKDIR+ "/../../../../../Binaries/Win64/pthreadVC2.dll"));

            

        } else if (Target.Platform == UnrealTargetPlatform.Android){
            
            PublicIncludePaths.Add(SDKDIR + "/include/Android/");
            string LibPathAndroid = SDKDIR + "/lib/Android/armeabi-v7a/";
            PublicLibraryPaths.Add(LibPathAndroid);

            PublicAdditionalLibraries.Add(LibPathAndroid + "libar.a");
            PublicAdditionalLibraries.Add(LibPathAndroid + "libar2.a");
            PublicAdditionalLibraries.Add(LibPathAndroid + "libaricp.a");
            PublicAdditionalLibraries.Add(LibPathAndroid + "libkpm.a");
            PublicAdditionalLibraries.Add(LibPathAndroid + "libutil.a");
            PublicAdditionalLibraries.Add(LibPathAndroid + "libarmulti.a");
            PublicAdditionalLibraries.Add(LibPathAndroid + "libjpeg.a");

            PublicAdditionalLibraries.Add(LibPathAndroid + "libc++_shared.so"); 
           
        }

	}
}
