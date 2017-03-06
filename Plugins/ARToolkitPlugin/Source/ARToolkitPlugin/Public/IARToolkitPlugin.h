//
// Copyright 2016 Adam Horvath - WWW.UNREAL4AR.COM - info@unreal4ar.com - All Rights Reserved.
//

#pragma once

#include "ModuleManager.h"


/**
 * The public interface to this module.  In most cases, this interface is only public to sibling modules 
 * within this plugin.
 */
class IARToolkitPlugin : public IModuleInterface
{

public:

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline IARToolkitPlugin& Get()
	{
		return FModuleManager::LoadModuleChecked< IARToolkitPlugin >( "ARToolkitPlugin" );
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "ARToolkitPlugin" );
	}

	FORCEINLINE TSharedPtr<class FARToolkitDevice> GetARToolkitDevice() const
	{
		return ARToolkitDevice;
	}

	/**
	* Simple helper function to get the device currently active.
	* @return	Pointer to the ARToolkitDevice, or nullptr if Device is not available.
	*/
	static FARToolkitDevice* GetARToolkitDeviceSafe()
	{
#if WITH_EDITOR
		FARToolkitDevice* ARToolkitDevice = IARToolkitPlugin::IsAvailable() ? IARToolkitPlugin::Get().GetARToolkitDevice().Get() : nullptr;
#else
		FARToolkitDevice* ARToolkitDevice = IARToolkitPlugin::Get().GetARToolkitDevice().Get();
#endif
		return ARToolkitDevice;
	}


protected:
	/**
	* Reference to the actual ARToolkitDevice, grabbed through the GetKinectV2Device() interface, and created and destroyed in Startup/ShutdownModule
	*/
	TSharedPtr<class FARToolkitDevice> ARToolkitDevice;
};


