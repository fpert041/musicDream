//
// Copyright 2016 Adam Horvath - WWW.UNREAL4AR.COM - info@unreal4ar.com - All Rights Reserved.
//

#include "ARToolkitPluginPrivatePCH.h"
#include "ARToolkitDevice.h"

#include <string>
#include <iostream>
#include <sstream>

//General Log
DEFINE_LOG_CATEGORY(ARToolkit);

//Quickfix for MSVC 2015 linker errors (libjpeg.lib)
#if (_MSC_VER >= 1900)
FILE _iob[] = { *stdin, *stdout, *stderr };

extern "C" FILE * __cdecl __iob_func(void)
{
	return _iob;
}
#endif

#ifdef __ANDROID__
	#include "AndroidJNI.h"
#endif

#include "Core.h"

#ifdef __ANDROID__

extern void AndroidThunkCpp_CamStart();
extern void AndroidThunkCpp_CamStop();
extern void AndroidThunkCpp_UnpackData();

extern int FrameWidth;
extern int FrameHeight;

extern bool newFrame;
extern unsigned char* Buffer;

#endif

extern bool processingTexture;


ARHandle*			FARToolkitDevice::arHandle;
ARPattHandle*		FARToolkitDevice::arPattHandle;
AR3DHandle*			FARToolkitDevice::ar3DHandle;

// Image acquisition.
ARUint8*			FARToolkitDevice::gARTImage = NULL;

int                 FARToolkitDevice::xsize;
int					FARToolkitDevice::ysize;
int                 FARToolkitDevice::mWebcamResX = 640;
int					FARToolkitDevice::mWebcamResY = 480;
double              FARToolkitDevice::patt_width;
ARParamLT*			FARToolkitDevice::gCparamLT;

FString             FARToolkitDevice::DataPath;

// NFT
THREAD_HANDLE_T*	FARToolkitDevice::threadHandle = NULL;
AR2HandleT*			FARToolkitDevice::ar2Handle = NULL;
KpmHandle*			FARToolkitDevice::kpmHandle = NULL;
int                 FARToolkitDevice::surfaceSetCount = 0;
AR2SurfaceSetT*		FARToolkitDevice::surfaceSet[PAGES_MAX];

int					FARToolkitDevice::filterSampleRate=30;
int					FARToolkitDevice::filterCutOffFreq=15;
bool				FARToolkitDevice::filterEnabled = true;

int				FARToolkitDevice::debugMode = false;
int				FARToolkitDevice::threshold;


FARToolkitDevice::FARToolkitDevice()
{
	initiated = 0;
	patt_width = 80.0;
	gCparamLT = NULL;
	paused=false;

	patternDetectionMode = EArPatternDetectionMode::TEMPLATE_MATCHING_COLOR;
	deviceOrientation = EDeviceOrientation::LANDSCAPE;
    
	iPhoneLaunched = true;



    UE_LOG(ARToolkit, Log, TEXT("Device start"));
}

FARToolkitDevice::~FARToolkitDevice()
{
	UE_LOG(ARToolkit, Log, TEXT("Device shutdown"));
    Cleanup();
}

bool FARToolkitDevice::StartupDevice()
{

//
//Setup data directory path 
//

//ANDROID
#if defined __ANDROID__
	FString AbsoluteContentPath = GFilePathBase + TEXT("/UE4Game/") + FApp::GetGameName() + TEXT("/") + FString::Printf(TEXT("%s/Content/"), FApp::GetGameName());
	DataPath = AbsoluteContentPath + FString("ARToolkit/");

#endif

//WIN64 , MAC OSX
#if defined _WIN64 || defined MAC_ONLY
	DataPath = FPaths::GameContentDir() + "ARToolkit/";
	DataPath = FPaths::ConvertRelativePathToFull(DataPath); //Absolute path
#endif

//IPHONE
#if defined IPHONE_ONLY
	DataPath = ConvertToIOSPath(FString::Printf(TEXT("%s"), FApp::GetGameName()).ToLower() + FString("/content/ARToolkit/"), 0);
	DataPath = FPaths::ConvertRelativePathToFull(DataPath); //Absolute path

	UE_LOG(ARToolkit, Log, TEXT("IOS Datapath:%s"),*DataPath);
#endif

//Default texture size

#ifdef IPHONE_ONLY
	int width = 320;
	int height = 240;
#endif

#if defined _WIN64 || defined MAC_ONLY
	xsize = 640;
	ysize = 480;
#endif

	int pixelSize = 3;
	return true;
}

UTexture2D* FARToolkitDevice::GetWebcamTexture(){
	if (this->initiated) {
		return this->WebcamTexture;
	} else {
		/*
		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("ARToolKit is not initialized properly. Possible webcam init error")));
		}
		*/
		return this->DummyTexture;
	}
}


void FARToolkitDevice::ShutdownDevice()
{
	Cleanup();
}

void FARToolkitDevice::UpdateDevice() {
	if (initiated == false) return;
	
#ifdef __ANDROID__
	UpdateTextureAndroid();
#endif

#if defined __APPLE__
	UpdateTextureApple();
#endif

#if defined _WIN64
	UpdateTextureWindows();
#endif

	
	switch (this->patternDetectionMode) {
		//TEMPLATE
		case EArPatternDetectionMode::TEMPLATE_MATCHING_COLOR: {
			if (this->Markers.Num() > 0) {
				DetectMarkers();
			}
		};
		break;

		//MATRIX
	case EArPatternDetectionMode::MATRIX_CODE_DETECTION: {
			if (this->MarkersMATRIX.Num() > 0) {
				DetectMarkersMATRIX();
			}
		};
		break;
		}

	//NFT
	if (this->MarkersNFT.Num() > 0) DetectMarkersNFT();

}


int FARToolkitDevice::YUVtoRGB(int y, int u, int v)
{
	int r, g, b;
	r = y + (int)1.402f*v;
	g = y - (int)(0.344f*u + 0.714f*v);
	b = y + (int)1.772f*u;
	r = r>255 ? 255 : r<0 ? 0 : r;
	g = g>255 ? 255 : g<0 ? 0 : g;
	b = b>255 ? 255 : b<0 ? 0 : b;
	return 0xff000000 | (b << 16) | (g << 8) | r;
}

uint8* FARToolkitDevice::GetDebugTexture(int width, int height) {
	if (arHandle->labelInfo.bwImage != NULL) {
		RGBQUAD* WebcamRGBXDebug = new RGBQUAD[width*height]; //Will be deleted after texture update!!!

		//Debug image
		uint8* Src = (uint8*)arHandle->labelInfo.bwImage;

		int u = 0;
		for (int i = 0; i < (width * height); i += 1) {

			WebcamRGBXDebug[u].rgbRed = Src[i];
			WebcamRGBXDebug[u].rgbGreen = Src[i];
			WebcamRGBXDebug[u].rgbBlue = Src[i];
			WebcamRGBXDebug[u].rgbReserved = 255;
			u++;
		}
		return (uint8*) WebcamRGBXDebug;
	} else {
		return NULL;
	}
}
void FARToolkitDevice::UpdateTextureWindows() {
#if defined _WIN64

	ARUint8 *image;
	if ((image = arVideoGetImage()) == NULL) return;
	const size_t Size = arHandle->xsize * arHandle->ysize* arHandle->arPixelSize;
	gARTImage = image;

	//Region
	FUpdateTextureRegion2D* region = new FUpdateTextureRegion2D(0, 0, 0, 0, xsize, ysize);

	uint8* debugBuffer = GetDebugTexture(arHandle->xsize, arHandle->ysize);

	if (debugBuffer != NULL) {
		//Debug image
		UpdateTexture(WebcamTexture, 0, 1, region, (uint32)(4 * arHandle->xsize), 4, debugBuffer, true);
	}
	else {
		//Color image
		int u = 0;
		for (int i = 0; i < Size; i += arHandle->arPixelSize) {
			WebcamRGBX[u].rgbRed = image[i + 2];
			WebcamRGBX[u].rgbGreen = image[i + 1];
			WebcamRGBX[u].rgbBlue = image[i];
			WebcamRGBX[u].rgbReserved = 255;
			u++;
		}

		UpdateTexture(WebcamTexture, 0, 1, region, (uint32)(4 * xsize), 4, (uint8*)WebcamRGBX, false);
	}

#endif
}


