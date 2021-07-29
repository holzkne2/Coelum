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

void ABuilding::CreateMesh()
{
	TArray<FVector> vertices;
	TArray<int32> tris;
	TArray<FVector2D> uvs;
	TArray<FVector> normals;
	int triangleCount = 0;

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
					vertices.Add(startPosition + Offset);
					vertices.Add(startPosition + FVector::UpVector * Floors[f].Height + Offset);
					vertices.Add(endPosition + FVector::UpVector * Floors[f].Height + Offset);
					vertices.Add(endPosition + Offset);

					int index = triangleCount;
					tris.Add(index);
					tris.Add(index + 2);
					tris.Add(index + 1);
					tris.Add(index);
					tris.Add(index + 3);
					tris.Add(index + 2);
					triangleCount += 4;

					if (sectionsLength < DefaultSectionLength) {
						float d = sectionsLength / DefaultSectionLength;
						uvs.Add(FVector2D(0, 0));
						uvs.Add(FVector2D(0, 1));
						uvs.Add(FVector2D(d, 1));
						uvs.Add(FVector2D(d, 0));
					} else {
						uvs.Add(FVector2D(0, 0));
						uvs.Add(FVector2D(0, 1));
						uvs.Add(FVector2D(1, 1));
						uvs.Add(FVector2D(1, 0));
					}

					normals.Add(FVector::ForwardVector);
					normals.Add(FVector::ForwardVector);
					normals.Add(FVector::ForwardVector);
					normals.Add(FVector::ForwardVector);
				} else {
					const auto& MeshData = MeshTypes[Floors[f].Pattern[patternS]];
					UStaticMesh* StaticMesh = MeshData.StaticMesh;
					const auto& MeshLOD0 = StaticMesh->GetRenderData()->LODResources[0];
					
					FTransform transform = FTransform(
						UKismetMathLibrary::FindLookAtRotation(startPosition, endPosition),
						startPosition + Offset,
						FVector(sectionsLength / 100.0f, 1.0f, Floors[f].Height));

					for (int v = 0; v < MeshLOD0.GetNumVertices(); ++v) {
						FVector pos = MeshLOD0.VertexBuffers.PositionVertexBuffer.VertexPosition(v);
						FVector posUnit = MeshData.UnionizeTransform.TransformPosition(pos);
						FVector newPos = transform.TransformPosition(posUnit);
						vertices.Add(newPos);
						uvs.Add(MeshLOD0.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(v, 0));
						normals.Add(MeshLOD0.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(v));
					}

					for (int t = 0; t < MeshLOD0.GetNumTriangles()*3; ++t) {
						tris.Add(triangleCount + MeshLOD0.IndexBuffer.GetIndex(t));
					}
					triangleCount += MeshLOD0.GetNumVertices();
				}
			}
		}
		HeightOffset += Floors[f].Height;
	}

	//normals.AddZeroed(vertices.Num());
	//for (int t = 0; t < tris.Num(); t += 3) {
	//	const auto doNormal = [vertices, &normals](int a, int b, int c) {
	//		const auto& vertexA = vertices[a];
	//		const auto& vertexB = vertices[b];
	//		const auto& vertexC = vertices[c];
	//		FVector U = vertexB - vertexA;
	//		FVector V = vertexC - vertexA;

	//		FVector normal = FVector::CrossProduct(V, U);
	//		normals[a] += normal;
	//		normals[b] += normal;
	//		normals[c] += normal;
	//	};
	//	const auto& a = tris[t];
	//	const auto& b = tris[t + 1];
	//	const auto& c = tris[t + 2];
	//	doNormal(a, b, c);
	//}

	//for (int n = 0; n < normals.Num(); ++n) {
	//	normals[n].Normalize();
	//}

	TArray<FVector> _normals;
	TArray<FProcMeshTangent> tangents;
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(vertices, tris, uvs, _normals, tangents);

	MeshComponent->CreateMeshSection(0, vertices, tris, normals, uvs, TArray<FVector2D>(), TArray<FVector2D>(), TArray<FVector2D>(), TArray<FColor>(), tangents, true);
	MeshComponent->SetMaterial(0, BaseMaterial);
}

// Called every frame
void ABuilding::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

