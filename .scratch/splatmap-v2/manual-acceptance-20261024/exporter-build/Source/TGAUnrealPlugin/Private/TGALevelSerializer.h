// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JsonObjectConverter.h"
#include "TGALevelSerializer.generated.h"

DECLARE_DELEGATE_ThreeParams(FComponentSerializationSignature, const UActorComponent*, TSharedPtr<FJsonObject>&, bool&);

USTRUCT()
struct FJsonTransform
{
	GENERATED_BODY()

	UPROPERTY()
	double X;
	UPROPERTY()
	double Y;
	UPROPERTY()
	double Z;
	UPROPERTY()
	double Pitch;
	UPROPERTY()
	double Yaw;
	UPROPERTY()
	double Roll;
	UPROPERTY()
	double ScaleX;
	UPROPERTY()
	double ScaleY;
	UPROPERTY()
	double ScaleZ;
};

USTRUCT()
struct FJsonTransformMatrix
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<double> TransformNoScale;

	UPROPERTY()
	TArray<double> Scale;
};

//
// USTRUCT()
// struct FJsonMatrix
// {
//   GENERATED_BODY();
//
//   UPROPERTY()
//   FMatrix Bengt;
//  
//   UPROPERTY()
//   double m00;
//   UPROPERTY()
//   double m01;
//   UPROPERTY()
//   double m02;
//   UPROPERTY()
//   double m03;
//   UPROPERTY()
//   double m10;
//   UPROPERTY()
//   double m11;
//   UPROPERTY()
//   double m12;
//   UPROPERTY()
//   double m13;
//   UPROPERTY()
//   double m20;
//   UPROPERTY()
//   double m21;
//   UPROPERTY()
//   double m22;
//   UPROPERTY()
//   double m23;
//   UPROPERTY()
//   double m30;
//   UPROPERTY()
//   double m31;
//   UPROPERTY()
//   double m32;
//   UPROPERTY()
//   double m33;
// };

USTRUCT()
struct FJsonVector
{
	GENERATED_BODY()

	UPROPERTY()
	double X;
	UPROPERTY()
	double Y;
	UPROPERTY()
	double Z;
};

// USTRUCT()
// struct FJsonLinearColor
// {
// 	GENERATED_BODY()
// 	
// 	UPROPERTY()
// 	TArray<double> Color;
// };


/**
 * 
 */
class TGALevelSerializer
{

	struct SerializationInfo
	{
		FComponentSerializationSignature Signature;
		uint32 TypeID;
		UClass* Class;
	};
 
	//TMap<UClass*, FComponentSerializationSignature::TMethodPtr<TGAComponentSerializer>> ComponentHandlers;
	TMap<UClass*, SerializationInfo> ComponentHandlers;

	void RegisterComponentHandler(UClass* Class, uint32 TypeID, FComponentSerializationSignature::TMethodPtr<TGALevelSerializer> Func);

	// Handles serialization via the TGASerializableInterface if implemented.
	void HandleCustomSerialization(UObject* Component, TSharedPtr<FJsonObject>& OutJson, bool& OutResult);
 
	void HandleSceneComponent(const UActorComponent* Component, TSharedPtr<FJsonObject>& OutJson, bool& OutResult);
	void HandleMeshComponent(const UActorComponent* Component, TSharedPtr<FJsonObject>& OutJson, bool& OutResult);
	void HandleStaticMeshComponent(const UActorComponent* Component, TSharedPtr<FJsonObject>& OutJson, bool& OutResult);
	void HandleSkeletalMeshComponent(const UActorComponent* Component, TSharedPtr<FJsonObject>& OutJson, bool& OutResult);
	void HandlePointLightComponent(const UActorComponent* Component, TSharedPtr<FJsonObject>& OutJson, bool& OutResult);
	void HandleSpotLightComponent(const UActorComponent* Component, TSharedPtr<FJsonObject>& OutJson, bool& OutResult);
	void HandleDirectionalLightComponent(const UActorComponent* Component, TSharedPtr<FJsonObject>& OutJson, bool& OutResult);
	void HandleBoxComponent(const UActorComponent* Component, TSharedPtr<FJsonObject>& OutJson, bool& OutResult);
	void HandleSphereComponent(const UActorComponent* Component, TSharedPtr<FJsonObject>& OutJson, bool& OutResult);
	void HandleCapsuleComponent(const UActorComponent* Component, TSharedPtr<FJsonObject>& OutJson, bool& OutResult);
	void HandleSpringArmComponent(const UActorComponent* Component, TSharedPtr<FJsonObject>& OutJson, bool& OutResult);
 
public:

	void Initialize();

	bool SerializeComponent(const ::AActor* Actor, ::UActorComponent* Component, TSharedPtr<FJsonObject>& OutJson);
	bool CanSerializeComponent(const ::AActor* Actor, const ::UActorComponent* Component) const;
	TSharedPtr<FJsonObject> TrySerializeField(const FField* Field, void const* FieldOwner, const AActor* Outer);

	TSharedPtr<FJsonObject> SerializeMaterial(const UMaterialInterface* Material, bool InstanceOnly) const;

	void HandleSkyLightComponent(const UActorComponent* Component, TSharedPtr<FJsonObject>& OutJson, bool& OutResult);
	void HandleCameraComponent(const UActorComponent* Component, TSharedPtr<FJsonObject>& OutJson, bool& OutResult);
	void HandleLandscapeComponent(const UActorComponent* Component, TSharedPtr<FJsonObject>& OutJson, bool& OutResult);
};
