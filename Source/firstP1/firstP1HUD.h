// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once 
#include "GameFramework/HUD.h"
#include "firstP1HUD.generated.h"

UCLASS()
class AfirstP1HUD : public AHUD
{
	GENERATED_BODY()

public:
	AfirstP1HUD();

	/** Primary draw call for the HUD */
	virtual void DrawHUD() override;

private:
	/** Crosshair asset pointer */
	class UTexture2D* CrosshairTex;

};

