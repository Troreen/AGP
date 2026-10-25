// Fill out your copyright notice in the Description page of Project Settings.


#include "TGALevelSerializer.h"

#include "../Public/TGAUnrealPlugin.h"
#include "Chaos/AABB.h"
#include "Chaos/AABB.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EditorFramework/AssetImportData.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureCube.h"
#include "Engine/Texture2D.h"
#include "GameFramework/SpringArmComponent.h"
#include "JsonBlueprintFunctionLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceConstant.h"
#include "LandscapeComponent.h"
#include "LandscapeLayerInfoObject.h"
#include "TGAPluginDeveloperSettings.h"
#include "TGASerializableInterface.h"
#include "UObject/UnrealTypePrivate.h"

PRAGMA_DISABLE_OPTIMIZATION_ACTUAL

void TGALevelSerializer::Initialize()
{
	uint32 TypeID = 0;
	RegisterComponentHandler(USceneComponent::StaticClass(), TypeID++, &TGALevelSerializer::HandleSceneComponent);
	RegisterComponentHandler(UStaticMeshComponent::StaticClass(), TypeID++, &TGALevelSerializer::HandleMeshComponent);
	RegisterComponentHandler(USkeletalMeshComponent::StaticClass(), TypeID++, &TGALevelSerializer::HandleMeshComponent);
	RegisterComponentHandler(UPointLightComponent::StaticClass(), TypeID++, &TGALevelSerializer::HandlePointLightComponent);
	RegisterComponentHandler(USpotLightComponent::StaticClass(), TypeID++, &TGALevelSerializer::HandleSpotLightComponent);
	RegisterComponentHandler(UDirectionalLightComponent::StaticClass(), TypeID++, &TGALevelSerializer::HandleDirectionalLightComponent);
	RegisterComponentHandler(UBoxComponent::StaticClass(), TypeID++, &TGALevelSerializer::HandleBoxComponent);
	RegisterComponentHandler(USphereComponent::StaticClass(), TypeID++, &TGALevelSerializer::HandleSphereComponent);
	RegisterComponentHandler(UCapsuleComponent::StaticClass(), TypeID++, &TGALevelSerializer::HandleCapsuleComponent);
	RegisterComponentHandler(USpringArmComponent::StaticClass(), TypeID++, &TGALevelSerializer::HandleSpringArmComponent);
	RegisterComponentHandler(USkyLightComponent::StaticClass(), TypeID++, &TGALevelSerializer::HandleSkyLightComponent);
	RegisterComponentHandler(UCameraComponent::StaticClass(), TypeID++, &TGALevelSerializer::HandleCameraComponent);
	// Keep camera ID 11 compatible with existing KnightHeart exports.
	RegisterComponentHandler(ULandscapeComponent::StaticClass(), TypeID++, &TGALevelSerializer::HandleLandscapeComponent);
}

void TGALevelSerializer::RegisterComponentHandler(UClass* Class, uint32 TypeID,
	FComponentSerializationSignature::TMethodPtr<TGALevelSerializer> Func)
{
	FComponentSerializationSignature Handler;
	Handler.BindRaw(this, Func);

	SerializationInfo Info;
	Info.Signature = Handler;
	Info.TypeID = TypeID;
	Info.Class = Class;

	ComponentHandlers.Add(Info.Class, Info);
}

void TGALevelSerializer::HandleCustomSerialization(UObject* Component, TSharedPtr<FJsonObject>& OutJson, bool& OutResult)
{
	if(!Component->Implements<UTGASerializableInterface>())
	{
		OutResult = false;
		return;
	}
	
	FJsonObjectWrapper Object{};
	ITGASerializableInterface::Execute_TGA_Serialize(Component, Object);
	if(!Object.JsonObject.IsValid())
	{
		Object.JsonObject = MakeShared<FJsonObject>();
	}

	OutJson = Object.JsonObject;
	OutResult = true;
}

TSharedPtr<FJsonObject> TGALevelSerializer::TrySerializeField(const FField* Field, void const* FieldOwner, const AActor* Outer)
{
	FString FieldName = Field->GetName();
	FString InternalFieldName = Field->GetAuthoredName();

	bool bWasIdentified = true;

	TSharedPtr<FJsonObject> PropertyJson = MakeShareable(new FJsonObject());
	PropertyJson->SetStringField("Name", FieldName);
	
	if(const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Field))
	{
		PropertyJson->SetStringField("Type", "bool");
		bool PropValue = false;
		BoolProperty->GetValue_InContainer(FieldOwner, &PropValue);
		PropertyJson->SetBoolField("Value", PropValue);
	}
	else if(const FIntProperty* IntProperty = CastField<FIntProperty>(Field))
	{
		PropertyJson->SetStringField("Type", "int");
		int PropValue = 0;
		IntProperty->GetValue_InContainer(FieldOwner, &PropValue);
		PropertyJson->SetNumberField("Value", PropValue);
	}
	else if(const FFloatProperty* FloatProperty = CastField<FFloatProperty>(Field))
	{
		PropertyJson->SetStringField("Type", "float");
		float PropValue = 0;
		FloatProperty->GetValue_InContainer(FieldOwner, &PropValue);
		PropertyJson->SetNumberField("Value", PropValue);
	}
	else if(const FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Field))
	{
		PropertyJson->SetStringField("Type", "double");
		double PropValue = 0;
		DoubleProperty->GetValue_InContainer(FieldOwner, &PropValue);
		PropertyJson->SetNumberField("Value", PropValue);
	}
	else if(const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Field))
	{
		TObjectPtr<UObject> PropValue = nullptr;
		ObjectProperty->GetValue_InContainer(FieldOwner, &PropValue);
		UClass* PropValueClass = PropValue->GetClass();
		FString ClassName = PropValueClass->GetName();

		const UActorComponent* ActorComponent = Cast<UActorComponent>(PropValue.Get());
		if(!ActorComponent)
		{
			PropertyJson->SetStringField("Type", "object");
			PropertyJson->SetStringField("Value", IsValid(PropValue) ? GetNameSafe(PropValue) : "None");
		}
		else
		{
			bWasIdentified = false;
		}
	}
	else if(const FStrProperty* StringProperty = CastField<FStrProperty>(Field))
	{
		FString PropValue;
		StringProperty->GetValue_InContainer(FieldOwner, &PropValue);
		PropertyJson->SetStringField("Type", "string");
		PropertyJson->SetStringField("Value", PropValue);
	}
	else if(const FTextProperty* TextProperty = CastField<FTextProperty>(Field))
	{
		FText PropValue;
		TextProperty->GetValue_InContainer(FieldOwner, &PropValue);
		PropertyJson->SetStringField("Type", "text");
		PropertyJson->SetStringField("Value", PropValue.ToString());
	}
	else if(const FNameProperty* NameProperty = CastField<FNameProperty>(Field))
	{
		FName PropValue;
		NameProperty->GetValue_InContainer(FieldOwner, &PropValue);
		PropertyJson->SetStringField("Type", "name");
		PropertyJson->SetStringField("Value", PropValue.ToString());
	}
	else if(const FStructProperty* StructProperty = CastField<FStructProperty>(Field))
	{
		// This is a Struct property like a Vector, Rotator, UStruct, Array etc.
		
		if(UScriptStruct* Struct = StructProperty->Struct)
		{
			FString StructName = Struct->GetName();
			
			PropertyJson->SetStringField("Type", StructName);

			TArray<TSharedPtr<FJsonValue>> StructProperties;			
			FField* StructChildProperty = Struct->ChildProperties;
			while(StructChildProperty)
			{
				TSharedPtr<FJsonObject> StructPropertyJson = TrySerializeField(StructChildProperty, Struct, Outer);
				if(StructPropertyJson)
				{
					StructProperties.Add(MakeShareable(new FJsonValueObject(StructPropertyJson)));
				}

				StructChildProperty = StructChildProperty->Next;
			}
			
			if(!StructProperties.IsEmpty())
			{
				PropertyJson->SetArrayField("Value", StructProperties);
			}
			else
			{
				PropertyJson->SetStringField("Value", "Could not serialize!");
			}
		}
	}
	else
	{
		FFieldClass* fieldClass = Field->GetClass();
		FString fieldClassName = fieldClass->GetName();

		int a = 1;
		
		bWasIdentified = false;
	}

	if(bWasIdentified)
	{
		return PropertyJson;
	}

	return nullptr;
}