void FARToolkitDevice::UpdateTextureAndroid() {
#ifdef __ANDROID__
	if (newFrame == false) return;
	processingTexture = true;
	int width = FrameWidth;
	int height = FrameHeight;

	char* yuv420sp = (char*)Buffer;
	int* rgb = new int[width * height]; //width and height of the image to be converted

	if (!Buffer) return;

	//Region
	FUpdateTextureRegion2D* region = new FUpdateTextureRegion2D(0, 0, 0, 0, FrameWidth, FrameHeight);

	//Debug image
	uint8* debugBuffer = GetDebugTexture(FrameWidth, FrameHeight);
	if (debugBuffer != NULL) {
		//Debug image
		UpdateTexture(WebcamTexture, 0, 1, region, (uint32)(4 * FrameWidth), 4, debugBuffer, true);
	}
	else {
		//YUV -> RGB converison

		int size = width*height;
		int offset = size;

		int u, v, y1, y2, y3, y4;

		for (int i = 0, k = 0; i < size; i += 2, k += 2) {
			y1 = yuv420sp[i] & 0xff;
			y2 = yuv420sp[i + 1] & 0xff;
			y3 = yuv420sp[width + i] & 0xff;
			y4 = yuv420sp[width + i + 1] & 0xff;

			u = yuv420sp[offset + k] & 0xff;
			v = yuv420sp[offset + k + 1] & 0xff;
			u = u - 128;
			v = v - 128;

			rgb[i] = YUVtoRGB(y1, u, v);
			rgb[i + 1] = YUVtoRGB(y2, u, v);
			rgb[width + i] = YUVtoRGB(y3, u, v);
			rgb[width + i + 1] = YUVtoRGB(y4, u, v);


			if (i != 0 && (i + 2) % width == 0)
				i += width;
		}

		UpdateTexture(WebcamTexture, 0, 1, region, (uint32)(4 * FrameWidth), 4, (uint8*)rgb, true);
	}

	newFrame = false; //Frame processed waiting for a new one
	gARTImage = (ARUint8*)Buffer; //for AR Toolkit
#endif
}



void FARToolkitDevice::UpdateTextureApple(){
#if defined __APPLE__
    ARUint8 *image;
   
    // update dynamic texture
    if ((image = arVideoGetImage()) != NULL) {
        gARTImage = image;	// Save the fetched image.

		const size_t SizeWebcamRGBX = arHandle->xsize * arHandle->ysize* sizeof(RGBQUAD);
        
		//Region
		FUpdateTextureRegion2D* region = new FUpdateTextureRegion2D(0, 0, 0, 0, xsize, ysize);

		//Debug image
		uint8* debugBuffer = GetDebugTexture(xsize, ysize);

		if (debugBuffer != NULL) {
			//Debug image
			UpdateTexture(WebcamTexture, 0, 1, region, (uint32)(4 * arHandle->xsize), 4, debugBuffer, true);
		} else {
			
			UpdateTexture(WebcamTexture, 0, 1, region, (uint32)(4 * xsize), 4, (uint8*)image, false);
		}
        
		
        
    }
#endif
}


void FARToolkitDevice::UpdateTexture(UTexture2D* Texture, int32 MipIndex, uint32 NumRegions, FUpdateTextureRegion2D* Regions, uint32 SrcPitch, uint32 SrcBpp, uint8* SrcData, bool bFreeData)
{
	if (SrcData == nullptr) return;

		if (Texture->Resource)
		{
			struct FUpdateTextureRegionsData
			{
				FTexture2DResource* Texture2DResource;
				int32 MipIndex;
				uint32 NumRegions;
				FUpdateTextureRegion2D* Regions;
				uint32 SrcPitch;
				uint32 SrcBpp;
				uint8* SrcData;
			};

			FUpdateTextureRegionsData* RegionData = new FUpdateTextureRegionsData;

			RegionData->Texture2DResource = (FTexture2DResource*)Texture->Resource;
			RegionData->MipIndex = MipIndex;
			RegionData->NumRegions = NumRegions;
			RegionData->Regions = Regions;
			RegionData->SrcPitch = SrcPitch;
			RegionData->SrcBpp = SrcBpp;
			RegionData->SrcData = SrcData;

			ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
				UpdateTextureRegionsData,
				FUpdateTextureRegionsData*, RegionData, RegionData,
				bool, bFreeData, bFreeData,
				{
					for (uint32 RegionIndex = 0; RegionIndex < RegionData->NumRegions; ++RegionIndex)
					{
						int32 CurrentFirstMip = RegionData->Texture2DResource->GetCurrentFirstMip();
						if (RegionData->MipIndex >= CurrentFirstMip)
						{
							RHIUpdateTexture2D(
								RegionData->Texture2DResource->GetTexture2DRHI(),
								RegionData->MipIndex - CurrentFirstMip,
								RegionData->Regions[RegionIndex],
								RegionData->SrcPitch,
								RegionData->SrcData
								+ RegionData->Regions[RegionIndex].SrcY * RegionData->SrcPitch
								+ RegionData->Regions[RegionIndex].SrcX * RegionData->SrcBpp
								);
						}
					}
			if (bFreeData)
			{
				FMemory::Free(RegionData->Regions);
				FMemory::Free(RegionData->SrcData);
			}
			delete RegionData;

			processingTexture = false;

				});
		}
	

}



