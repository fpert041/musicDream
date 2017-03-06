//
// Copyright 2016 Adam Horvath - WWW.UNREAL4AR.COM - info@unreal4ar.com - All Rights Reserved.
//

#include "ARToolkitPluginPrivatePCH.h"
#include "IARToolkitPlugin.h"


#ifdef __ANDROID__
#include "../../../Core/Public/Android/AndroidApplication.h"
#include "../../../Launch/Public/Android/AndroidJNI.h"
#include <android/log.h>

#define LOG_TAG "CameraLOG"

int SetupJNICamera(JNIEnv* env);
JNIEnv* ENV = NULL;

static jmethodID AndroidThunkJava_CamStart;
static jmethodID AndroidThunkJava_CamStop;
static jmethodID AndroidThunkJava_UnpackData;

int FrameWidth = 320;
int FrameHeight = 240;
bool newFrame = false;
bool processing = false;

unsigned char* Buffer =  new unsigned char[FrameWidth*FrameHeight];
signed char* BufferTmp = new signed char[FrameWidth*FrameHeight];

#endif

bool processingTexture = false;

class FARToolkitPlugin : public IARToolkitPlugin
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE( FARToolkitPlugin, ARToolkitPlugin )

void FARToolkitPlugin::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
	// Attempt to create the device, and start it up.  Caches a pointer to the device if it successfully initializes

#if PLATFORM_ANDROID
	JNIEnv* env = FAndroidApplication::GetJavaEnv();
	SetupJNICamera(env);
#endif


	TSharedPtr<FARToolkitDevice> ARToolkitStartup(new FARToolkitDevice);
	if (ARToolkitStartup->StartupDevice())
	{
		ARToolkitDevice = ARToolkitStartup;
	}
	//TODO: error handling	if dll cannot be loaded  

}


void FARToolkitPlugin::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	if (ARToolkitDevice.IsValid())
	{
		ARToolkitDevice->ShutdownDevice();
		ARToolkitDevice = nullptr;
	}

#ifdef __ANDROID__
	FMemory::Free(Buffer);
	FMemory::Free(BufferTmp);
#endif

	
}

#ifdef __ANDROID__
int SetupJNICamera(JNIEnv* env)
{
	if (!env) return JNI_ERR;

	ENV = env;


	AndroidThunkJava_CamStart = FJavaWrapper::FindMethod(ENV, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_CamStart", "()V", false);
	if (!AndroidThunkJava_CamStart)
	{
		UE_LOG(ARToolkit, Log, TEXT("ERROR: CamStart"));
		return JNI_ERR;
	}

	AndroidThunkJava_CamStop = FJavaWrapper::FindMethod(ENV, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_CamStop", "()V", false);
	if (!AndroidThunkJava_CamStop)
	{
		UE_LOG(ARToolkit, Log, TEXT("ERROR: CamStop"));
		return JNI_ERR;
	}

	AndroidThunkJava_UnpackData = FJavaWrapper::FindMethod(ENV, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_UnpackData", "()V", false);
	if (!AndroidThunkJava_UnpackData)
	{
		UE_LOG(ARToolkit, Log, TEXT("ERROR: UnpackData"));
		return JNI_ERR;
	}

	return JNI_OK;
}
void AndroidThunkCpp_CamStart()
{
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, AndroidThunkJava_CamStart);
	}
}

void AndroidThunkCpp_CamStop()
{
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, AndroidThunkJava_CamStop);
	}
}

void AndroidThunkCpp_UnpackData()
{
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, AndroidThunkJava_UnpackData);
	}
}

extern "C" bool Java_com_epicgames_ue4_GameActivity_nativeCameraFrameArrived(JNIEnv* LocalJNIEnv, jobject LocalThiz, jint frameWidth, jint frameHeight, jbyteArray data)
{
	if (!processingTexture){
		int len = LocalJNIEnv->GetArrayLength(data);

		//Copy webcam data to the buffer
		LocalJNIEnv->GetByteArrayRegion(data, 0, len, reinterpret_cast<jbyte*>(BufferTmp));
		Buffer = (unsigned char*)BufferTmp;
		newFrame = true;
		//FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Native Camera frame arrived: '%d %d'\n"), frameWidth, frameHeight);
	}

	return JNI_TRUE;
}
#endif



