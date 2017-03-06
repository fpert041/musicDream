//
// Copyright 2015 Adam Horvath - WWW.UNREAL4AR.COM - info@unreal4ar.com - All Rights Reserved.
//

#include "ARToolkitPluginPrivatePCH.h"
#include "ARToolkitComponent.h"

#include "IARToolkitPlugin.h"
#include "ARToolkitDevice.h"



UARToolkitComponent::UARToolkitComponent(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Make sure this component ticks
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	bAutoActivate = true;

	

}



void UARToolkitComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction){
	
	
	IARToolkitPlugin::GetARToolkitDeviceSafe()->UpdateDevice();
	

}


UTexture2D* UARToolkitComponent::GetCameraFrame(){
	return IARToolkitPlugin::GetARToolkitDeviceSafe()->GetWebcamTexture();
}

void UARToolkitComponent::SetDebugMode(bool DebugMode) {
	IARToolkitPlugin::GetARToolkitDeviceSafe()->SetDebugMode(DebugMode);
}

void UARToolkitComponent::SetThresholdMode(EArLabelingThresholdMode mode) {
	IARToolkitPlugin::GetARToolkitDeviceSafe()->SetThresholdMode(mode);
}

void UARToolkitComponent::SetThreshold(uint8 threshold) {
	IARToolkitPlugin::GetARToolkitDeviceSafe()->SetThreshold(threshold);
}

void UARToolkitComponent::GetThreshold(uint8& threshold) {
	threshold = IARToolkitPlugin::GetARToolkitDeviceSafe()->GetThreshold();
}

void UARToolkitComponent::SetFilter(bool enabled, uint8 sampleRate, uint8 cutOffFreq) {
	IARToolkitPlugin::GetARToolkitDeviceSafe()->SetFilter(enabled, sampleRate, cutOffFreq);
}

void UARToolkitComponent::GetFilter(bool& enabled, uint8& sampleRate, uint8& cutOffFreq) {
	IARToolkitPlugin::GetARToolkitDeviceSafe()->GetFilter(enabled, (int&)sampleRate, (int&)cutOffFreq);
}

UMaterialInstanceDynamic* UARToolkitComponent::CreateDynamicMaterialInstance(UStaticMeshComponent* Mesh, UMaterial* SourceMaterial){
	UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(SourceMaterial, this);
	Mesh->SetMaterial(0, DynamicMaterial);
	return DynamicMaterial;
}

void UARToolkitComponent::SetTextureParameterValue(UMaterialInstanceDynamic* SourceMaterial, UTexture2D* Texture, FName Param){
	SourceMaterial->SetTextureParameterValue(Param, Texture);

}

void UARToolkitComponent::GetMarker(uint8 markerId, FVector& Position, FRotator& Rotation, FVector& CameraPosition, FRotator& CameraRotation, bool& Visible){
	FMarker* marker=IARToolkitPlugin::GetARToolkitDeviceSafe()->GetMarker(markerId);
	ProcessMarker(marker, Position, Rotation, CameraPosition, CameraRotation, Visible);
}

void UARToolkitComponent::GetMarkerMATRIX(uint8 markerId, FVector& Position, FRotator& Rotation, FVector& CameraPosition, FRotator& CameraRotation, bool& Visible) {
	FMarker* marker = IARToolkitPlugin::GetARToolkitDeviceSafe()->GetMarkerMATRIX(markerId);
	ProcessMarker(marker, Position, Rotation, CameraPosition, CameraRotation, Visible);
}

void UARToolkitComponent::GetMarkerNFT(uint8 markerId, FVector& Position, FRotator& Rotation, FVector& CameraPosition, FRotator& CameraRotation, bool& Visible){
	FMarker* marker = IARToolkitPlugin::GetARToolkitDeviceSafe()->GetMarkerNFT(markerId);
	ProcessMarker(marker, Position, Rotation, CameraPosition, CameraRotation, Visible);
}

