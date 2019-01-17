// Fill out your copyright notice in the Description page of Project Settings.

#include "PointAndPixelUtility.h"
#include "Runtime/Engine/Classes/Engine/TextureRenderTarget2D.h"
#include "Runtime/Engine/Public/SceneView.h"

bool UPointAndPixelUtility::calcBoundingFromViewInfo(USceneCaptureComponent2D * RenderComponent, FVector Origin, FVector Extend, FBox2D & BoxOut, TArray<FVector>& Points, TArray<FVector2D>& Points2D, int borderClamp)
{
	bool isCompletelyInView = true;
	// get render target for texture size
	UTextureRenderTarget2D* RenderTexture = RenderComponent->TextureTarget;

	// initialise viewinfo for projection matrix
	FMinimalViewInfo Info;
	Info.Location = RenderComponent->GetComponentTransform().GetLocation();
	Info.Rotation = RenderComponent->GetComponentTransform().GetRotation().Rotator();
	Info.FOV = RenderComponent->FOVAngle;
	Info.ProjectionMode = RenderComponent->ProjectionType;
	Info.AspectRatio = float(RenderTexture->SizeX) / float(RenderTexture->SizeY);
	Info.OrthoNearClipPlane = 1;
	Info.OrthoFarClipPlane = 1000;
	Info.bConstrainAspectRatio = true;
	// calculate 3D corner Points of bounding box
	Points.Add(Origin + FVector(Extend.X, Extend.Y, Extend.Z));
	Points.Add(Origin + FVector(-Extend.X, Extend.Y, Extend.Z));
	Points.Add(Origin + FVector(Extend.X, -Extend.Y, Extend.Z));
	Points.Add(Origin + FVector(-Extend.X, -Extend.Y, Extend.Z));
	Points.Add(Origin + FVector(Extend.X, Extend.Y, -Extend.Z));
	Points.Add(Origin + FVector(-Extend.X, Extend.Y, -Extend.Z));
	Points.Add(Origin + FVector(Extend.X, -Extend.Y, -Extend.Z));
	Points.Add(Origin + FVector(-Extend.X, -Extend.Y, -Extend.Z));
	// initialize pixel values
	FVector2D MinPixel(RenderTexture->SizeX, RenderTexture->SizeY);
	FVector2D MaxPixel(0, 0);
	FIntRect ScreenRect(0, 0, RenderTexture->SizeX, RenderTexture->SizeY);
	// initialize projection data for sceneview
	FSceneViewProjectionData ProjectionData;
	ProjectionData.ViewOrigin = Info.Location;
	// do some voodoo rotation that is somehow mandatory and stolen from UGameplayStatics::ProjectWorldToScreen
	ProjectionData.ViewRotationMatrix = FInverseRotationMatrix(Info.Rotation) * FMatrix(
		FPlane(0, 0, 1, 0),
		FPlane(1, 0, 0, 0),
		FPlane(0, 1, 0, 0),
		FPlane(0, 0, 0, 1));
	if (RenderComponent->bUseCustomProjectionMatrix == true) {
		ProjectionData.ProjectionMatrix = RenderComponent->CustomProjectionMatrix;
	}
	else {
		ProjectionData.ProjectionMatrix = Info.CalculateProjectionMatrix();;
	}
	ProjectionData.SetConstrainedViewRectangle(ScreenRect);
	// Project Points to pixels and get the corner pixels
	for (FVector& Point : Points) {
		FVector2D Pixel(0, 0);
		FSceneView::ProjectWorldToScreen((Point), ScreenRect, ProjectionData.ComputeViewProjectionMatrix(), Pixel);
		Points2D.Add(Pixel);
		MaxPixel.X = FMath::Max(Pixel.X, MaxPixel.X);
		MaxPixel.Y = FMath::Max(Pixel.Y, MaxPixel.Y);
		MinPixel.X = FMath::Min(Pixel.X, MinPixel.X);
		MinPixel.Y = FMath::Min(Pixel.Y, MinPixel.Y);
	}

	BoxOut = FBox2D(MinPixel, MaxPixel);
	// clamp min point
	if (BoxOut.Min.X < borderClamp) {
		BoxOut.Min.X = borderClamp;
		isCompletelyInView = false;
	}
	else if (BoxOut.Min.X > RenderTexture->SizeX - (borderClamp * 2)) {
		BoxOut.Min.X = RenderTexture->SizeX - (borderClamp * 2);
		isCompletelyInView = false;
	}
	if (BoxOut.Min.Y < borderClamp) {
		BoxOut.Min.Y = borderClamp;
		isCompletelyInView = false;
	}
	else if (BoxOut.Min.Y > RenderTexture->SizeY - (borderClamp * 2)) {
		BoxOut.Min.Y = RenderTexture->SizeY - (borderClamp * 2);
		isCompletelyInView = false;
	}
	// clamp max point
	if (BoxOut.Max.X > RenderTexture->SizeX - borderClamp) {
		BoxOut.Max.X = RenderTexture->SizeX - borderClamp;
		isCompletelyInView = false;
	}
	else if (BoxOut.Max.X < (borderClamp * 2)) {
		BoxOut.Max.X = borderClamp * 2;
		isCompletelyInView = false;
	}
	if (BoxOut.Max.Y > RenderTexture->SizeY - borderClamp) {
		BoxOut.Max.Y = RenderTexture->SizeY - borderClamp;
		isCompletelyInView = false;
	}
	else if (BoxOut.Max.Y < (borderClamp * 2)) {
		BoxOut.Max.Y = borderClamp * 2;
		isCompletelyInView = false;
	}
	return isCompletelyInView;
}