bool FARToolkitDevice::Init(bool showPIN, int devNum, EDeviceOrientation deviceOri, EArPatternDetectionMode detectionMode, int32&WebcamResX, int32&WebcamResY, bool&fr){
	fr=false;

	//Check for device orientation settings
	
	//ANDROID
	FString AndroidOrientation;
	GConfig->GetString(
		TEXT("/Script/AndroidRuntimeSettings.AndroidRuntimeSettings"),
		TEXT("Orientation"),
		AndroidOrientation,
		GEngineIni
		);

	UE_LOG(ARToolkit, Log, TEXT("Android orientation: %s"), *AndroidOrientation);



	this->deviceOrientation = deviceOri;


	//IOS
	//bSupportsPortraitOrientation = True
	//bSupportsLandscapeLeftOrientation = False
	//bSupportsLandscapeRightOrientation = False
	//bSupportsUpsideDownOrientation = False

#ifdef __ANDROID__	
	AndroidThunkCpp_UnpackData();
#endif

#if defined _WIN64
	numWebcams = 0;
	IEnumMoniker *pEnum;

	HRESULT hr = EnumerateDevices(CLSID_VideoInputDeviceCategory, &pEnum);
	if (SUCCEEDED(hr))
	{
		DisplayDeviceInformation(pEnum);
		pEnum->Release();
	}
	
	UE_LOG(ARToolkit, Log, TEXT("Number of webcams: %d"),numWebcams);
	
	if (devNum <= 0 && numWebcams > 0) devNum = 1;
	if (devNum > numWebcams) devNum = 1;
	if (numWebcams == 0) {
		UE_LOG(ARToolkit, Error, TEXT("No webcam detected!"));
		return 0;
	}
#endif	
	
	UE_LOG(ARToolkit, Log, TEXT("Init started"));

	ARParam         cparam;
	AR_PIXEL_FORMAT pixFormat;

	//Return this if WebcamTexture cannot be created by any reason.
	DummyTexture = UTexture2D::CreateTransient(640,480);
	DummyTexture->SRGB = 1;
	DummyTexture->UpdateResource();
	
	xsize = WebcamResX = 640;
	ysize = WebcamResY = 480;

	//Camera calibration data file
#if defined _WIN64 || defined __APPLE__ || defined __ANDROID__	
	const char* pathData = TCHAR_TO_UTF8(*(DataPath + TEXT("Data/camera_para.dat")));
#endif

	//
	//Setup camera, create video texture 
	//


	//ANDROID
#ifdef __ANDROID__	
	//Android camera is handled in GameActivity.java / AndroidJNI.cpp , not using ARVideo lib
	pixFormat = AR_PIXEL_FORMAT_NV21;
	WebcamTexture = UTexture2D::CreateTransient(FrameWidth,FrameHeight);
	WebcamTexture->SRGB = 1;
	WebcamTexture->UpdateResource();
	

	WebcamResX = FrameWidth;
	WebcamResY = FrameHeight;

	//Video texture size
	xsize = FrameWidth;
	ysize = FrameHeight;

	//Create webcam video texture
	WebcamRGBX = new RGBQUAD[xsize*ysize];
#endif

		//
		// Windows / iPhone / OSX are using ARVideo lib
		//
#if defined _WIN64 || defined __APPLE__

		//WINDOWS
#if defined _WIN64 
		//Open camera with config parameters

		std::stringstream config;

		config << "-devNum=" << devNum<<" -flipV ";
		if (showPIN) config << "-showDialog";


		if (arVideoOpen(config.str().c_str()) < 0) {
			arVideoClose();
			return 0;
		}
		
		
#endif

	//MAC OSX
#if defined MAC_ONLY
	//Open camera with config parameters
	std::stringstream config;

	config << " -width=640 -height=480 ";
	if (showPIN) config << "-dialog";

	if (arVideoOpen(config.str().c_str()) < 0) return 0;
#endif

//IPHONE
#if defined IPHONE_ONLY
	__block bool CameraAccessGranted = false;
	__block bool CameraAccessDenied = false;
	__block bool FirstRun = false;


	AVAuthorizationStatus authStatus = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
	if(authStatus == AVAuthorizationStatusAuthorized) {
		// do your logic
		CameraAccessGranted = true;
	} else if(authStatus == AVAuthorizationStatusDenied){
		// denied
	} else if(authStatus == AVAuthorizationStatusRestricted){
		// restricted, normally won't happen
	} else if(authStatus == AVAuthorizationStatusNotDetermined){
		// not determined?!
		[AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
			if (granted){
				NSLog(@"Granted access to %@", AVMediaTypeVideo);
				CameraAccessGranted = true;
			}
			else {
				NSLog(@"Not granted access to %@", AVMediaTypeVideo);
				CameraAccessDenied = true;
			}
		 }];

		while (!CameraAccessGranted && !CameraAccessDenied) {
			[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow : 0.1]];
		}
		if (!CameraAccessDenied){
			FirstRun = true;
		}
	}
	else {
		// impossible, unknown authorization status
	}
	if (!CameraAccessGranted) return 0;
	if (FirstRun){
		fr = true;
		return 0;
	}

	UE_LOG(ARToolkit, Log, TEXT("Accessing iOS camera start"));

	//Open iPhone camera with config parameters
	NSString *flipV;
	NSString *flipH;

	if(deviceOrientation == EDeviceOrientation::PORTRAIT){
		flipV=[NSString stringWithFormat : @"-noflipv"];
		flipH=[NSString stringWithFormat : @"-nofliph"];
	}else{
        flipV=[NSString stringWithFormat : @"-flipv"];
        flipH=[NSString stringWithFormat : @"-fliph"];
	}

	NSString *vconf = [NSString stringWithFormat : @"%@ %@ %@ %@ %@", @"-format=BGRA", flipH, flipV, @"-preset=cif", @"-position=rear"];
    
    NSLog(@"%@",vconf);
	
	if (arVideoOpen(vconf.UTF8String)) {
		UE_LOG(ARToolkit, Error, TEXT("Error: Unable to open connection to camera.\n"));
		return 0;
	}

	UE_LOG(ARToolkit, Log, TEXT("Camera ok.\n"));
#endif



	//WINDOWS , MAX OSX camera resolution, pixel format
#if defined _WIN64 || __APPLE__
	//Get camera resolution
	if (arVideoGetSize(&xsize, &ysize) < 0) return 0;
	UE_LOG(ARToolkit, Log, TEXT("Camera resolution (%d,%d)\n"), xsize, ysize);

	//Get camera pixel format
	if ((pixFormat = arVideoGetPixelFormat()) < 0) return 0;
	UE_LOG(ARToolkit, Log, TEXT("Camera pixel format: %d"), (uint32)pixFormat);
#endif

	//IPHONE 
#if defined IPHONE_ONLY
	arVideoSetParami(AR_VIDEO_PARAM_IOS_FOCUS, AR_VIDEO_IOS_FOCUS_0_3M); // Default is 0.3 metres. See <AR/sys/videoiPhone.h> for allowable values.

	// Set up default camera parameters.
	arParamClear(&cparam, xsize, ysize, AR_DIST_FUNCTION_VERSION_DEFAULT);

	UE_LOG(ARToolkit, Log, TEXT("End of iPhone camera init"));
#endif

	//Create webcam video texture
	WebcamRGBX = new RGBQUAD[xsize*ysize];
	WebcamTexture = UTexture2D::CreateTransient(xsize, ysize);
	WebcamTexture->SRGB = 1;
	WebcamTexture->UpdateResource();

	WebcamResX = xsize;
	WebcamResY = ysize;

	UE_LOG(ARToolkit, Log, TEXT("Video texture created (%d,%d)\n"), xsize, ysize);

#endif

	//
	//Load camera param file
	//
	UE_LOG(ARToolkit, Log, TEXT("Param load start"));


	//Setup camera data file for 
	std::string path="";
	std::string SDataPath(TCHAR_TO_UTF8(*DataPath));
	//IPHONE
#ifdef IPHONE_ONLY  
	if(iPhoneLaunched){
		path=SDataPath+"data/camera_para.dat";
	}
	else{
		path=SDataPath+"Data/camera_para.dat";
	}

	UE_LOG(ARToolkit, Log, TEXT("ios full path camera data: %s\n"), *FString(path.c_str()));
#endif


	//WINDOWS, MAX OSX , ANDROID
#if defined _WIN64 || defined MAC_ONLY || defined __ANDROID__
	path = std::string(pathData);
