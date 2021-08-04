// Fill out your copyright notice in the Description page of Project Settings.


#include "Building.h"
#include "ProceduralMeshComponent.h"
#include <Components/SplineComponent.h>
#include "KismetProceduralMeshLibrary.h"
#include <Kismet/KismetMathLibrary.h>

#define INDEX(_x, _y, _z) (((_z) * Length * Width) + ((_y) * Width) + (_x))

// Sets default values
ABuilding::ABuilding()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>("Mesh");
	SetRootComponent(MeshComponent);
	MeshComponent->bUseAsyncCooking = true;

	SplineComponent = CreateDefaultSubobject<USplineComponent>("Spline");

	FFloorType First = FFloorType();
	Floors.Add(First);
}

ABuilding::~ABuilding()
{
}



void ABuilding::OnConstruction(const FTransform& Transform)
{
	const int pointCount = SplineComponent->GetNumberOfSplinePoints();
	int Sections = pointCount - 1;
	if (SplineComponent->IsClosedLoop()) {
		++Sections;
	}
	
	for (int f = 0; f < Floors.Num(); ++f) {
		int sectionsDifference = Sections - Floors[f].Sections.Num();
		Floors[f].Sections.AddDefaulted(sectionsDifference);
		for (int s = 0; s < Sections; ++s) {
			if (Floors[f].Sections[s].Pattern.Num() == 0) {
				Floors[f].Sections[s].Pattern.Add(0);
			}
		}
	}

	CreateMesh();
}

// Called when the game starts or when spawned
void ABuilding::BeginPlay()
{
	Super::BeginPlay();
	
}

struct TMesh {
	TArray<FVector> vertices;
	TArray<int32> tris;
	TArray<FVector2D> uvs;
	TArray<FVector> normals;
	TArray<FProcMeshTangent> tangents;
	int triangleCount = 0;
};

void ABuilding::CreateMesh()
{
	TMap<UMaterialInterface*, TMesh> Meshes;
	for (int meshType = 0; meshType < MeshTypes.Num(); ++meshType) {
		const auto& DebugMaterials = MeshTypes[meshType].StaticMesh->GetStaticMaterials();
		for (int material = 0; material < DebugMaterials.Num(); ++material) {
			const auto& Material = DebugMaterials[material];
			Meshes.FindOrAdd(Material.MaterialInterface, TMesh());
		}
	}

	float HeightOffset = 0;
	for (int f = 0; f < Floors.Num(); ++f) {
		const auto& floor = Floors[f];

		const int pointCount = SplineComponent->GetNumberOfSplinePoints();
		int TotalBuildingSections = pointCount - 1;
		if (SplineComponent->IsClosedLoop()) {
			++TotalBuildingSections;
		}
		for (int BuildingSection = 0; BuildingSection < TotalBuildingSections; ++BuildingSection) {
			const auto& currentSection = floor.Sections[BuildingSection];

			const float startDistance = SplineComponent->GetDistanceAlongSplineAtSplinePoint(BuildingSection);
			const float endDistance = SplineComponent->GetDistanceAlongSplineAtSplinePoint(BuildingSection + 1);

			const float distance = endDistance - startDistance;
			const int sections = FMath::Max((int)distance / (int)DefaultSectionLength, 1);
			const float sectionsLength = distance / (float)sections;

			const int patternSize = currentSection.Pattern.Num();
			for (int PatternSection = 0; PatternSection < sections; ++PatternSection) {
				const int patternS = PatternSection % patternSize;
				const int& PatternSectionIndex = currentSection.Pattern[patternS];

				float a = startDistance + PatternSection * sectionsLength;
				float b = a + sectionsLength;

				FVector startPosition = SplineComponent->GetLocationAtDistanceAlongSpline(a, ESplineCoordinateSpace::Local);
				FVector endPosition = SplineComponent->GetLocationAtDistanceAlongSpline(b, ESplineCoordinateSpace::Local);

				FVector Offset = FVector::UpVector * HeightOffset;
				if (MeshTypes.Num() == 0
					|| MeshTypes[PatternSectionIndex].StaticMesh == nullptr) {
				} else {
					const auto& MeshData = MeshTypes[PatternSectionIndex];
					UStaticMesh* StaticMesh = MeshData.StaticMesh;
					const auto& MeshLOD0 = StaticMesh->GetRenderData()->LODResources[0];
					
					for (int meshSectionIndex = 0; meshSectionIndex < MeshLOD0.Sections.Num(); ++meshSectionIndex) {
						const auto& meshSection = MeshLOD0.Sections[meshSectionIndex];
						const auto& material = StaticMesh->GetMaterial(meshSection.MaterialIndex);

						FTransform transform = FTransform(
							UKismetMathLibrary::FindLookAtRotation(startPosition, endPosition),
							startPosition + Offset,
							FVector(sectionsLength / 100.0f, 1.0f, Floors[f].Height));
						FTransform normalTransform = FTransform(
							UKismetMathLibrary::FindLookAtRotation(startPosition, endPosition),
							FVector::ZeroVector,
							FVector::OneVector);

						const auto addVertex = [&](int index) {
							FVector pos = MeshLOD0.VertexBuffers.PositionVertexBuffer.VertexPosition(index);
							FVector posUnit = MeshData.UnionizeTransform.TransformPosition(pos);
							FVector newPos = transform.TransformPosition(posUnit);
							Meshes[material].vertices.Add(newPos);
							Meshes[material].uvs.Add(MeshLOD0.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(index, 0));
							Meshes[material].normals.Add(normalTransform.TransformVector(MeshLOD0.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(index)));
							Meshes[material].tangents.Add(FProcMeshTangent(
								normalTransform.TransformVector(MeshLOD0.VertexBuffers.StaticMeshVertexBuffer.VertexTangentX(index)), false));
						};

						for (uint32 v = meshSection.MinVertexIndex; v <= meshSection.MaxVertexIndex; ++v) {
							addVertex(v);
						}

						for (uint32 t = 0; t < meshSection.NumTriangles; ++t) {
							int tIndex = t * 3 + meshSection.FirstIndex;

							int offset = Meshes[material].triangleCount - meshSection.MinVertexIndex;
							Meshes[material].tris.Add(offset + MeshLOD0.IndexBuffer.GetIndex(tIndex));
							Meshes[material].tris.Add(offset + MeshLOD0.IndexBuffer.GetIndex(tIndex + 1));
							Meshes[material].tris.Add(offset + MeshLOD0.IndexBuffer.GetIndex(tIndex + 2));
						}
						Meshes[material].triangleCount = Meshes[material].vertices.Num();
					}
				}
			}
		}
		HeightOffset += Floors[f].Height;
	}

	if (FillBottom) {

	}

	int i = 0;
	for (auto Itr = Meshes.CreateConstIterator(); Itr; ++Itr) {
		const auto& Mesh = Itr.Value();
		MeshComponent->CreateMeshSection(i, Mesh.vertices, Mesh.tris, Mesh.normals, Mesh.uvs, TArray<FVector2D>(), TArray<FVector2D>(), TArray<FVector2D>(), TArray<FColor>(), Mesh.tangents, true);
		const auto& DebugMaterial = Itr.Key();
		const auto& FinalMaterial = Materials.Find(DebugMaterial);
		if (FinalMaterial == nullptr) {
			MeshComponent->SetMaterial(i, DebugMaterial);
		} else {
			MeshComponent->SetMaterial(i, *FinalMaterial);
		}
		++i;
	}
}

// Called every frame
void ABuilding::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