void UARToolkitComponent::ProcessMarker(FMarker* marker, FVector& Position, FRotator& Rotation, FVector& CameraPosition, FRotator& CameraRotation, bool& Visible) {
	
	//Position landscape
	Position = marker->position;

	//Position portrait mode
	if (IARToolkitPlugin::GetARToolkitDeviceSafe()->GetDeviceOrientation() == EDeviceOrientation::PORTRAIT) {
		Position = FRotator(0, 90, 0).RotateVector(marker->position);
	}

	//Rotation
	FRotator rotTemp;
	rotTemp.Yaw = marker->rotation.Yaw;
	rotTemp.Pitch = -marker->rotation.Pitch;
	rotTemp.Roll = 180 - marker->rotation.Roll;
	Rotation = rotTemp;


	//Rotation portrait mode
	if (IARToolkitPlugin::GetARToolkitDeviceSafe()->GetDeviceOrientation() == EDeviceOrientation::PORTRAIT) {
		Rotation = (FQuat(rotTemp)*FQuat(FRotator(0, 90, 0))).Rotator();
	}


	//Visibility
	Visible = marker->visible;


	//Camera rotation landscape
	rotTemp.Roll = marker->cameraRotation.Roll;
	rotTemp.Pitch = -marker->cameraRotation.Pitch;
	rotTemp.Yaw = -marker->cameraRotation.Yaw;
	CameraRotation = (FQuat(rotTemp)*FQuat(FRotator(90, 0, -90))).Rotator();

	//Camera rotation portrait
	if (IARToolkitPlugin::GetARToolkitDeviceSafe()->GetDeviceOrientation() == EDeviceOrientation::PORTRAIT) {
		CameraRotation.Roll = CameraRotation.Roll - 90;
	}

	//Camera position landscape / portrait
	CameraPosition = marker->cameraPosition;



}

void UARToolkitComponent::GetRelativeTransformation(uint8 MarkerID1, uint8 MarkerID2, FVector& RelativePos, FRotator& RelativeRot, bool& result) {
	result = IARToolkitPlugin::GetARToolkitDeviceSafe()->GetRelativeTransformation(MarkerID1, MarkerID2, RelativePos, RelativeRot);

	RelativeRot.Roll = RelativeRot.Roll;
	RelativeRot.Pitch = -RelativeRot.Pitch;
	RelativeRot.Yaw = -RelativeRot.Yaw;
}

void UARToolkitComponent::GetRelativeTransformationNFT(uint8 MarkerID1, uint8 MarkerID_NFT, FVector& RelativePos, FRotator& RelativeRot, bool& result) {
	result = IARToolkitPlugin::GetARToolkitDeviceSafe()->GetRelativeTransformation(MarkerID1, MarkerID_NFT, RelativePos, RelativeRot);

	RelativeRot.Roll = RelativeRot.Roll;
	RelativeRot.Pitch = -RelativeRot.Pitch;
	RelativeRot.Yaw = -RelativeRot.Yaw;
}

bool UARToolkitComponent::Init(bool showPIN, uint8 devNum, EDeviceOrientation deviceOrientation, EArPatternDetectionMode detectionMode, int32& WebcamResX, int32& WebcamResY, bool&FirstRunIOS) {
	
	switch (deviceOrientation) {
		
		case EDeviceOrientation::LANDSCAPE: {
			return IARToolkitPlugin::GetARToolkitDeviceSafe()->Init(showPIN, devNum, deviceOrientation,detectionMode, WebcamResX, WebcamResY, FirstRunIOS);
			break;
		}

		case EDeviceOrientation::PORTRAIT: {
			return IARToolkitPlugin::GetARToolkitDeviceSafe()->Init(showPIN, devNum, deviceOrientation, detectionMode, WebcamResY, WebcamResX, FirstRunIOS);
			break;
		}
	}

	return false;
}

void UARToolkitComponent::Cleanup(){
	IARToolkitPlugin::GetARToolkitDeviceSafe()->Cleanup();
}

void UARToolkitComponent::TogglePause(){
    IARToolkitPlugin::GetARToolkitDeviceSafe()->TogglePause();
}

bool UARToolkitComponent::LoadMarkers(TArray<FString> markers){
	return IARToolkitPlugin::GetARToolkitDeviceSafe()->LoadMarkers(markers);
}

bool UARToolkitComponent::LoadMarkersNFT(TArray<FString> markers){
	return IARToolkitPlugin::GetARToolkitDeviceSafe()->LoadMarkersNFT(markers);
}

void UARToolkitComponent::GetCameraResolution(int32& WebcamResX, int32& WebcamResY) {
	
	//PORTRAIT
	switch (IARToolkitPlugin::GetARToolkitDeviceSafe()->GetDeviceOrientation()) {

		case EDeviceOrientation::PORTRAIT: {
			WebcamResY = IARToolkitPlugin::GetARToolkitDeviceSafe()->GetCameraResX();
			WebcamResX = IARToolkitPlugin::GetARToolkitDeviceSafe()->GetCameraResY();
			break;
		}

		case EDeviceOrientation::LANDSCAPE: {
			//LANDSCAPE
			WebcamResX = IARToolkitPlugin::GetARToolkitDeviceSafe()->GetCameraResX();
			WebcamResY = IARToolkitPlugin::GetARToolkitDeviceSafe()->GetCameraResY();
			break;
		}

	}
	
}

EDeviceOrientation UARToolkitComponent::GetDeviceOrientation() {
	return IARToolkitPlugin::GetARToolkitDeviceSafe()->GetDeviceOrientation();
}