#endif

	//Load the camera data file
	if (arParamLoad(path.c_str(), 1, &cparam) < 0) {
		UE_LOG(ARToolkit, Error, TEXT("Camera parameter load error !!\n"));
		return 0;
	}
	UE_LOG(ARToolkit, Log, TEXT("Param load success"));
	UE_LOG(ARToolkit, Log, TEXT("Camera init succesfull!"));
	
	//
	// Configure AR Toolkit 
	//

	//Change AR param size
	arParamChangeSize(&cparam, xsize, ysize, &cparam);

	//Create param 
	if ((gCparamLT = arParamLTCreate(&cparam, AR_PARAM_LT_DEFAULT_OFFSET)) == NULL) {
		UE_LOG(ARToolkit, Error, TEXT("Error: arParamLTCreate.\n"));
		return 0;
	}

	//Create AR handle	
	if ((arHandle = arCreateHandle(gCparamLT)) == NULL) {
		UE_LOG(ARToolkit, Error, TEXT("Error: arCreateHandle.\n"));
		return 0;
	}

	//Set pixel format
	if (arSetPixelFormat(arHandle, pixFormat) < 0) {
		UE_LOG(ARToolkit, Log, TEXT("Error: arSetPixelFormat.\n"));
		return 0;
	}

	//Create Patt handle
	if ((arPattHandle = arPattCreateHandle()) == NULL) {
		UE_LOG(ARToolkit, Error, TEXT("Error: arPattCreateHandle.\n"));
		return 0;
	}
	else{
		UE_LOG(ARToolkit, Log, TEXT("AR Patt Handle created\n"));
	}

	//Create 3D handle
	if ((ar3DHandle = ar3DCreateHandle(&cparam)) == NULL) {
		UE_LOG(ARToolkit, Error, TEXT("Error:  ar3DCreateHandle.\n"));
		return 0;
	}else{
		UE_LOG(ARToolkit, Log, TEXT("3D Handle created\n"));
	}

	switch (detectionMode){
		case EArPatternDetectionMode::MATRIX_CODE_DETECTION: {
			//Create MATRIX marker objects 0-63 (3x3 default size)
			for (int i = 0; i < 64; i++) {
				//Setup marker data in Markers array
				FMarker* marker = new FMarker();

				marker->name = FName(*FString::FromInt(i));
				marker->position = FVector::ZeroVector;
				marker->rotation = FRotator::ZeroRotator;
				marker->cameraPosition = FVector::ZeroVector;
				marker->cameraRotation = FRotator::ZeroRotator;
				marker->visible = false;
				marker->resetFilter = true;
				marker->ftmi = 0;
				marker->pageNo = 0;
				marker->filterCutoffFrequency = 0;
				marker->filterSampleRate = 0;

				//Filtering
				ApplyFilter(marker);
				MarkersMATRIX.Add(marker);
			}
			arSetPatternDetectionMode(arHandle, AR_MATRIX_CODE_DETECTION);
			UE_LOG(ARToolkit, Log, TEXT("Detection mode : MATRIX"));
			break;
		}
		case EArPatternDetectionMode::TEMPLATE_MATCHING_COLOR: {
			arSetPatternDetectionMode(arHandle, AR_TEMPLATE_MATCHING_COLOR);
			UE_LOG(ARToolkit, Log, TEXT("Detection mode : TEMPLATE"));
			break;
		}
	}

	this->patternDetectionMode = detectionMode;


	

	

	UE_LOG(ARToolkit, Log, TEXT("ARToolkit init succesfull!"));

	//////////////////////////////////////////////////////////////////////////////////////
	// NFT init.
	//////////////////////////////////////////////////////////////////////////////////////
	UE_LOG(ARToolkit, Log, TEXT("NFT init started..."));


	// KPM init.
	kpmHandle = kpmCreateHandle(gCparamLT, pixFormat);
	if (!kpmHandle) {
		UE_LOG(ARToolkit, Error, TEXT("Error: kpmCreateHandle.\n"));
		return 0;
	}

	// AR2 init.
	if ((ar2Handle = ar2CreateHandle(gCparamLT, pixFormat, AR2_TRACKING_DEFAULT_THREAD_NUM)) == NULL) {
		UE_LOG(ARToolkit, Error, TEXT("Error: ar2CreateHandle.\n"));
		kpmDeleteHandle(&kpmHandle);
		return 0;
	}

	if (threadGetCPU() <= 1) {
		UE_LOG(ARToolkit, Log, TEXT("Using NFT tracking settings for a single CPU.\n"));
		ar2SetTrackingThresh(ar2Handle, 5.0);
		ar2SetSimThresh(ar2Handle, 0.50);
		ar2SetSearchFeatureNum(ar2Handle, 16);
		ar2SetSearchSize(ar2Handle, 6);
		ar2SetTemplateSize1(ar2Handle, 6);
		ar2SetTemplateSize2(ar2Handle, 6);
	}
	else {
		UE_LOG(ARToolkit, Log, TEXT("Using NFT tracking settings for more than one CPU.\n"));
		ar2SetTrackingThresh(ar2Handle, 5.0);
		ar2SetSimThresh(ar2Handle, 0.50);
		ar2SetSearchFeatureNum(ar2Handle, 16);
		ar2SetSearchSize(ar2Handle, 12);
		ar2SetTemplateSize1(ar2Handle, 6);
		ar2SetTemplateSize2(ar2Handle, 6);
	}

	//Multi NFT
	nftMultiMode = true;
	kpmRequired = true;
	kpmBusy = false;
	
	UE_LOG(ARToolkit, Log, TEXT("NFT init successfull..."));

	//////////////////////////////////////////////////////////////////////////////////////
	// END of NFT init.
	//////////////////////////////////////////////////////////////////////////////////////
	

	//
	// Start video capture 
	//

	//Android
#ifdef __ANDROID__	
	//AndroidThunkCpp_Vibrate(1000);
	AndroidThunkCpp_CamStart();
#endif

	//Windows , MAX OSX, iPhone
#if defined _WIN64 || defined __APPLE__

	if (arVideoCapStart() != 0) {
		UE_LOG(ARToolkit, Error, TEXT("Error: video capture start error !!\n"));
		arVideoClose();
		return 0;
	}
#endif

	//Get threshold
	arGetLabelingThresh(arHandle, &threshold);


	initiated = 1;
    UE_LOG(ARToolkit, Log, TEXT("Init complete !!\n"));


	this->mWebcamResX = WebcamResX;
	this->mWebcamResY = WebcamResY;

	return 1; //Succesfull init
}
void FARToolkitDevice::SetThresholdMode(EArLabelingThresholdMode mode) {
	
	//Set treshold mode 

	switch (mode) {
		case EArLabelingThresholdMode::MANUAL: arSetLabelingThreshMode(arHandle, AR_LABELING_THRESH_MODE_MANUAL); break;
		case EArLabelingThresholdMode::AUTO_MEDIAN: arSetLabelingThreshMode(arHandle, AR_LABELING_THRESH_MODE_AUTO_MEDIAN); break;
		case EArLabelingThresholdMode::AUTO_OTSU: arSetLabelingThreshMode(arHandle, AR_LABELING_THRESH_MODE_AUTO_OTSU); break;
		case EArLabelingThresholdMode::AUTO_ADAPTIVE: arSetLabelingThreshMode(arHandle, AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE); break;
		case EArLabelingThresholdMode::AUTO_BRACKETING: {
			arSetDebugMode(arHandle, AR_DEBUG_DISABLE);
			arSetLabelingThreshMode(arHandle, AR_LABELING_THRESH_MODE_AUTO_BRACKETING); 
			
			if (this->debugMode == 1) {
				arSetDebugMode(arHandle, AR_DEBUG_ENABLE);
			}
			break;
		}
		default: arSetLabelingThreshMode(arHandle, AR_LABELING_THRESH_MODE_DEFAULT); break;
	};

	
}
void FARToolkitDevice::SetThreshold(int thresholdValue) {
	arSetLabelingThreshMode(arHandle, AR_LABELING_THRESH_MODE_MANUAL);
	arSetLabelingThresh(arHandle, thresholdValue);
	threshold = thresholdValue;
}

int FARToolkitDevice::GetThreshold() {
	arGetLabelingThresh(arHandle, &threshold);
	return threshold;
}

void FARToolkitDevice::SetDebugMode(bool mode) {
	//Set debug mode
	if (mode == true) {
		arSetDebugMode(arHandle, AR_DEBUG_ENABLE);
		this->debugMode = 1;
	} else {
		arSetDebugMode(arHandle, AR_DEBUG_DISABLE);
		this->debugMode = 0;
	}
}

void FARToolkitDevice::Cleanup(void){
	if (initiated == false) return;
	

#ifdef __ANDROID__
	AndroidThunkCpp_CamStop();
#endif

	arPattDetach(arHandle);
	arPattDeleteHandle(arPattHandle);
	ar3DDeleteHandle(&ar3DHandle);
	arDeleteHandle(arHandle);
	
	// NFT cleanup.
	UnloadNFTData();
	ar2DeleteHandle(&ar2Handle);
	kpmDeleteHandle(&kpmHandle);
	arParamLTFree(&gCparamLT);
	

	//Reset marker arrays
	Markers.Reset();
	MarkersNFT.Reset();
	MarkersMATRIX.Reset();


#if defined _WIN64 || defined __APPLE__
    arVideoCapStop();
    arVideoClose();

    
#endif
	if (WebcamRGBX)
	{
		delete[] WebcamRGBX;
		WebcamRGBX = NULL;
	}

	initiated = 0;

	UE_LOG(ARToolkit, Log, TEXT("Cleanup ready !!\n"));
}



