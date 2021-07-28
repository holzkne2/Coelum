// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Building.generated.h"

USTRUCT(BlueprintType) struct FloorType {
	GENERATED_BODY();

	FloorType() : Height(250.0f) {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Height;

	UPROPERTY(EditAnywhere, Category = "Building|Selection")
	TArray<UStaticMesh*> MeshTypes;
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building|Const")
	float DefaultSectionLength = 75;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	TArray<FloorType> Floors;

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
