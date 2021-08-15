// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Building.generated.h"

USTRUCT(BlueprintType) struct FMeshData {
	GENERATED_BODY();
	
	FMeshData() :
		StaticMesh(nullptr),
		Length(250.0f),
		Height(300.0f) {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* StaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Length;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Height;
};

USTRUCT(BlueprintType) struct FBuildingSection {
	GENERATED_BODY();

	FBuildingSection() : Pattern() {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<uint8> Pattern;
};

USTRUCT(BlueprintType) struct FFloorType {
	GENERATED_BODY();

	FFloorType() : Height(250.0f), Sections() {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Height;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FBuildingSection> Sections;
};

UCLASS()
class FANTASY_API ABuilding : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class UProceduralMeshComponent* MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class USplineComponent* SplineComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Const")
	float DefaultSectionLength = 140;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	TArray<FFloorType> Floors;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	bool FillBottom = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	UMaterialInterface* BottomMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	bool FillTop = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	UMaterialInterface* TopMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	float TopSink = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Selection")
	TArray<FMeshData> MeshTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Selection")
	TMap<UMaterialInterface*, UMaterialInterface*> Materials;

public:	
	// Sets default values for this actor's properties
	ABuilding();

	virtual ~ABuilding();


protected:
	virtual void OnConstruction(const FTransform& Transform);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void CreateBlankData();
	void CreateMesh();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