bool FARToolkitDevice::LoadMarkers(TArray<FString> markerNames){
	if (!initiated) return false; 
	for (int32 i = 0; i != markerNames.Num(); ++i){
		FString markerName = markerNames[i];
		std::string path = "";
		std::string SDataPath(TCHAR_TO_UTF8(*DataPath));
		std::string SMarkerName(TCHAR_TO_UTF8(*markerName.ToLower()));
#ifdef IPHONE_ONLY    
		//Load Marker
		if (iPhoneLaunched){
			path=SDataPath+"data/patt."+SMarkerName;
		}
		else{
			path=SDataPath+"Data/patt."+SMarkerName;
		}
#endif   



#if defined _WIN64 || defined MAC_ONLY || defined __ANDROID__
		path = TCHAR_TO_UTF8(*(DataPath + "Data/patt." + markerName));
		
#endif

		UE_LOG(ARToolkit, Log, TEXT("Reading %s"), *FString(UTF8_TO_TCHAR(path.c_str())));

		if ((arPattLoad(arPattHandle,path.c_str())) < 0) {
			UE_LOG(ARToolkit, Error, TEXT("Error %s  pattern load error !!\n"), *markerName);

			if (GEngine){
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Unable to load marker: %s"), *markerName));
			}

			return 0;
		}
		else {
			arPattAttach(arHandle, arPattHandle);
		}
		//Setup marker data in Markers array
		FMarker* marker = new FMarker();

		marker->name = FName(*markerName);
		marker->position = FVector::ZeroVector;
		marker->rotation = FRotator::ZeroRotator;
		marker->cameraPosition = FVector::ZeroVector;
		marker->cameraRotation = FRotator::ZeroRotator;
		marker->visible = false;
		marker->resetFilter = true;
		marker->ftmi = 0;
		marker->pageNo = 0;
		marker->filterCutoffFrequency = 0;
		marker->filterSampleRate = 0;


		//Filtering
		ApplyFilter(marker);

		Markers.Add(marker);

		UE_LOG(ARToolkit, Log, TEXT("%s loaded\n"), *markerName);
	}
	return 1;
}

bool FARToolkitDevice::LoadMarkersNFT(TArray<FString> markerNames){
	if (!initiated) return false;
	KpmRefDataSet *refDataSet;

	// If data was already loaded, stop KPM tracking thread and unload previously loaded data.
	if (threadHandle) {
		UE_LOG(ARToolkit, Log, TEXT("Reloading NFT data.\n"));
		UnloadNFTData();
	}
	

	refDataSet = NULL;

	for (int32 i = 0; i != markerNames.Num(); i++){

		FString markerName = markerNames[i];

		UE_LOG(ARToolkit, Log, TEXT("Loading NFT data: %s.\n"), *markerName);

		// Load KPM data.
		KpmRefDataSet  *refDataSet2;


		//Dataset Path
		std::string path = "";
		std::string SDataPath(TCHAR_TO_UTF8(*DataPath));
		std::string SMarkerName(TCHAR_TO_UTF8(*markerName.ToLower()));

#ifdef IPHONE_ONLY    
		//Load Marker
		//
		if (iPhoneLaunched){
			path=SDataPath+"datanft/"+SMarkerName;
		} else{
			path=SDataPath+"DataNFT/"+SMarkerName;
		}

#endif


	#if defined _WIN64 || defined MAC_ONLY || defined __ANDROID__
		path = TCHAR_TO_UTF8(*(DataPath + "DataNFT/" + markerName));
	#endif

		//Setup NFT marker in MarkersNFT array
		FMarker* markerNFT = new FMarker();

		markerNFT->name = FName(*markerName);
		markerNFT->position = FVector::ZeroVector;
		markerNFT->rotation = FRotator::ZeroRotator;
		markerNFT->cameraPosition = FVector::ZeroVector;
		markerNFT->cameraRotation = FRotator::ZeroRotator;
		markerNFT->visible = false;
		markerNFT->resetFilter = true;
		markerNFT->ftmi = 0;
		markerNFT->pageNo = 0;
		markerNFT->filterCutoffFrequency = 0;
		markerNFT->filterSampleRate = 0;

		UE_LOG(ARToolkit, Log, TEXT("Reading %s.fset3\n"), *FString(UTF8_TO_TCHAR(path.c_str())));

		if (kpmLoadRefDataSet(path.c_str(), "fset3", &refDataSet2) < 0) {
			UE_LOG(ARToolkit, Error, TEXT("Error reading KPM data from %s.fset3\n"), *FString(UTF8_TO_TCHAR(path.c_str())));
			markerNFT->pageNo = -1;
			return 0;
		}

		markerNFT->pageNo = surfaceSetCount;
		

		UE_LOG(ARToolkit, Log, TEXT("Assigned page no. %d.\n"), surfaceSetCount);

		if (kpmChangePageNoOfRefDataSet(refDataSet2, KpmChangePageNoAllPages, surfaceSetCount) < 0) {
			UE_LOG(ARToolkit, Error, TEXT("Error: kpmChangePageNoOfRefDataSet\n"));
			return 0;
		}
		if (kpmMergeRefDataSet(&refDataSet, &refDataSet2) < 0) {
			UE_LOG(ARToolkit, Error, TEXT("Error: kpmMergeRefDataSet\n"));
			return 0;
		}

		UE_LOG(ARToolkit, Log, TEXT("Done\n"));


		// Load AR2 data.

		UE_LOG(ARToolkit, Log, TEXT("Reading %s.fset\n"), *FString(UTF8_TO_TCHAR(path.c_str())));

		if ((surfaceSet[surfaceSetCount] = ar2ReadSurfaceSet(path.c_str(), "fset", NULL)) == NULL) {
			UE_LOG(ARToolkit, Error, TEXT("Error reading data from %s.fset\n"), *FString(UTF8_TO_TCHAR(path.c_str())));
		}
		UE_LOG(ARToolkit, Log, TEXT("Done\n"));

		//Validity default
		markerNFT->valid = false;
		markerNFT->validPrev = false;

		//Filtering
		ApplyFilter(markerNFT);

		//Add to markersNFT array
		MarkersNFT.Add(markerNFT);

		UE_LOG(ARToolkit, Log, TEXT("Loading of NFT marker %s complete.\n"), *markerName);

		surfaceSetCount++;
		if (surfaceSetCount == PAGES_MAX) break; 
	}
	
	if (kpmSetRefDataSet(kpmHandle, refDataSet) < 0) {
		UE_LOG(ARToolkit, Error, TEXT("Error: kpmSetRefDataSet\n"));
		return 0;
	}
	kpmDeleteRefDataSet(&refDataSet);

	// Start the KPM tracking thread.
	threadHandle = trackingInitInit(kpmHandle);
	if (!threadHandle) {
		return 0;
	}
	else {
		UE_LOG(ARToolkit, Log, TEXT("KPM Thread handle initialised\n"));
	}

	
	return 1;
}