TSharedPtr<FJsonObject> TGALevelSerializer::SerializeMaterial(const UMaterialInterface* Material, bool InstanceOnly) const
{
	TSharedPtr<FJsonObject> MaterialJson = MakeShareable(new FJsonObject());
	if(Material)
	{
		MaterialJson->SetStringField("Name", Material->GetName());
		const UMaterial* ParentMaterial = Material->GetMaterial();
		if (const UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(Material))
		{
			MaterialJson->SetStringField("Parent", MaterialInstance->Parent->GetName());
		}
		else
		{
			if (!ParentMaterial)
			{
				MaterialJson->SetStringField("Parent", "NOT SET IN UE!");
				TArray<TSharedPtr<FJsonValue>> JsonParams;
				MaterialJson->SetArrayField("Parameters", JsonParams);
				return MaterialJson;				
			}
			
			if (InstanceOnly)
			{
				MaterialJson->SetStringField("Parent", "Self");
				TArray<TSharedPtr<FJsonValue>> JsonParams;
				MaterialJson->SetArrayField("Parameters", JsonParams);
				return MaterialJson;
			}

			EMaterialDomain Domain = ParentMaterial->MaterialDomain;
			EMaterialShadingModel ShadingModel = Material->GetShadingModels().GetFirstShadingModel();
			EBlendMode BlendMode = Material->GetBlendMode();
			FLinearColor baseColor = ParentMaterial->GetEditorOnlyData()->BaseColor.Constant;

			MaterialJson->SetNumberField("Domain", static_cast<int>(Domain));
			MaterialJson->SetNumberField("ShadingModel", static_cast<int>(ShadingModel));
			MaterialJson->SetNumberField("BlendMode", static_cast<int>(BlendMode));

			TArray<TSharedPtr<FJsonValue>> JsonColorArray;
			JsonColorArray.Reserve(4);
			JsonColorArray.Add(MakeShared<FJsonValueNumber>(baseColor.R));
			JsonColorArray.Add(MakeShared<FJsonValueNumber>(baseColor.G));
			JsonColorArray.Add(MakeShared<FJsonValueNumber>(baseColor.B));
			JsonColorArray.Add(MakeShared<FJsonValueNumber>(baseColor.A));
			MaterialJson->SetArrayField("BaseColor", JsonColorArray);

			MaterialJson->SetNumberField("Metallic", ParentMaterial->GetEditorOnlyData()->Metallic.Constant);
			MaterialJson->SetNumberField("Roughness", ParentMaterial->GetEditorOnlyData()->Roughness.Constant);

			MaterialJson->SetBoolField("TwoSided", ParentMaterial->TwoSided);
		}

		struct ParameterData
		{
			int32 Index;
			FString Name;
			FHashedMaterialParameterInfo Info;
			EMaterialParameterType Type;
		};
		TArray<ParameterData> ParamInfos;

		TArray<FMaterialParameterInfo> TypeParamInfos;
		TArray<FGuid> ParamIds;			
		Material->GetAllScalarParameterInfo(TypeParamInfos, ParamIds);
		ParamInfos.Reserve(ParamInfos.Max() + TypeParamInfos.Num());
		for(const FMaterialParameterInfo& info : TypeParamInfos)
		{
			ParameterData data;
			data.Name = info.Name.ToString();
			data.Index = info.Index;
			data.Type = EMaterialParameterType::Scalar;
			data.Info = info;
			ParamInfos.Emplace(data);
		}

		TypeParamInfos.Empty();
		ParamIds.Empty();
		
		Material->GetAllVectorParameterInfo(TypeParamInfos, ParamIds);
		ParamInfos.Reserve(ParamInfos.Max() + TypeParamInfos.Num());
		for(const FMaterialParameterInfo& info : TypeParamInfos)
		{
			ParameterData data;
			data.Name = info.Name.ToString();
			data.Index = info.Index;
			data.Type = EMaterialParameterType::Vector;
			data.Info = info;
			ParamInfos.Emplace(data);
		}

		TypeParamInfos.Empty();
		ParamIds.Empty();
		
		Material->GetAllTextureParameterInfo(TypeParamInfos, ParamIds);
		ParamInfos.Reserve(ParamInfos.Max() + TypeParamInfos.Num());
		for(const FMaterialParameterInfo& info : TypeParamInfos)
		{
			ParameterData data;
			data.Name = info.Name.ToString();
			data.Index = info.Index;
			data.Type = EMaterialParameterType::Texture;
			data.Info = info;
			ParamInfos.Emplace(data);
		}
		
		Material->GetAllStaticSwitchParameterInfo(TypeParamInfos, ParamIds);
		ParamInfos.Reserve(ParamInfos.Max() + TypeParamInfos.Num());
		for(const FMaterialParameterInfo& info : TypeParamInfos)
		{
			ParameterData data;
			data.Name = info.Name.ToString();
			data.Index = info.Index;
			data.Type = EMaterialParameterType::StaticSwitch;
			data.Info = info;
			ParamInfos.Emplace(data);
		}

		ParamInfos.Sort([](const ParameterData& A, const ParameterData& B)
		{
			return A.Index < B.Index;
		});

		TArray<TSharedPtr<FJsonValue>> JsonParams;

		for(const ParameterData& Data : ParamInfos)
		{
			TSharedPtr<FJsonObject> ParamJson = MakeShareable(new FJsonObject());
			ParamJson->SetStringField("Name", Data.Name);
			ParamJson->SetNumberField("Type", static_cast<unsigned>(Data.Type));
			switch(Data.Type)
			{
			case EMaterialParameterType::Scalar:
				{
					float value = 0;
					Material->GetScalarParameterValue(Data.Info, value);
					// if (ParentMaterial)
					// {
					// 	float parentValue = 0;
					// 	ParentMaterial->GetScalarParameterValue(Data.Info, parentValue);
					// 	if (parentValue == value)
					// 		continue;
					// }
					ParamJson->SetNumberField("Value", value);
				}
				break;
			case EMaterialParameterType::Vector:
				{
					FLinearColor value = FLinearColor::Black;
					Material->GetVectorParameterValue(Data.Info, value);

					// if (ParentMaterial)
					// {
					// 	FLinearColor parentValue = FLinearColor::Black;
					// 	ParentMaterial->GetVectorParameterValue(Data.Info, parentValue);
					// 	if (parentValue == value)
					// 		continue;
					// }

					TArray<TSharedPtr<FJsonValue>> JsonColorArray;
					JsonColorArray.Reserve(4);
					JsonColorArray.Add(MakeShared<FJsonValueNumber>(value.R));
					JsonColorArray.Add(MakeShared<FJsonValueNumber>(value.G));
					JsonColorArray.Add(MakeShared<FJsonValueNumber>(value.B));
					JsonColorArray.Add(MakeShared<FJsonValueNumber>(value.A));
					ParamJson->SetArrayField("Value", JsonColorArray);
				}
				break;
			case EMaterialParameterType::Texture:
				{
					UTexture* value = nullptr;
					Material->GetTextureParameterValue(Data.Info, value);

					// if (ParentMaterial)
					// {
					// 	UTexture* parentValue = nullptr;
					// 	ParentMaterial->GetTextureParameterValue(Data.Info, parentValue);
					// 	if (parentValue == value)
					// 		continue;
					// }
					
					TSharedPtr<FJsonObject> TextureJson = MakeShareable(new FJsonObject());
					if(value)
					{
						TextureJson->SetStringField("Name", value->GetName());
						TextureJson->SetStringField("Path", value->GetPathName());
						TextureJson->SetStringField("AssetImportPath", value->AssetImportData->GetFirstFilename());
					}
					ParamJson->SetObjectField("Value", TextureJson);
				}
				break;
			case EMaterialParameterType::StaticSwitch:
				{
					bool value = false;
					FGuid outValueGuid;
					
					Material->GetStaticSwitchParameterValue(Data.Info, value, outValueGuid);

					// if (ParentMaterial)
					// {
					// 	FGuid outParentValueGuid;
					// 	bool parentValue = false;						
					// 	ParentMaterial->GetStaticSwitchParameterValue(Data.Info, parentValue, outParentValueGuid);
					// 	if (parentValue == value)
					// 		continue;
					// }
					
					ParamJson->SetBoolField("Value", value);
				}
				break;
			}
			JsonParams.Emplace(MakeShareable(new FJsonValueObject(ParamJson)));
		}

		MaterialJson->SetArrayField("Parameters", JsonParams);
	}

	return MaterialJson;
}

