// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Building.generated.h"

USTRUCT(BlueprintType) struct FMeshData {
	GENERATED_BODY();
	
	FMeshData() :
		StaticMesh(nullptr),
		UnionizeTransform(FQuat::Identity, FVector::ZeroVector, FVector(1.0f, 1.0f, 1.0f / 250.0f)) {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* StaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform UnionizeTransform;
};

USTRUCT(BlueprintType) struct FFloorType {
	GENERATED_BODY();

	FFloorType() : Height(250.0f), Pattern() {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Height;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<uint8> Pattern;
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


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Selection")
	TArray<FMeshData> MeshTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|Selection")
	UMaterialInstance* BaseMaterial;

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