void FARToolkitDevice::DetectMarkersMATRIX() {
	if (gARTImage == NULL) return; 
	//Return if camera image buffer is NULL
	//////////////////////////////////////////////////////////////////////
	//
	// Performance optimisation (don't check markers in every frame)
	//
	/////////////////////////////////////////////////////////////////////

	static double ms_prev;
	double ms;
	double s_elapsed;

	// Find out how long since mainLoop() last ran.
	ms = FPlatformTime::Seconds();
	s_elapsed = ms - ms_prev;

#if defined _WIN64 || defined MAC_ONLY
	if (s_elapsed < 0.01f) return; // Don't update more often than 100 Hz.
#endif

#if defined __ANDROID__ || defined IPHONE_ONLY
	if (s_elapsed < 0.033f) return; // Don't update more often than 30Hz
#endif
	ms_prev = ms;

	/////////////////////////////////////////////////////////////////////

	TArray<int> visibleMarkers;

	/* detect the markers in the video frame */

	if (arDetectMarker(arHandle, gARTImage) < 0) {
		UE_LOG(ARToolkit, Error, TEXT("Error: marker detection error !!\n"));
		return;
	}

	int markerNum = arGetMarkerNum(arHandle);
	/* check for object visibility */
	ARMarkerInfo   *markerInfo;
	markerInfo = arGetMarker(arHandle);

	for (int j = 0; j < markerNum; j++) {

		//Matrix markers only
		if (markerInfo[j].idMatrix != -1) {
			
			//UE_LOG(ARToolkit, Log, TEXT("Marker idMatrix : %d"), markerInfo[j].idMatrix);
			
			ARdouble quaternions[4];
			ARdouble positions[3];
			ARdouble patt_trans_inv[3][4];
			ARdouble patt_trans[3][4];
			ARdouble err;



			if (MarkersMATRIX[markerInfo[j].idMatrix]->visible == false) {
				//NEW
				MarkersMATRIX[markerInfo[j].idMatrix]->resetFilter = true;
				err = arGetTransMatSquare(ar3DHandle, &(markerInfo[j]), patt_width, patt_trans);
				//UE_LOG(ARToolkit, Log, TEXT("New tracking %d ERR: %f"), markerInfo[j].idMatrix, (float)err);
			}
			else {
				//CONTINUE FROM LAST FRAME
				memcpy(patt_trans, MarkersMATRIX[markerInfo[j].idMatrix]->matrix, sizeof(ARdouble) * 3 * 4);
				err = arGetTransMatSquareCont(ar3DHandle, &(markerInfo[j]), patt_trans, patt_width, patt_trans);


				//UE_LOG(ARToolkit, Log, TEXT("Continue tracking %d ERR: %f"), markerInfo[j].idMatrix, (float)err);
			}
			memcpy(MarkersMATRIX[markerInfo[j].idMatrix]->matrix, patt_trans, sizeof(ARdouble) * 3 * 4);

			// Filter the pose estimate.
			if (MarkersMATRIX[markerInfo[j].idMatrix]->ftmi && filterEnabled) {
				bool reset = false;

				if (MarkersMATRIX[markerInfo[j].idMatrix]->resetFilter) {
					reset = true;
					MarkersMATRIX[markerInfo[j].idMatrix]->resetFilter = false;
				}

				if (arFilterTransMat(MarkersMATRIX[markerInfo[j].idMatrix]->ftmi, MarkersMATRIX[markerInfo[j].idMatrix]->matrix, reset) < 0) {

				}
			}
			memcpy(patt_trans, MarkersMATRIX[markerInfo[j].idMatrix]->matrix, sizeof(ARdouble) * 3 * 4);

			arUtilMatInv(patt_trans, patt_trans_inv);
			arUtilMat2QuatPos(patt_trans_inv, quaternions, positions);

			MarkersMATRIX[markerInfo[j].idMatrix]->position = FVector(patt_trans[0][3], patt_trans[1][3], -patt_trans[2][3]);
			MarkersMATRIX[markerInfo[j].idMatrix]->rotation = FQuat(quaternions[0], quaternions[1], quaternions[2], quaternions[3]).Rotator();

			//Camera position, rotation
			MarkersMATRIX[markerInfo[j].idMatrix]->cameraPosition = FVector(-positions[0], positions[1], positions[2]);
			arUtilMat2QuatPos(patt_trans, quaternions, positions);
			MarkersMATRIX[markerInfo[j].idMatrix]->cameraRotation = FQuat(quaternions[0], quaternions[1], quaternions[2], quaternions[3]).Rotator();


			visibleMarkers.Add(markerInfo[j].idMatrix);
		}
	}

	//Reset marker visibility
	for (int c = 0; c < MarkersMATRIX.Num(); c++) {
		MarkersMATRIX[c]->visible = false;
	}

	if (visibleMarkers.Num() > 0) {
		//Mark tracked markers
		for (int z = 0; z < visibleMarkers.Num(); z++) {

			MarkersMATRIX[visibleMarkers[z]]->visible = true;
		}
	}
}

void FARToolkitDevice::DetectMarkers(){
	if (gARTImage == NULL) return; //Return if camera image buffer is NULL
	//////////////////////////////////////////////////////////////////////
	//
	// Performance optimisation (don't check markers in every frame)
	//
	/////////////////////////////////////////////////////////////////////

	static double ms_prev;
	double ms;
	double s_elapsed;

	// Find out how long since mainLoop() last ran.
	ms = FPlatformTime::Seconds();
	s_elapsed = ms - ms_prev;

#if defined _WIN64 || defined MAC_ONLY
	if (s_elapsed < 0.01f) return; // Don't update more often than 100 Hz.
#endif

#if defined __ANDROID__ || defined IPHONE_ONLY
	if (s_elapsed < 0.033f) return; // Don't update more often than 30Hz
#endif
	ms_prev = ms;
	
	/////////////////////////////////////////////////////////////////////

	TArray<int> visibleMarkers;

	/* detect the markers in the video frame */

	if (arDetectMarker(arHandle, gARTImage) < 0) {
		UE_LOG(ARToolkit, Error, TEXT("Error: marker detection error !!\n"));
		return;
	}

	int markerNum = arGetMarkerNum(arHandle);
	
	

	/* check for object visibility */
	ARMarkerInfo   *markerInfo;
	markerInfo = arGetMarker(arHandle);

	for (int j = 0; j < markerNum; j++) {
		
		

		if (markerInfo[j].cf>0.7 && markerInfo[j].id != -1){

			
			
			ARdouble quaternions[4];
			ARdouble positions[3];
			ARdouble patt_trans_inv[3][4];
			ARdouble patt_trans[3][4];
			ARdouble err;

			
			//UE_LOG(ARToolkit, Log, TEXT("Last id: %d"), markerInfo[j].id);
			if (markerInfo[j].id > Markers.Num()) return;
			if (Markers[markerInfo[j].id]->visible == false){
				//NEW
				Markers[markerInfo[j].id]->resetFilter = true;
				err = arGetTransMatSquare(ar3DHandle, &(markerInfo[j]), patt_width, patt_trans);
				//UE_LOG(ARToolkit, Log, TEXT("New tracking %d ERR: %f"), markerInfo[j].id, (float)err);
			}
			else{
				//CONTINUE FROM LAST FRAME
				memcpy(patt_trans, Markers[markerInfo[j].id]->matrix, sizeof(ARdouble) * 3 * 4);
				err = arGetTransMatSquareCont(ar3DHandle, &(markerInfo[j]), patt_trans, patt_width, patt_trans);

				
				//UE_LOG(ARToolkit, Log, TEXT("Continue tracking %d ERR: %f"), markerInfo[j].id, (float)err);
			}
			memcpy(Markers[markerInfo[j].id]->matrix,patt_trans, sizeof(ARdouble) * 3 * 4);

			// Filter the pose estimate.
			if (Markers[markerInfo[j].id]->ftmi && filterEnabled) {
				bool reset = false;

				if (Markers[markerInfo[j].id]->resetFilter) {
					reset = true;
					Markers[markerInfo[j].id]->resetFilter = false;
				}
				
				if (arFilterTransMat(Markers[markerInfo[j].id]->ftmi, Markers[markerInfo[j].id]->matrix, reset) < 0) {
					
				}
			}
			memcpy(patt_trans, Markers[markerInfo[j].id]->matrix, sizeof(ARdouble) * 3 * 4);

			arUtilMatInv(patt_trans, patt_trans_inv);
			arUtilMat2QuatPos(patt_trans_inv, quaternions, positions);

			Markers[markerInfo[j].id]->position = FVector(patt_trans[0][3], patt_trans[1][3], -patt_trans[2][3]);
			Markers[markerInfo[j].id]->rotation = FQuat(quaternions[0], quaternions[1], quaternions[2], quaternions[3]).Rotator();

			//Camera position, rotation
			Markers[markerInfo[j].id]->cameraPosition = FVector(-positions[0], positions[1], positions[2]);
			arUtilMat2QuatPos(patt_trans, quaternions, positions);
			Markers[markerInfo[j].id]->cameraRotation = FQuat(quaternions[0], quaternions[1], quaternions[2], quaternions[3]).Rotator();

			visibleMarkers.Add(markerInfo[j].id);
		}
	}
	
	//Reset marker visibility
	for (int c = 0; c < Markers.Num(); c++){
		Markers[c]->visible = false;
	}

	if (visibleMarkers.Num() > 0) {
		//Mark tracked markers
		for (int z = 0; z < visibleMarkers.Num(); z++) {
			Markers[visibleMarkers[z]]->visible = true;
		}
	}
}