PRAGMA_DISABLE_OPTIMIZATION_ACTUAL

bool TGALevelSerializer::SerializeComponent(const AActor* Actor, UActorComponent* Component, TSharedPtr<FJsonObject>& OutComponentJson)
{
	bool Result = false;

	const UClass* ClassPtr = Component->GetClass();

	TArray<TSharedPtr<FJsonValue>> JsonComponents;

	const bool bHasHandler = ComponentHandlers.Contains(ClassPtr);
	const bool bHasCustomSerializer = Component->Implements<UTGASerializableInterface>();
	
	if(bHasHandler || bHasCustomSerializer)
	{
		OutComponentJson = MakeShareable(new FJsonObject());
		
		// Write common things
		OutComponentJson->SetStringField("Name", Component->GetName());
		OutComponentJson->SetStringField("Parent", "");
		
		if(bHasHandler)
		{
			const auto* Info = ComponentHandlers.Find(ClassPtr);
			OutComponentJson->SetNumberField("TypeID", Info->TypeID);
			//OutComponentJson->SetStringField("TypeName", Info->Class->GetName());
		}
		else
		{
			OutComponentJson->SetNumberField("TypeID", -1);
			OutComponentJson->SetStringField("CustomType", Component->GetClass()->GetName());
		}
		
		if(const USceneComponent* SceneComponent = Cast<USceneComponent>(Component))
		{
			if(const USceneComponent* ParentComponent = SceneComponent->GetAttachParent())
			{
				OutComponentJson->SetStringField("Parent", ParentComponent->GetName());
			}
			else
			{
				OutComponentJson->SetStringField("Parent", "");
			}
		}

		TArray<TSharedPtr<FJsonValue>> JsonProperties;
		if(!ClassPtr->IsNative())
		{
			const FField* ClassField = ClassPtr->ChildProperties;
			// while(ClassField)
			// {
			// 	if(const TSharedPtr<FJsonObject> PropertyJson = TrySerializeField(ClassField, Actor, Actor))
			// 	{
			// 		JsonProperties.Add(MakeShareable(new FJsonValueObject(PropertyJson)));	
			// 	}
			// 		
			// 	ClassField = ClassField->Next;
			// }
			FJsonObjectConverter Converter;
			while(ClassField)
			{
				FProperty* Prop = (FProperty*)ClassField;
				if(Prop)
				{
					FString FieldName = Prop->GetName();
					FString VariableName = Prop->GetAuthoredName();
					FString CPPType = Prop->GetCPPType();
					const void* Value = Prop->ContainerPtrToValuePtr<uint8>(Component);

					TSharedPtr<FJsonValue> JsonValue = Converter.UPropertyToJsonValue(Prop, Value);
					if (JsonValue.IsValid())
					{
						const TSharedPtr<FJsonObject> PropertyJson = MakeShareable(new FJsonObject());
						PropertyJson->SetStringField("Name", VariableName);
						PropertyJson->SetStringField("Type", CPPType);
						PropertyJson->SetField("Value", JsonValue);
						//PropertyJson->SetStringField()
						JsonProperties.Add(MakeShareable(new FJsonValueObject(PropertyJson)));
					}
				}				
					
				ClassField = ClassField->Next;
			}
		}
		
		if(!JsonProperties.IsEmpty())
		{
			OutComponentJson->SetArrayField("Properties", JsonProperties);	
		}

		TArray<TSharedPtr<FJsonValue>> JsonTags;
		if(!Component->ComponentTags.IsEmpty())
		{
			for(const FName& Tag : Component->ComponentTags)
			{
				JsonTags.Add(MakeShareable(new FJsonValueString(Tag.ToString())));
			}
		}

		OutComponentJson->SetArrayField("Tags", JsonTags);
		
		//FJsonObjectConverter Converter;
		FJsonTransformMatrix JsonTransform;
		//FJsonTransform JsonComponentTransform;
		
		// Are we a component with transform?
		if(const USceneComponent* SceneComponent = Cast<USceneComponent>(Component))
		{	
			FTransform RelativeComponentTransform = SceneComponent->GetRelativeTransform();
			FTransform WorldComponentTransform = SceneComponent->GetComponentTransform();
			
			if(SceneComponent == Actor->GetRootComponent())
			{
				RelativeComponentTransform = FTransform();
			}

			FName name = "Test";

			const FMatrix TransformMatrix = RelativeComponentTransform.ToMatrixWithScale();

			TArray<TSharedPtr<FJsonValue>> JsonTransformMatrix;
			JsonTransformMatrix.Reserve(16);
			for(int r = 0; r < 4; ++r)
				for(int c = 0; c < 4; ++c)
				{
					JsonTransformMatrix.Add(MakeShared<FJsonValueNumber>(TransformMatrix.M[r][c]));
				}

			OutComponentJson->SetArrayField("Transform", JsonTransformMatrix);

			// Extra transforms
			TSharedPtr<FJsonObject> TransformBreakdown = MakeShareable(new FJsonObject());
			const UTGAPluginDeveloperSettings* Settings = GetDefault<UTGAPluginDeveloperSettings>();

			if (Settings->ExportComponentRelativeTransformNoScale)
			{
				const FMatrix Matrix = RelativeComponentTransform.ToMatrixNoScale();
				TArray<TSharedPtr<FJsonValue>> JsonMatrix;
				JsonMatrix.Reserve(16);
				for(int r = 0; r < 4; ++r)
					for(int c = 0; c < 4; ++c)
						JsonMatrix.Add(MakeShared<FJsonValueNumber>(Matrix.M[r][c]));
				TransformBreakdown->SetArrayField("RelativeTransformNoScale", JsonMatrix);
			}

			if (Settings->ExportComponentWorldTransform)
			{
				const FMatrix Matrix = WorldComponentTransform.ToMatrixWithScale();
				TArray<TSharedPtr<FJsonValue>> JsonMatrix;
				JsonMatrix.Reserve(16);
				for(int r = 0; r < 4; ++r)
					for(int c = 0; c < 4; ++c)
						JsonMatrix.Add(MakeShared<FJsonValueNumber>(Matrix.M[r][c]));
				TransformBreakdown->SetArrayField("WorldTransform", JsonMatrix);
			}
			
			if (Settings->ExportComponentWorldTransformNoScale)
			{
				const FMatrix TransformMatrixNoScale = WorldComponentTransform.ToMatrixNoScale();
				TArray<TSharedPtr<FJsonValue>> JsonTransformMatrixNoScale;
				JsonTransformMatrixNoScale.Reserve(16);
				for(int r = 0; r < 4; ++r)
					for(int c = 0; c < 4; ++c)
						JsonTransformMatrixNoScale.Add(MakeShared<FJsonValueNumber>(TransformMatrixNoScale.M[r][c]));
				TransformBreakdown->SetArrayField("WorldTransformNoScale", JsonTransformMatrixNoScale);
			}

			if (Settings->ExportComponentRelativeScale)
			{
				TArray<TSharedPtr<FJsonValue>> JsonTransformBreakdownLocalScale;
				JsonTransformBreakdownLocalScale.SetNum(3);
				const FVector localScale = RelativeComponentTransform.GetScale3D(); 
				JsonTransformBreakdownLocalScale[0] = MakeShared<FJsonValueNumber>(localScale.X);
				JsonTransformBreakdownLocalScale[1] = MakeShared<FJsonValueNumber>(localScale.Y);
				JsonTransformBreakdownLocalScale[2] = MakeShared<FJsonValueNumber>(localScale.Z);
				TransformBreakdown->SetArrayField("RelativeScale", JsonTransformBreakdownLocalScale);
			}

			if (Settings->ExportComponentWorldScale)
			{
				TArray<TSharedPtr<FJsonValue>> JsonTransformBreakdownScale;
				JsonTransformBreakdownScale.SetNum(3);
				const FVector scale = WorldComponentTransform.GetScale3D(); 
				JsonTransformBreakdownScale[0] = MakeShared<FJsonValueNumber>(scale.X);
				JsonTransformBreakdownScale[1] = MakeShared<FJsonValueNumber>(scale.Y);
				JsonTransformBreakdownScale[2] = MakeShared<FJsonValueNumber>(scale.Z);
				TransformBreakdown->SetArrayField("WorldScale", JsonTransformBreakdownScale);
			}

			if (Settings->ExportComponentRelativeRotation)
			{
				TArray<TSharedPtr<FJsonValue>> JsonTransformBreakdownLocalRotation;
				JsonTransformBreakdownLocalRotation.SetNum(3);
				const FRotator localRot = RelativeComponentTransform.GetRotation().Rotator(); 
				JsonTransformBreakdownLocalRotation[0] = MakeShared<FJsonValueNumber>(localRot.Pitch);
				JsonTransformBreakdownLocalRotation[1] = MakeShared<FJsonValueNumber>(localRot.Yaw);
				JsonTransformBreakdownLocalRotation[2] = MakeShared<FJsonValueNumber>(localRot.Roll);
				TransformBreakdown->SetArrayField("RelativeRotationPYR", JsonTransformBreakdownLocalRotation);
			}

			if (Settings->ExportComponentWorldRotation)
			{
				TArray<TSharedPtr<FJsonValue>> JsonTransformBreakdownRotation;
				JsonTransformBreakdownRotation.SetNum(3);
				const FRotator rotation = WorldComponentTransform.GetRotation().Rotator(); 
				JsonTransformBreakdownRotation[0] = MakeShared<FJsonValueNumber>(rotation.Pitch);
				JsonTransformBreakdownRotation[1] = MakeShared<FJsonValueNumber>(rotation.Yaw);
				JsonTransformBreakdownRotation[2] = MakeShared<FJsonValueNumber>(rotation.Roll);
				TransformBreakdown->SetArrayField("WorldRotationPYR", JsonTransformBreakdownRotation);
			}
			
			if (!TransformBreakdown->Values.IsEmpty())
			{
				OutComponentJson->SetObjectField("TransformBreakdown", TransformBreakdown);	
			}			

			//const FMatrix TransformNoScaleMatrix = ComponentTransform.ToMatrixNoScale();
			// JsonTransform.TransformNoScale.Reserve(16);
			//
			// for(int r = 0; r < 4; ++r)
			// 	for(int c = 0; c < 4; ++c)
			// 	{
			// 		JsonTransform.TransformNoScale.Add(TransformNoScaleMatrix.M[r][c]);
			// 	}
			//
			// const FVector_NetQuantize100 Scale = ComponentTransform.GetScale3D();
			// JsonTransform.Scale.SetNum(3);
			// JsonTransform.Scale[0] = Scale.X;
			// JsonTransform.Scale[1] = Scale.Y;
			// JsonTransform.Scale[2] = Scale.Z;
			//
			// TSharedPtr<FJsonObject> Transform = Converter.UStructToJsonObject(JsonTransform);
			// OutComponentJson->SetObjectField("Transform", Transform);
		}
		else
		{
			// Behavior components have no spatial transform of their own.
			TArray<TSharedPtr<FJsonValue>> IdentityTransform;
			IdentityTransform.Reserve(16);
			for(int index = 0; index < 16; ++index)
			{
				IdentityTransform.Add(MakeShared<FJsonValueNumber>(index % 5 == 0 ? 1.0 : 0.0));
			}
			OutComponentJson->SetArrayField("Transform", IdentityTransform);
		}
		
		if(bHasHandler)
		{
			// Give the individual handler time to write component specific things.
			const auto* Info = ComponentHandlers.Find(ClassPtr);
			Info->Signature.Execute(Component, OutComponentJson, Result);			
		}

		// Finally allow custom serialization if we support that
		if(bHasCustomSerializer)
		{
			TSharedPtr<FJsonObject> CustomSerializeData;
			HandleCustomSerialization(Component, CustomSerializeData, Result);
			if(Result)
			{
				OutComponentJson->SetObjectField("Custom", CustomSerializeData);	
			}
		}
	}
	
	return Result;
}
PRAGMA_ENABLE_OPTIMIZATION_ACTUAL

