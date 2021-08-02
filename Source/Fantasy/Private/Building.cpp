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
	CreateMesh();
}

// Called when the game starts or when spawned
void ABuilding::BeginPlay()
{
	Super::BeginPlay();
	
}

struct Mesh {
	TArray<FVector> vertices;
	TArray<int32> tris;
	TArray<FVector2D> uvs;
	TArray<FVector> normals;
	TArray<FProcMeshTangent> tangents;
	int triangleCount = 0;
};

void ABuilding::CreateMesh()
{
	TArray<Mesh> meshes;
	meshes.AddDefaulted(3);

	float HeightOffset = 0;
	for (int f = 0; f < Floors.Num(); ++f) {

		const int pointCount = SplineComponent->GetNumberOfSplinePoints();
		for (int i = 0; i < pointCount - 1; ++i) {
			const float startDistance = SplineComponent->GetDistanceAlongSplineAtSplinePoint(i);
			const float endDistance = SplineComponent->GetDistanceAlongSplineAtSplinePoint(i + 1);

			const float distance = endDistance - startDistance;
			const int sections = FMath::Max((int)distance / (int)DefaultSectionLength, 1);
			const float sectionsLength = distance / (float)sections;

			const int patternSize = FMath::Max(Floors[f].Pattern.Num(), 1);
			for (int s = 0; s < sections; ++s) {
				const int patternS = s % patternSize;
				float a = startDistance + s * sectionsLength;
				float b = a + sectionsLength;

				FVector startPosition = SplineComponent->GetLocationAtDistanceAlongSpline(a, ESplineCoordinateSpace::Local);
				FVector endPosition = SplineComponent->GetLocationAtDistanceAlongSpline(b, ESplineCoordinateSpace::Local);

				FVector Offset = FVector::UpVector * HeightOffset;
				if (MeshTypes.Num() == 0
					|| Floors[f].Pattern.Num() == 0
					|| Floors[f].Pattern[patternS] >= MeshTypes.Num()
					|| MeshTypes[Floors[f].Pattern[patternS]].StaticMesh == nullptr) {
					meshes[0].vertices.Add(startPosition + Offset);
					meshes[0].vertices.Add(startPosition + FVector::UpVector * Floors[f].Height + Offset);
					meshes[0].vertices.Add(endPosition + FVector::UpVector * Floors[f].Height + Offset);
					meshes[0].vertices.Add(endPosition + Offset);

					int index = meshes[0].triangleCount;
					meshes[0].tris.Add(index);
					meshes[0].tris.Add(index + 2);
					meshes[0].tris.Add(index + 1);
					meshes[0].tris.Add(index);
					meshes[0].tris.Add(index + 3);
					meshes[0].tris.Add(index + 2);
					meshes[0].triangleCount += 4;

					if (sectionsLength < DefaultSectionLength) {
						float d = sectionsLength / DefaultSectionLength;
						meshes[0].uvs.Add(FVector2D(0, 0));
						meshes[0].uvs.Add(FVector2D(0, 1));
						meshes[0].uvs.Add(FVector2D(d, 1));
						meshes[0].uvs.Add(FVector2D(d, 0));
					} else {
						meshes[0].uvs.Add(FVector2D(0, 0));
						meshes[0].uvs.Add(FVector2D(0, 1));
						meshes[0].uvs.Add(FVector2D(1, 1));
						meshes[0].uvs.Add(FVector2D(1, 0));
					}

					meshes[0].normals.Add(FVector::ForwardVector);
					meshes[0].normals.Add(FVector::ForwardVector);
					meshes[0].normals.Add(FVector::ForwardVector);
					meshes[0].normals.Add(FVector::ForwardVector);
				} else {
					const auto& MeshData = MeshTypes[Floors[f].Pattern[patternS]];
					UStaticMesh* StaticMesh = MeshData.StaticMesh;
					const auto& MeshLOD0 = StaticMesh->GetRenderData()->LODResources[0];
					
					for (int meshSectionIndex = 0; meshSectionIndex < MeshLOD0.Sections.Num(); ++meshSectionIndex) {
						const auto& meshSection = MeshLOD0.Sections[meshSectionIndex];
						const int meshMaterialIndex = meshSection.MaterialIndex;

						FTransform transform = FTransform(
							UKismetMathLibrary::FindLookAtRotation(startPosition, endPosition),
							startPosition + Offset,
							FVector(sectionsLength / 100.0f, 1.0f, Floors[f].Height));

						const auto addVertex = [&](int index) {
							FVector pos = MeshLOD0.VertexBuffers.PositionVertexBuffer.VertexPosition(index);
							FVector posUnit = MeshData.UnionizeTransform.TransformPosition(pos);
							FVector newPos = transform.TransformPosition(posUnit);
							meshes[meshMaterialIndex].vertices.Add(newPos);
							meshes[meshMaterialIndex].uvs.Add(MeshLOD0.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(index, 0));
							meshes[meshMaterialIndex].normals.Add(MeshLOD0.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(index));
							meshes[meshMaterialIndex].tangents.Add(FProcMeshTangent(
								MeshLOD0.VertexBuffers.StaticMeshVertexBuffer.VertexTangentX(index), false));
						};

						for (uint32 v = meshSection.MinVertexIndex; v <= meshSection.MaxVertexIndex; ++v) {
							addVertex(v);
						}

						for (uint32 t = 0; t < meshSection.NumTriangles; ++t) {
							int tIndex = t * 3 + meshSection.FirstIndex;

							int offset = meshes[meshMaterialIndex].triangleCount - meshSection.MinVertexIndex;
							meshes[meshMaterialIndex].tris.Add(offset + MeshLOD0.IndexBuffer.GetIndex(tIndex));
							meshes[meshMaterialIndex].tris.Add(offset + MeshLOD0.IndexBuffer.GetIndex(tIndex + 1));
							meshes[meshMaterialIndex].tris.Add(offset + MeshLOD0.IndexBuffer.GetIndex(tIndex + 2));
						}
						meshes[meshMaterialIndex].triangleCount = meshes[meshMaterialIndex].vertices.Num();
					}
				}
			}
		}
		HeightOffset += Floors[f].Height;
	}

	for (int i = 0; i < meshes.Num(); ++i) {
		MeshComponent->CreateMeshSection(i, meshes[i].vertices, meshes[i].tris, meshes[i].normals, meshes[i].uvs, TArray<FVector2D>(), TArray<FVector2D>(), TArray<FVector2D>(), TArray<FColor>(), meshes[i].tangents, true);
		if (i < Materials.Num()) {
			MeshComponent->SetMaterial(i, Materials[i]);
		} else {
			MeshComponent->SetMaterial(i, nullptr);
		}
	}
}

// Called every frame
void ABuilding::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