FMarker* FARToolkitDevice::GetMarker(uint8 markerId){
	
	if (Markers.Num() > markerId){
		//UE_LOG(ARToolkit, Log, TEXT("Marker num %s"), *Markers[markerId]->position.ToString());
		return Markers[markerId];
	} else {
		FMarker* marker = new FMarker();

		marker->name = ""; 
		marker->position = FVector::ZeroVector;
		marker->rotation = FRotator::ZeroRotator;
		marker->cameraPosition = FVector::ZeroVector;
		marker->cameraRotation = FRotator::ZeroRotator;
		marker->visible = false;

		return marker;
	}
}
FMarker* FARToolkitDevice::GetMarkerMATRIX(uint8 markerId) {
	
	if (MarkersMATRIX.Num() > markerId) {
		return MarkersMATRIX[markerId];
	}
	else {
		FMarker* marker = new FMarker();

		marker->name = "";
		marker->position = FVector::ZeroVector;
		marker->rotation = FRotator::ZeroRotator;
		marker->cameraPosition = FVector::ZeroVector;
		marker->cameraRotation = FRotator::ZeroRotator;
		marker->visible = false;
		
		return marker;
	}
}

FMarker* FARToolkitDevice::GetMarkerNFT(uint8 markerId){
	if (MarkersNFT.Num() > markerId){
		return MarkersNFT[markerId];
	} else {
		FMarker* marker = new FMarker();

		marker->name = ""; 
		marker->position = FVector::ZeroVector;
		marker->rotation = FRotator::ZeroRotator;
		marker->cameraPosition = FVector::ZeroVector;
		marker->cameraRotation = FRotator::ZeroRotator;
		marker->visible = false;
		
		return marker;
	}
}

bool FARToolkitDevice::GetRelativeTransformation(uint8 MarkerID1, uint8 MarkerID2, FVector& RelativePosition, FRotator& RelativeRotation) {
	RelativePosition = FVector::ZeroVector;
	RelativeRotation = FRotator::ZeroRotator;
	
	FMarker* Marker1 = GetMarker(MarkerID1);
	FMarker* Marker2 = GetMarker(MarkerID2);
	
	if (Marker1->visible == true && Marker2->visible == true) {

		ARdouble wmat1[3][4], wmat2[3][4];
		ARdouble quaternions[4];
		ARdouble positions[3];

		arUtilMatInv(Marker1->matrix, wmat1);
		arUtilMatMul(wmat1, Marker2->matrix, wmat2);

		
		arUtilMat2QuatPos(wmat2, quaternions, positions);
		
		RelativePosition = FVector(positions[0], -positions[1], -positions[2]);
		RelativeRotation = FQuat(quaternions[0], quaternions[1], quaternions[2], quaternions[3]).Rotator();

		return 1;

	} else {
		return 0;
	}
}

bool FARToolkitDevice::GetRelativeTransformationNFT(uint8 MarkerID1, uint8 MarkerIdNFT, FVector& RelativePosition, FRotator& RelativeRotation) {
	RelativePosition = FVector::ZeroVector;
	RelativeRotation = FRotator::ZeroRotator;

	FMarker* Marker1 = GetMarker(MarkerID1);
	FMarker* Marker2 = GetMarkerNFT(MarkerIdNFT);

	if (Marker1->visible == true && Marker2->visible == true) {

		ARdouble wmat1[3][4], wmat2[3][4];
		ARdouble quaternions[4];
		ARdouble positions[3];

		arUtilMatInv(Marker1->matrix, wmat1);
		arUtilMatMul(wmat1, Marker2->matrix, wmat2);


		arUtilMat2QuatPos(wmat2, quaternions, positions);

		RelativePosition = FVector(positions[0], -positions[1], -positions[2]);
		RelativeRotation = FQuat(quaternions[0], quaternions[1], quaternions[2], quaternions[3]).Rotator();

		return 1;

	}
	else {
		return 0;
	}
}

int FARToolkitDevice::UnloadNFTData(void)
{
	int i, j;

	if (threadHandle) {
		UE_LOG(ARToolkit, Log, TEXT("Stopping NFT2 tracking thread.\n"));
		trackingInitQuit(&threadHandle);
	}
	j = 0;
	for (i = 0; i < surfaceSetCount; i++) {
		if (j == 0) UE_LOG(ARToolkit, Log, TEXT("Unloading NFT tracking surfaces.\n"));
		ar2FreeSurfaceSet(&surfaceSet[i]); // Also sets surfaceSet[i] to NULL.
		j++;
	}
	if (j > 0) UE_LOG(ARToolkit, Log, TEXT("Unloaded %d NFT tracking surfaces.\n"), j);
	surfaceSetCount = 0;

	return 0;
}

void FARToolkitDevice::DetectMarkersNFT(void)
{
	if (gARTImage == NULL) return;

	//////////////////////////////////////////////////////////////////////
	//
	// Performance optimisation (don't check markers in every frame)
	//
	/////////////////////////////////////////////////////////////////////

	static double ms_prev_nft;
	double ms_nft;
	double s_elapsed_nft;

	// Find out how long since mainLoop() last ran.
	ms_nft = FPlatformTime::Seconds();
	s_elapsed_nft = ms_nft - ms_prev_nft;

#if defined _WIN64 || defined MAC_ONLY
	if (s_elapsed_nft < 0.01f) return; // Don't update more often than 100 Hz.
#endif

#if defined __ANDROID__ || defined IPHONE_ONLY
    if (s_elapsed_nft < 0.033f) return; // Don't update more often than 30Hz
#endif
	ms_prev_nft = ms_nft;

	////////////////////////////////////////////////////////////////////
	
	// NFT results.
	
	// Process video frame.
	// Run marker detection on frame
	if (threadHandle) {
		// Do KPM tracking.
		float err;
		static float trackingTrans[3][4];

		if (kpmRequired) {
			
			if (!kpmBusy) {
				trackingInitStart(threadHandle, gARTImage);
				kpmBusy = true;
			} else {
				int ret;
				int pageNo;
				ret = trackingInitGetResult(threadHandle, trackingTrans, &pageNo);
				if (ret != 0) {
					kpmBusy = false;
					if (ret == 1) {
						if (pageNo >= 0 && pageNo < PAGES_MAX) {
							if (surfaceSet[pageNo]->contNum < 1) {
								
								UE_LOG(ARToolkit,Log,TEXT("Detected page %d.\n"), pageNo);
								//NEW
								MarkersNFT[pageNo]->resetFilter = true;

								ar2SetInitTrans(surfaceSet[pageNo], trackingTrans); // Sets surfaceSet[page]->contNum = 1.
							}
						}
						else {
							UE_LOG(ARToolkit, Log, TEXT("ARController::update(): Detected bad page %d"), pageNo);
						}
							
					} else  {
						//UE_LOG(ARToolkit, Log, TEXT("No page detected."));
					}
				}
			}
		}
		
		// Do AR2 tracking and update NFT markers.
		int page = 0;
		int pagesTracked = 0;
		bool success = true;

		
		for (int i = 0; i < MarkersNFT.Num(); i++){
				ARdouble patt_trans[3][4];
				ARdouble patt_trans_inv[3][4];
				ARdouble quaternions[4];
				ARdouble positions[3];

				if (surfaceSet[page]->contNum > 0) {
					if (ar2Tracking(ar2Handle, surfaceSet[page], gARTImage, trackingTrans, &err) < 0) {
						UE_LOG(ARToolkit, Log, TEXT("Tracking lost on page %d."), page);
						MarkersNFT[i]->visible = false;
						
					}
					else {
						//UE_LOG(ARToolkit, Log, TEXT("Tracked page %d (pos = {% 4f, % 4f, % 4f}).\n"), page, trackingTrans[0][3], trackingTrans[1][3], trackingTrans[2][3]);
						for (int j = 0; j < 3; j++) for (int k = 0; k < 4; k++) MarkersNFT[i]->matrix[j][k] = trackingTrans[j][k];

						MarkersNFT[i]->visible = true;

						// Filter the pose estimate.
						if (MarkersNFT[i]->ftmi && filterEnabled) {
							bool reset = false;

							if (MarkersNFT[i]->resetFilter) {
								reset = true;
								MarkersNFT[i]->resetFilter = false;
							}

							if (arFilterTransMat(MarkersNFT[i]->ftmi, MarkersNFT[i]->matrix, reset) < 0) {
								//Do nothing for now....
							}
						}

						memcpy(patt_trans, MarkersNFT[i]->matrix, sizeof(ARdouble) * 3 * 4);

						arUtilMatInv(patt_trans, patt_trans_inv);
						arUtilMat2QuatPos(patt_trans_inv, quaternions, positions);

						MarkersNFT[i]->position = FVector(patt_trans[0][3], patt_trans[1][3], -patt_trans[2][3]);
						MarkersNFT[i]->rotation = FQuat(quaternions[0], quaternions[1], quaternions[2], quaternions[3]).Rotator();

						//Camera position, rotation
						MarkersNFT[i]->cameraPosition = FVector(-positions[0], positions[1], positions[2]);
						arUtilMat2QuatPos(patt_trans, quaternions, positions);
						MarkersNFT[i]->cameraRotation = FQuat(quaternions[0], quaternions[1], quaternions[2], quaternions[3]).Rotator();

						pagesTracked++;
					}
				}
				page++;
		}
		kpmRequired = (pagesTracked < (nftMultiMode ? page : 1));

	} // threadHandle
}