bool TGALevelSerializer::CanSerializeComponent(const AActor* Actor, const UActorComponent* Component) const
{
	const UClass* ClassPtr = Component->GetClass();
	const bool bCustomSerializer = Component->Implements<UTGASerializableInterface>();
	return ComponentHandlers.Contains(ClassPtr) || bCustomSerializer;
}

void TGALevelSerializer::HandleSceneComponent(const UActorComponent* Component, TSharedPtr<FJsonObject>& OutJson, bool& OutResult)
{
    const USceneComponent* SceneComponent = Cast<USceneComponent>(Component);
    ensure(SceneComponent);

	// Nothing in particular happens here, it's a scene component.
    OutResult = true;
}

void TGALevelSerializer::HandleMeshComponent(const UActorComponent* Component,
	TSharedPtr<FJsonObject>& OutJson, bool& OutResult)
{
	const UMeshComponent* MeshComponent = Cast<UMeshComponent>(Component);
	ensure(MeshComponent);

	FString MeshName, MeshPath, AssetImportPath;
	if (const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(Component))
	{
		if (const UStaticMesh* Mesh = StaticMeshComponent->GetStaticMesh())
		{
			MeshName = Mesh->GetName();
			MeshPath = Mesh->GetPathName();
			AssetImportPath = Mesh->GetAssetImportData()->GetFirstFilename();
		}
	}
	else if (const USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(Component))
	{
		if (const USkeletalMesh* Mesh = SkeletalMeshComponent->GetSkeletalMeshAsset())
		{
			MeshName = Mesh->GetName();
			MeshPath = Mesh->GetPathName();
			AssetImportPath = Mesh->GetAssetImportData()->GetFirstFilename();
		}
	}

	if (MeshName.IsEmpty() || MeshPath.IsEmpty())
	{
		UE_LOG(LogTGA, Error, TEXT("-- The MeshComponent %s on %s does not have a Mesh set and will not be exported!"),
			*MeshComponent->GetName(), *MeshComponent->GetOwner()->GetName());
		OutResult = false;
        return;
	}

	OutJson->SetStringField("Mesh", MeshName);
	OutJson->SetStringField("ContentPath", MeshPath);
	OutJson->SetStringField("AssetImportPath", AssetImportPath);

	TArray<TSharedPtr<FJsonValue>> JsonMaterials;
	const uint32 NumMaterials = MeshComponent->GetNumMaterials();
	for(uint32 m = 0; m < NumMaterials; m++)
	{
		const UMaterialInterface* Material = MeshComponent->GetMaterial(m);
		TSharedPtr<FJsonObject> MaterialJson = SerializeMaterial(Material, true);
		if (MaterialJson.IsValid())
		{
			JsonMaterials.Add(MakeShareable(new FJsonValueObject(MaterialJson)));	
		}
	}

	OutJson->SetArrayField("Materials", JsonMaterials);

	OutResult = true;
}

