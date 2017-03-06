//
// Copyright 2015 Adam Horvath - WWW.UNREAL4AR.COM - info@unreal4ar.com - All Rights Reserved.
//

#pragma once
#include "ARToolkitFunctionLibrary.h"
#include "Engine.h"


//ARToolkit log
DECLARE_LOG_CATEGORY_EXTERN(ARToolkit, Log, All);

//NFT
#define PAGES_MAX               10          // Maximum number of NFT pages expected. 

#ifdef TARGET_OS_MAC
	#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
	// iOS code here
		#define IPHONE_ONLY 1
	#else
	// OS X code here
		#define MAC_ONLY 1
	#endif
#endif

#if defined __ANDROID__ || defined __APPLE__
typedef unsigned char BYTE;
typedef struct tagRGBQUAD {
	BYTE rgbBlue;
	BYTE rgbGreen;
	BYTE rgbRed;
	BYTE rgbReserved;
} RGBQUAD;
#endif

extern FString GFilePathBase;

class FARToolkitDevice 
{
	
public:
	FARToolkitDevice();
	~FARToolkitDevice();

	/** Startup the device, and do any initialization that may be needed */
	bool StartupDevice();

	/** Tear down the device */
	void ShutdownDevice();
	
	/** Update device on each Tick */
	void UpdateDevice();

	int YUVtoRGB(int y, int u, int v);
	
	
	
	UTexture2D* GetWebcamTexture();

	FMarker* GetMarker(uint8 markerId);
	FMarker* GetMarkerNFT(uint8 markerId);
	FMarker* GetMarkerMATRIX(uint8 markerId);

	
bool GetRelativeTransformation(uint8 MarkerID1, uint8 MarkerID2, FVector& RelativePosition, FRotator& RelativeRotation);
	bool GetRelativeTransformationNFT(uint8 MarkerID1, uint8 MarkerIdNFT, FVector& RelativePosition, FRotator& RelativeRotation);

	
	bool Init(bool showPIN, int devNum, EDeviceOrientation deviceOri, EArPatternDetectionMode detectionMode, int32&WebcamResX, int32&WebcamResY, bool&fr);
	bool LoadMarkers(TArray<FString> markerNames);
	bool LoadMarkersNFT(TArray<FString> markerNames);
	int  UnloadNFTData(void);
    void TogglePause();
	void SetDebugMode(bool mode);
	void Cleanup(void);
	void SetThresholdMode(EArLabelingThresholdMode mode);
	void SetThreshold(int thresholdValue);
	int  GetThreshold();

	void SetFilter(bool enabled, int sampleRate, int cutOffFreq);
	void GetFilter(bool& enabled, int& sampleRate, int& cutOffFreq);

	int GetCameraResX();
	int GetCameraResY();

	EDeviceOrientation GetDeviceOrientation();
	
protected:
	
	void UpdateTexture(UTexture2D* Texture, int32 MipIndex, uint32 NumRegions, FUpdateTextureRegion2D* Regions, uint32 SrcPitch, uint32 SrcBpp, uint8* SrcData, bool bFreeData);
	void UpdateTextureWindows();
    void UpdateTextureApple();
	void UpdateTextureAndroid();
	void DetectMarkers();
	void DetectMarkersNFT();
	void DetectMarkersMATRIX();
	uint8* GetDebugTexture(int width, int height);
	
#if defined _WIN64
	void DisplayDeviceInformation(IEnumMoniker *pEnum);
	HRESULT EnumerateDevices(REFGUID category, IEnumMoniker **ppEnum);
#endif

	void ApplyFilter(FMarker* marker);
	FUpdateTextureRegion2D *echoUpdateTextureRegion;
	TArray<FColor> rawData;


	
   
#ifdef IPHONE_ONLY
	FString ConvertToIOSPath(const FString& Filename, bool bForWrite);
#endif

	UTexture2D*					WebcamTexture;
	UTexture2D*					DummyTexture;
	RGBQUAD*					WebcamRGBX;
    
	static ARHandle				*arHandle;
	static ARPattHandle			*arPattHandle;
	static AR3DHandle			*ar3DHandle;

	static ARUint8				*gARTImage;

	static int					xsize, ysize;
	static int					mWebcamResX, mWebcamResY;
	
	static double				patt_width;
	static ARParamLT			*gCparamLT;

	TArray<FMarker*>			Markers;
	TArray<FMarker*>			MarkersNFT;
	TArray<FMarker*>			MarkersMATRIX;

	bool initiated;
	bool paused;

	static FString DataPath;
    
	// NFT.
	static THREAD_HANDLE_T     *threadHandle;
	static AR2HandleT          *ar2Handle;
	static KpmHandle           *kpmHandle;
	static int                  surfaceSetCount;
	static AR2SurfaceSetT      *surfaceSet[PAGES_MAX];
	

	static int					debugMode;
	static int				    threshold;

	static int					filterSampleRate;
	static int					filterCutOffFreq;
	static bool					filterEnabled;

	int							numWebcams;

	bool						iPhoneLaunched;

	EArPatternDetectionMode		patternDetectionMode;
	EDeviceOrientation			deviceOrientation;

	
	//Multi NFT
	bool						nftMultiMode;
	bool						kpmRequired;
	bool						kpmBusy;

	
};