#ifdef IPHONE_ONLY
FString FARToolkitDevice::ConvertToIOSPath(const FString& Filename, bool bForWrite)
{
	FString Result = Filename;
	Result.ReplaceInline(TEXT("../"), TEXT(""));
	Result.ReplaceInline(TEXT(".."), TEXT(""));
	Result.ReplaceInline(FPlatformProcess::BaseDir(), TEXT(""));

	if(bForWrite)
	{
		static FString WritePathBase = FString([NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0]) + TEXT("/");
		return WritePathBase + Result;
	}
	else
	{
		// if filehostip exists in the command line, cook on the fly read path should be used
		FString Value;
		// Cache this value as the command line doesn't change...
		static bool bHasHostIP = FParse::Value(FCommandLine::Get(), TEXT("filehostip"), Value) || FParse::Value(FCommandLine::Get(), TEXT("streaminghostip"), Value);
		static bool bIsIterative = FParse::Value(FCommandLine::Get(), TEXT("iterative"), Value);
		if (bHasHostIP)
		{
			static FString ReadPathBase = FString([NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0]) + TEXT("/");
			return ReadPathBase + Result.ToLower();
		}
		else if (bIsIterative)
		{
			static FString ReadPathBase = FString([NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0]) + TEXT("/");
			return ReadPathBase + Result.ToLower();
		}
		else
		{
			static FString ReadPathBase = FString([[NSBundle mainBundle] bundlePath]) + TEXT("/cookeddata/");
			iPhoneLaunched = false;
			return ReadPathBase + Result;
		}
	}

	return Result;
}
#endif

void FARToolkitDevice::SetFilter(bool enabled, int sampleRate, int cutOffFreq) {
	//Sample rate
	if (sampleRate != 0) {
		this->filterSampleRate = sampleRate;
	}else {
		this->filterSampleRate = 1;
	}

	//Cut off frequency
	if (cutOffFreq != 0) {
		this->filterCutOffFreq = cutOffFreq;
	}else {
		this->filterCutOffFreq = 1;
	}

	//Filter on/off
	this->filterEnabled = enabled;

	//Apply filter for all NFT markers
	for (int i = 0; i < this->MarkersNFT.Num(); i++) {
		ApplyFilter(this->MarkersNFT[i]);
	}

	//Apply filter for all fiducial markers
	for (int i = 0; i < this->Markers.Num(); i++) {
		ApplyFilter(this->Markers[i]);
	}
	
}
void FARToolkitDevice::ApplyFilter(FMarker* marker) {
	marker->resetFilter = true;
	if (this->filterEnabled) {
		if (marker->ftmi != 0) {
			arFilterTransMatSetParams(marker->ftmi,this->filterSampleRate, this->filterCutOffFreq);
		}else {
			marker->ftmi = arFilterTransMatInit(this->filterSampleRate, this->filterCutOffFreq);
		}
	} else {
		
		marker->ftmi = 0;
	}
}

void FARToolkitDevice::GetFilter(bool& enabled, int& sampleRate, int& cutOffFreq) {
	sampleRate=this->filterSampleRate;
	cutOffFreq = this->filterCutOffFreq;
	enabled=this->filterEnabled;
}

void FARToolkitDevice::TogglePause(){
#ifdef IPHONE_ONLY    
        UIApplicationState state = [[UIApplication sharedApplication] applicationState];
        if (state == UIApplicationStateBackground || state == UIApplicationStateInactive)
        {
            if (!paused){
                //Stop camera
                arVideoCapStop();
                paused=true;
            }
        } else{
            if (paused){

                paused=false;
                //Restart camera feed
                arVideoCapStart();
             }
        }
#endif
}

int FARToolkitDevice::GetCameraResX() {
	return this->mWebcamResX;
}

int FARToolkitDevice::GetCameraResY() {
	return this->mWebcamResY;
	
}

#if defined _WIN64
void FARToolkitDevice::DisplayDeviceInformation(IEnumMoniker *pEnum)
{
	IMoniker *pMoniker = NULL;

	while (pEnum->Next(1, &pMoniker, NULL) == S_OK)
	{
		numWebcams++;
		IPropertyBag *pPropBag;
		HRESULT hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
		if (FAILED(hr))
		{
			pMoniker->Release();
			continue;
		}

		VARIANT var;
		VariantInit(&var);

		// Get description or friendly name.
		hr = pPropBag->Read(L"Description", &var, 0);
		if (FAILED(hr))
		{
			hr = pPropBag->Read(L"FriendlyName", &var, 0);
		}
		if (SUCCEEDED(hr))
		{
			UE_LOG(ARToolkit, Log, TEXT("%s\n"), var.bstrVal);
			VariantClear(&var);
		}

		hr = pPropBag->Write(L"FriendlyName", &var);

		// WaveInID applies only to audio capture devices.
		hr = pPropBag->Read(L"WaveInID", &var, 0);
		if (SUCCEEDED(hr))
		{
			UE_LOG(ARToolkit,Log,TEXT("WaveIn ID: %d\n"), var.lVal);
			VariantClear(&var);
		}

		hr = pPropBag->Read(L"DevicePath", &var, 0);
		if (SUCCEEDED(hr))
		{
			// The device path is not intended for display.
			UE_LOG(ARToolkit, Log, TEXT("Device path: %s\n"), var.bstrVal);
			VariantClear(&var);
		}

		pPropBag->Release();
		pMoniker->Release();
	}
}
#endif

#if defined _WIN64
HRESULT FARToolkitDevice::EnumerateDevices(REFGUID category, IEnumMoniker **ppEnum)
{
	// Create the System Device Enumerator.
	ICreateDevEnum *pDevEnum;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
		CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));

	if (SUCCEEDED(hr))
	{
		// Create an enumerator for the category.
		hr = pDevEnum->CreateClassEnumerator(category, ppEnum, 0);
		if (hr == S_FALSE)
		{
			hr = VFW_E_NOT_FOUND;  // The category is empty. Treat as an error.
		}
		pDevEnum->Release();
	}
	return hr;
}
#endif


EDeviceOrientation FARToolkitDevice::GetDeviceOrientation() {
	return this->deviceOrientation;
}