void TGALevelSerializer::HandleStaticMeshComponent(const UActorComponent* Component,
	TSharedPtr<FJsonObject>& OutJson, bool& OutResult)
{
	const UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(Component);
	ensure(MeshComponent);
	const UStaticMesh* Mesh = MeshComponent->GetStaticMesh();
	if(!Mesh)
	{
		UE_LOG(LogTGA, Error, TEXT("-- The MeshComponent %s on %s does not have a Static Mesh set! Skipping Component."),
			*MeshComponent->GetName(), *MeshComponent->GetOwner()->GetName());
		OutResult = false;
        return;
	}

	OutJson->SetStringField("Mesh", Mesh->GetName());
	OutJson->SetStringField("ContentPath", Mesh->GetPathName());

	const FAssetImportInfo& MeshAssetInfo = Mesh->GetAssetImportData()->SourceData;

	TArray<TSharedPtr<FJsonValue>> JsonMaterials;
	const uint32 NumMaterials = MeshComponent->GetNumMaterials();
	for(uint32 m = 0; m < NumMaterials; m++)
	{
		const UMaterialInterface* Material = MeshComponent->GetMaterial(m);
		TSharedPtr<FJsonObject> MaterialJson = SerializeMaterial(Material, true);
		if (MaterialJson.IsValid())
		{
			JsonMaterials.Add(MakeShareable(new FJsonValueObject(MaterialJson)));	
		}
	}

	OutJson->SetArrayField("Materials", JsonMaterials);

	OutResult = true;
}

