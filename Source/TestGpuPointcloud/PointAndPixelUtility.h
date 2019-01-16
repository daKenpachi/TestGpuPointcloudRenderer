// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Runtime/Engine/Classes/Components/SceneCaptureComponent2D.h"
#include "PointAndPixelUtility.generated.h"

/**
 * 
 */
UCLASS()
class TESTGPUPOINTCLOUD_API UPointAndPixelUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	UFUNCTION(BluePrintCallable, Category = "PixelUtility")
		static bool calcBoundingFromViewInfo(USceneCaptureComponent2D* RenderComponent, FVector Origin, FVector Extend, FBox2D& BoxOut, TArray<FVector>& Points, TArray<FVector2D>& Points2D, int borderClamp = 0);

	UFUNCTION(BluePrintCallable, Category = "PixelUtility")
		static bool DeprojectSceneToWorld(USceneCaptureComponent2D* RenderComponent, FBox2D Roi, TArray<FVector>& Points, TArray<FColor>& Colors, bool cameraRelative = true);
	
	
};
