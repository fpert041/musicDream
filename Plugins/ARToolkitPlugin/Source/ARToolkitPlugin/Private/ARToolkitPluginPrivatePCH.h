//
// Copyright 2015 Adam Horvath - WWW.UNREAL4AR.COM - info@unreal4ar.com - All Rights Reserved.
//
#include "IARToolkitPlugin.h"

// You should place include statements to your module's private header files here.  You only need to
// add includes for headers that are used in most of your module's source files though.

#include "Engine.h"

#ifdef _WIN64
	#include "AllowWindowsPlatformTypes.h"

	#include <dshow.h>
	#pragma comment(lib, "strmiids")
#endif

#if defined __APPLE__
	#import <AVFoundation/AVFoundation.h>
	#import <CoreVideo/CoreVideo.h>
#endif



#include <AR/video.h>
#include <AR/ar.h>
#include <AR/config.h>
#include <AR/param.h>

//NFT
#include <AR/arMulti.h>
#include <AR/arFilterTransMat.h>
#include <AR2/tracking.h>

//KPM
#include "trackingSub.h"

#ifdef _WIN64
	#include "HideWindowsPlatformTypes.h"
#endif