void TGALevelSerializer::HandleSkeletalMeshComponent(const UActorComponent* Component,
	TSharedPtr<FJsonObject>& OutJson, bool& OutResult)
{
	const USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(Component);
	ensure(SkeletalMeshComponent);

	const USkeletalMesh* Mesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
	if(!Mesh)
	{
		UE_LOG(LogTGA, Error, TEXT("-- The SkeletalMeshComponent %s on %s does not have a Skeletal Mesh set! Skipping Component."),
			*SkeletalMeshComponent->GetName(), *SkeletalMeshComponent->GetOwner()->GetName());
		OutResult = false;
		return;
	}

	OutJson->SetStringField("Mesh", Mesh->GetName());
	OutJson->SetStringField("ContentPath", Mesh->GetPathName());
	const UAssetImportData* assetImportData = Mesh->GetAssetImportData();
	//const FAssetImportInfo& MeshAssetInfo = assetImportData->SourceData;
	
	TArray<TSharedPtr<FJsonValue>> JsonMaterials;
	const uint32 NumMaterials = SkeletalMeshComponent->GetNumMaterials();
	for(uint32 m = 0; m < NumMaterials; m++)
	{
		const UMaterialInterface* Material = SkeletalMeshComponent->GetMaterial(m);
		TSharedPtr<FJsonObject> MaterialJson = SerializeMaterial(Material, true);
		if (MaterialJson.IsValid())
		{
			JsonMaterials.Add(MakeShareable(new FJsonValueObject(MaterialJson)));	
		}
	}	

	OutJson->SetArrayField("Materials", JsonMaterials);

	OutResult = true;
}

void TGALevelSerializer::HandlePointLightComponent(const UActorComponent* Component,
                                                       TSharedPtr<FJsonObject>& OutJson, bool& OutResult)
{
	const UPointLightComponent* PointLightComponent = Cast<UPointLightComponent>(Component);
	ensure(PointLightComponent);

	//FJsonObjectConverter Converter;
	
	const FLinearColor LightColor = PointLightComponent->GetLightColor();
	// FJsonLinearColor JsonColor;
	// JsonColor.Color = {
	// 	LightColor.R,
	// 	LightColor.G,
	// 	LightColor.B,
	// 	LightColor.A
	// };
	//
	// const TSharedPtr<FJsonObject> Color = Converter.UStructToJsonObject(JsonColor);

	TArray<TSharedPtr<FJsonValue>> JsonColorArray;
	JsonColorArray.Reserve(4);
	JsonColorArray.Add(MakeShared<FJsonValueNumber>(LightColor.R));
	JsonColorArray.Add(MakeShared<FJsonValueNumber>(LightColor.G));
	JsonColorArray.Add(MakeShared<FJsonValueNumber>(LightColor.B));
	JsonColorArray.Add(MakeShared<FJsonValueNumber>(LightColor.A));

	//OutJson->SetNumberField("LightType", 1);
	OutJson->SetArrayField("Color", JsonColorArray);
	const float lightIntensity = PointLightComponent->Intensity;
	float candelaM2 = 0;
	switch (PointLightComponent->IntensityUnits)
	{
	case ELightUnits::Candelas:
		// CD can be used directly by the Game Engines, no change is needed.
		candelaM2 = lightIntensity;
		break;
	case ELightUnits::Lumens:
		// Candela = lumens / steradians which is 4PI for Point Lights
		candelaM2 = lightIntensity / (4.0f * PI);
		break;
	case ELightUnits::Unitless:
		// Unitless in UE is 1 Cd = 625 Unitless
		candelaM2 = lightIntensity / 625.0f;
		break;
	case ELightUnits::Nits:
		candelaM2 = lightIntensity;
		break;
	case ELightUnits::EV:
		candelaM2 = 0.125 * powf(2.0f, lightIntensity);
		break;
	}
	OutJson->SetNumberField("Intensity", candelaM2);
	OutJson->SetNumberField("FalloffExponent", PointLightComponent->LightFalloffExponent);
	OutJson->SetNumberField("AttenuationRadius", PointLightComponent->AttenuationRadius);;

	OutResult = true;
}