bool UPointAndPixelUtility::DeprojectSceneToWorld(USceneCaptureComponent2D * RenderComponent, FBox2D Roi, TArray<FVector>& Points, TArray<FColor>& Colors, bool cameraRelative)
{
	// get render target for texture size
	UTextureRenderTarget2D* RenderTexture = RenderComponent->TextureTarget;
	FRenderTarget *RenderTarget = RenderTexture->GameThread_GetRenderTargetResource();

	if (Roi.Min.X < 0 || Roi.Max.X > RenderTexture->SizeX || Roi.Min.Y < 0 || Roi.Max.Y > RenderTexture->SizeY) {
		return false;
	}

	// initialise viewinfo for projection matrix
	FMatrix ProjectionMatrix;
	if (RenderComponent->bUseCustomProjectionMatrix) {
		ProjectionMatrix = RenderComponent->CustomProjectionMatrix.Inverse();
	}
	else {
		FMinimalViewInfo Info;
		Info.Location = RenderComponent->GetComponentTransform().GetLocation();
		Info.Rotation = RenderComponent->GetComponentTransform().GetRotation().Rotator();
		Info.FOV = RenderComponent->FOVAngle;
		Info.ProjectionMode = RenderComponent->ProjectionType;
		Info.AspectRatio = float(RenderTexture->SizeX) / float(RenderTexture->SizeY);
		Info.OrthoNearClipPlane = 1;
		Info.OrthoFarClipPlane = 1000;
		Info.bConstrainAspectRatio = true;
		ProjectionMatrix = Info.CalculateProjectionMatrix().Inverse();
	}
	// get colors
	TArray<FLinearColor> FormatedImageData;
	RenderTarget->ReadLinearColorPixels(FormatedImageData);

	// init deprojection
	float u = RenderTexture->SizeX / 2.0;
	float v = RenderTexture->SizeY / 2.0;
	FVector MiddlePoint, MiddleDirection;
	FIntRect ScreenSize(0, 0, (int)RenderTexture->SizeX, (int)RenderTexture->SizeY);
	FSceneView::DeprojectScreenToWorld(FVector2D(u, v), ScreenSize, ProjectionMatrix, MiddlePoint, MiddleDirection);

	// init arrays
	int size = Roi.GetSize().Size();
	Points.Reset(size);
	Colors.Reset(size);
	// fill arrays
	for (int y = Roi.Min.Y; y <= Roi.Max.Y; y++) {
		for (int x = Roi.Min.X; x <= Roi.Max.X; x++) {

			int i = x + y * RenderTexture->SizeX;
			// calculations in cm
			FColor color = FormatedImageData[i].ToFColor(true);
			Colors.Add(color);

			FVector PointLocation, PointDirection;
			FSceneView::DeprojectScreenToWorld(FVector2D(x, y), ScreenSize, ProjectionMatrix, PointLocation, PointDirection);
			if (cameraRelative) {
				// get point coordinates in usual coordinate system (right-handed, z is forward vector)
				float z = FormatedImageData[i].A;
				float x3d = (PointLocation.X - MiddlePoint.X) * (z / PointLocation.Z);
				float y3d = -(PointLocation.Y - MiddlePoint.Y) * (z / PointLocation.Z);
				if (z > 1.0) {
					// convert to unreal left-handed system with x is forward vector
					Points.Add(FVector(z, x3d, -y3d));
				}
			}
			else {
				Points.Add(RenderComponent->GetComponentTransform().TransformPosition(PointLocation));
			}
		}
	}

	return true;
}