void TGALevelSerializer::HandleSpotLightComponent(const UActorComponent* Component,
	TSharedPtr<FJsonObject>& OutJson, bool& OutResult)
{
	const USpotLightComponent* SpotLightComponent = Cast<USpotLightComponent>(Component);
	ensure(SpotLightComponent);

	//FJsonObjectConverter Converter;
	
	const FLinearColor LightColor = SpotLightComponent->GetLightColor();
	// FJsonLinearColor JsonColor;
	// JsonColor.Color = {
	// 	LightColor.R,
	// 	LightColor.G,
	// 	LightColor.B,
	// 	LightColor.A
	// };
	//
	// const TSharedPtr<FJsonObject> Color = Converter.UStructToJsonObject(JsonColor);

	TArray<TSharedPtr<FJsonValue>> JsonColorArray;
	JsonColorArray.Reserve(4);
	JsonColorArray.Add(MakeShared<FJsonValueNumber>(LightColor.R));
	JsonColorArray.Add(MakeShared<FJsonValueNumber>(LightColor.G));
	JsonColorArray.Add(MakeShared<FJsonValueNumber>(LightColor.B));
	JsonColorArray.Add(MakeShared<FJsonValueNumber>(LightColor.A));

	
	//OutJson->SetNumberField("LightType", 2);
	OutJson->SetArrayField("Color", JsonColorArray);
	const float lightIntensity = SpotLightComponent->Intensity;
	float candelaM2 = 0;
	switch (SpotLightComponent->IntensityUnits)
	{
	case ELightUnits::Candelas:
		// CD can be used directly by the Game Engines, no change is needed.
		candelaM2 = lightIntensity;
		break;
	case ELightUnits::Lumens:
		// Candela = lumens / steradians which is 2PI(1-cos(th)) for Spot Lights
		candelaM2 = lightIntensity / (2.0f * PI * (1 - cos(FMath::DegreesToRadians(SpotLightComponent->InnerConeAngle))));
		break;
	case ELightUnits::Unitless:
		// Unitless in UE is 1 Cd = 625 Unitless
		candelaM2 = lightIntensity / 625.0f;
		break;
	case ELightUnits::Nits:
		candelaM2 = lightIntensity;
		break;
	case ELightUnits::EV:
		candelaM2 = 0.125 * powf(2.0f, lightIntensity);
		break;
	}
	OutJson->SetNumberField("Intensity", candelaM2);
	OutJson->SetNumberField("FalloffExponent", SpotLightComponent->LightFalloffExponent);
	OutJson->SetNumberField("InnerConeAngle", SpotLightComponent->InnerConeAngle);
	OutJson->SetNumberField("OuterConeAngle", SpotLightComponent->OuterConeAngle);
	OutJson->SetNumberField("AttenuationRadius", SpotLightComponent->AttenuationRadius);

	OutResult = true;
}

void TGALevelSerializer::HandleDirectionalLightComponent(const UActorComponent* Component,
	TSharedPtr<FJsonObject>& OutJson, bool& OutResult)
{
	const UDirectionalLightComponent* DirectionalLightComponent = Cast<UDirectionalLightComponent>(Component);
	ensure(DirectionalLightComponent);

	//FJsonObjectConverter Converter;
	
	const FLinearColor LightColor = DirectionalLightComponent->GetLightColor();
	// FJsonLinearColor JsonColor;
	// JsonColor.Color = {
	// 	LightColor.R,
	// 	LightColor.G,
	// 	LightColor.B,
	// 	LightColor.A
	// };
	//
	// const TSharedPtr<FJsonObject> Color = Converter.UStructToJsonObject(JsonColor);

	TArray<TSharedPtr<FJsonValue>> JsonColorArray;
	JsonColorArray.Reserve(4);
	JsonColorArray.Add(MakeShared<FJsonValueNumber>(LightColor.R));
	JsonColorArray.Add(MakeShared<FJsonValueNumber>(LightColor.G));
	JsonColorArray.Add(MakeShared<FJsonValueNumber>(LightColor.B));
	JsonColorArray.Add(MakeShared<FJsonValueNumber>(LightColor.A));

	//OutJson->SetNumberField("LightType", 0);
	OutJson->SetArrayField("Color", JsonColorArray);
	OutJson->SetNumberField("Intensity", DirectionalLightComponent->Intensity);

	OutResult = true;
}

void TGALevelSerializer::HandleBoxComponent(const UActorComponent* Component, TSharedPtr<FJsonObject>& OutJson,
	bool& OutResult)
{
	const UBoxComponent* BoxComponent = Cast<UBoxComponent>(Component);
	ensure(BoxComponent);

	FJsonObjectConverter Converter;
	
	const FBoxSphereBounds BoxBounds = BoxComponent->GetLocalBounds();

	const FJsonVector JsonBounds = {
		BoxBounds.BoxExtent.X,
		BoxBounds.BoxExtent.Y,
		BoxBounds.BoxExtent.Z
	};

	const TSharedPtr<FJsonObject> Bounds = Converter.UStructToJsonObject(JsonBounds);

	//OutJson->SetStringField("Collider", "Box");
	OutJson->SetObjectField("Bounds", Bounds);

	OutResult = true;
}

void TGALevelSerializer::HandleSphereComponent(const UActorComponent* Component, TSharedPtr<FJsonObject>& OutJson,
	bool& OutResult)
{
	const USphereComponent* SphereComponent = Cast<USphereComponent>(Component);
	ensure(SphereComponent);

	//OutJson->SetStringField("Collider", "Sphere");
	OutJson->SetNumberField("Radius", SphereComponent->GetUnscaledSphereRadius());

	OutResult = true;
}

void TGALevelSerializer::HandleCapsuleComponent(const UActorComponent* Component, TSharedPtr<FJsonObject>& OutJson,
	bool& OutResult)
{
	const UCapsuleComponent* CapsuleComponent = Cast<UCapsuleComponent>(Component);
	ensure(CapsuleComponent);

	//OutJson->SetStringField("Collider", "Capsule");
	OutJson->SetNumberField("Radius", CapsuleComponent->GetUnscaledCapsuleRadius());
	OutJson->SetNumberField("HalfHeight", CapsuleComponent->GetUnscaledCapsuleHalfHeight());

	OutResult = true;
}

void TGALevelSerializer::HandleSpringArmComponent(const UActorComponent* Component, TSharedPtr<FJsonObject>& OutJson,
	bool& OutResult)
{
	const USpringArmComponent* SpringArmComponent = Cast<USpringArmComponent>(Component);
	ensure(SpringArmComponent);

	OutJson->SetNumberField("TargetArmLength", SpringArmComponent->TargetArmLength);

	const FVector SocketOffset = SpringArmComponent->SocketOffset;
	
	const FJsonVector JsonSocketOffset = {
		SocketOffset.X,
		SocketOffset.Y,
		SocketOffset.Z
	};
	
	FJsonObjectConverter Converter;
	const TSharedPtr<FJsonObject> SocketOffsetObject = Converter.UStructToJsonObject(JsonSocketOffset);

	//OutJson->SetStringField("Collider", "Box");
	OutJson->SetObjectField("SocketOffset", SocketOffsetObject);
	OutJson->SetNumberField("ArmLength", SpringArmComponent->TargetArmLength);

	OutResult = true;
}

void TGALevelSerializer::HandleSkyLightComponent(const UActorComponent* Component, TSharedPtr<FJsonObject>& OutJson,
	bool& OutResult)
{
	const USkyLightComponent* SkyLightComponent = Cast<USkyLightComponent>(Component);
	ensure(SkyLightComponent);
	
	FLinearColor LightColor = SkyLightComponent->GetLightColor();
	
	TArray<TSharedPtr<FJsonValue>> JsonColorArray;
	JsonColorArray.Reserve(4);
	JsonColorArray.Add(MakeShared<FJsonValueNumber>(LightColor.R));
	JsonColorArray.Add(MakeShared<FJsonValueNumber>(LightColor.G));
	JsonColorArray.Add(MakeShared<FJsonValueNumber>(LightColor.B));
	JsonColorArray.Add(MakeShared<FJsonValueNumber>(LightColor.A));
	
	OutJson->SetArrayField("Color", JsonColorArray);
	OutJson->SetNumberField("Intensity", SkyLightComponent->Intensity);
	
	TSharedPtr<FJsonObject> TextureJson = MakeShareable(new FJsonObject());
	if (SkyLightComponent->SourceType == ESkyLightSourceType::SLS_SpecifiedCubemap && SkyLightComponent->Cubemap)
	{
		TextureJson->SetStringField("Name", SkyLightComponent->Cubemap->GetName());
		TextureJson->SetStringField("Path", SkyLightComponent->Cubemap->GetPathName());
		TextureJson->SetStringField("AssetImportPath", SkyLightComponent->Cubemap->AssetImportData->GetFirstFilename());
		TextureJson->SetNumberField("CubemapAngle", SkyLightComponent->SourceCubemapAngle);
	}
	else
	{		
		TextureJson->SetStringField("Name", "Not set to SLS_SpecifiedCubemap in UE!");
		TextureJson->SetStringField("Path", "None");
		TextureJson->SetStringField("AssetImportPath", "None");
		TextureJson->SetNumberField("CubemapAngle", 0);
	}
	OutJson->SetObjectField("CubeMap", TextureJson);
	OutResult = true;
}

void TGALevelSerializer::HandleCameraComponent(const UActorComponent* Component, TSharedPtr<FJsonObject>& OutJson,
	bool& OutResult)
{
	const UCameraComponent* cameraComponent = Cast<UCameraComponent>(Component);
	ensure(cameraComponent);

	OutJson->SetNumberField("FOV", cameraComponent->FieldOfView);

	OutResult = true;
}

void TGALevelSerializer::HandleLandscapeComponent(const UActorComponent* Component, TSharedPtr<FJsonObject>& OutJson, bool& OutResult)
{
	const ULandscapeComponent* LandscapeComponent = Cast<ULandscapeComponent>(Component);
	TArray<TSharedPtr<FJsonValue>> JsonMaterials;
	for(const UMaterialInstanceConstant* Material : LandscapeComponent->MaterialInstances)
	{
		TSharedPtr<FJsonObject> MaterialJson = SerializeMaterial(Material, true);
		if (MaterialJson.IsValid())
		{
			JsonMaterials.Add(MakeShareable(new FJsonValueObject(MaterialJson)));
		}
	}

	OutJson->SetArrayField("Materials", JsonMaterials);
	// Channels are allocated per component; consumers must not assume a global layer order.
	TArray<TSharedPtr<FJsonValue>> JsonWeightmaps;
	const TArray<UTexture2D*>& Weightmaps = LandscapeComponent->GetWeightmapTextures();
	const TArray<FWeightmapLayerAllocationInfo>& Allocations = LandscapeComponent->GetWeightmapLayerAllocations();
	for (int32 TextureIndex = 0; TextureIndex < Weightmaps.Num(); ++TextureIndex)
	{
		const UTexture2D* Texture = Weightmaps[TextureIndex];
		TSharedPtr<FJsonObject> WeightmapJson = MakeShared<FJsonObject>();
		const FString Filename = FString::Printf(TEXT("%s_%s_Weightmap_%d.png"),
			*LandscapeComponent->GetOwner()->GetName(), *LandscapeComponent->GetName(), TextureIndex);
		WeightmapJson->SetStringField("File", Filename);
		WeightmapJson->SetNumberField("TextureIndex", TextureIndex);
		WeightmapJson->SetNumberField("Width", Texture->Source.GetSizeX());
		WeightmapJson->SetNumberField("Height", Texture->Source.GetSizeY());
		TArray<TSharedPtr<FJsonValue>> JsonLayers;
		for (const FWeightmapLayerAllocationInfo& Allocation : Allocations)
		{
			if (Allocation.IsAllocated() && Allocation.WeightmapTextureIndex == TextureIndex)
			{
				TSharedPtr<FJsonObject> LayerJson = MakeShared<FJsonObject>();
				LayerJson->SetStringField("Name", Allocation.LayerInfo->GetLayerName().ToString());
				LayerJson->SetNumberField("Channel", Allocation.WeightmapTextureChannel);
				JsonLayers.Add(MakeShared<FJsonValueObject>(LayerJson));
			}
		}
		WeightmapJson->SetArrayField("Layers", JsonLayers);
		JsonWeightmaps.Add(MakeShared<FJsonValueObject>(WeightmapJson));
	}
	OutJson->SetArrayField("Weightmaps", JsonWeightmaps);
	OutResult = true;
}

PRAGMA_ENABLE_OPTIMIZATION_ACTUAL
