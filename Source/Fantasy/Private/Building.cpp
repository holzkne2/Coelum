// Fill out your copyright notice in the Description page of Project Settings.


#include "Building.h"
#include "ProceduralMeshComponent.h"
#include <Components/SplineComponent.h>
#include "KismetProceduralMeshLibrary.h"
#include <Kismet/KismetMathLibrary.h>
#include <DrawDebugHelpers.h>

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
		int sectionsDifference = FMath::Max(0, Sections - Floors[f].Sections.Num());
		if (sectionsDifference <= 0) {
			continue;
		}
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
	MeshComponent->ClearAllMeshSections();

	TMap<UMaterialInterface*, TMesh> Meshes;
	for (int meshType = 0; meshType < MeshTypes.Num(); ++meshType) {
		if (MeshTypes[meshType].StaticMesh == nullptr) {
			continue;
		}

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

			const int patternSize = currentSection.Pattern.Num();
			
			float patternLength = 0.0f;
			int patternCount;
			for (patternCount = 0; patternLength < distance; ++patternCount) {
				const int patternS = patternCount % patternSize;
				const int& PatternSectionIndex = currentSection.Pattern[patternS];
				const auto& PatternItem = MeshTypes[PatternSectionIndex];

				patternLength += PatternItem.Length;
			}
			const float patternScale = distance / patternLength;

			float currentLength = 0;
			for (int PatternSection = 0; PatternSection < patternCount; ++PatternSection) {
				const int PatternS = PatternSection % patternSize;
				const int& PatternSectionIndex = currentSection.Pattern[PatternS];
				const auto& PatternItem = MeshTypes[PatternSectionIndex];

				float a = startDistance + currentLength;
				float b = a + PatternItem.Length;
				currentLength += PatternItem.Length * patternScale;

				FVector startPosition = SplineComponent->GetLocationAtDistanceAlongSpline(a, ESplineCoordinateSpace::Local);
				FVector endPosition = SplineComponent->GetLocationAtDistanceAlongSpline(b, ESplineCoordinateSpace::Local);

				FVector ZOffset = FVector::UpVector * HeightOffset;
				if (MeshTypes.Num() == 0
					|| MeshTypes[PatternSectionIndex].StaticMesh == nullptr) {
					break; // ERROR
				} else {
					UStaticMesh const * StaticMesh = PatternItem.StaticMesh;
					const auto& MeshLOD0 = StaticMesh->GetRenderData()->LODResources[0];
					
					for (int meshSectionIndex = 0; meshSectionIndex < MeshLOD0.Sections.Num(); ++meshSectionIndex) {
						const auto& meshSection = MeshLOD0.Sections[meshSectionIndex];
						const auto& material = StaticMesh->GetMaterial(meshSection.MaterialIndex);

						FTransform transform = FTransform(
							UKismetMathLibrary::FindLookAtRotation(startPosition, endPosition),
							startPosition + ZOffset,
							FVector(patternScale, 1.0f, floor.Height / PatternItem.Height));
						FTransform normalTransform = FTransform(
							UKismetMathLibrary::FindLookAtRotation(startPosition, endPosition),
							FVector::ZeroVector,
							FVector::OneVector);

						const auto addVertex = [&](int index) {
							FVector pos = MeshLOD0.VertexBuffers.PositionVertexBuffer.VertexPosition(index);
							// TODO: Scale
							FVector newPos = transform.TransformPosition(pos);
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

	if (FillTop || FillBottom) {
		constexpr float TriangleSize = 64.0f;

		const auto& ComponentBounds = SplineComponent->Bounds.BoxExtent;
		const auto& ComponentOrigin = SplineComponent->Bounds.Origin;
		int NumX = FMath::CeilToInt((ComponentBounds.X / TriangleSize) + 1);
		int NumY = FMath::CeilToInt((ComponentBounds.Y / ((TriangleSize / 2.0f) * FMath::Tan(FMath::DegreesToRadians(60)))) + 1);

		TArray<FVector> generalVertices;
		TArray<int8> pointIndex;
		for (int y = -NumY; y <= NumY; ++y) {
			for (int x = -NumX; x <= NumX; ++x) {
				int CurrentX = ComponentOrigin.X + (TriangleSize * x) + ((TriangleSize / 2.0f) * (FMath::Abs(y + NumY) % 2));
				int CurrentY = ComponentOrigin.Y + (TriangleSize / 2.0f * FMath::Tan(FMath::DegreesToRadians(60)) * y);
				FVector CurrentLocation(CurrentX, CurrentY, ComponentOrigin.Z);
				
				FVector CurrentEdgeLocation = SplineComponent->FindLocationClosestToWorldLocation(CurrentLocation, ESplineCoordinateSpace::World);
				FVector DistanceClosest = SplineComponent->FindDirectionClosestToWorldLocation(CurrentLocation, ESplineCoordinateSpace::World);
				const bool inside = FVector::DotProduct(CurrentEdgeLocation - CurrentLocation, FVector::CrossProduct(FVector::UpVector, DistanceClosest)) > 0;
				const bool edge = (CurrentEdgeLocation - CurrentLocation).Size() < TriangleSize;

				if (inside) {
					// Inside
					generalVertices.Add(CurrentLocation);
					pointIndex.Add(0);
				} else if (edge) {
					// Edge
					generalVertices.Add(CurrentEdgeLocation);
					pointIndex.Add(1);
				} else {
					// Outside
					generalVertices.Add(CurrentLocation);
					pointIndex.Add(-1);
				}
			}
		}

		float Scale;
		constexpr float Padding = 0.0f;
		if (ComponentOrigin.X > ComponentOrigin.Y) {
			Scale = (1 - Padding) / (ComponentOrigin.X * 2);
		} else {
			Scale = (1 - Padding) / (ComponentOrigin.Y * 2);
		}
		const FVector OriginScaled = Scale * ComponentOrigin;
		TArray<FVector2D> UVs;
		for (int i = 0; i < generalVertices.Num(); ++i) {
			UVs.Add(FVector2D(
				generalVertices[i].X * Scale + 0.5f + (OriginScaled.X * -1),
				generalVertices[i].Y * Scale + 0.5f + (OriginScaled.Y * -1)));
		}

		const int GridX = NumX * 2;
		if (FillBottom) {
			TArray<FVector> vertices;
			TArray<FVector> normals;
			TArray<int32> triangles;

			for (int i = GridX + 1; i <= generalVertices.Num() - 2; ++i) {
				if ((i + 1) % (GridX + 1) == 0) {
					continue;
				}
				int a = ((((i / (GridX + 1)) % 2) * -1) + 1) + i;
				int b = i - (GridX + 1);
				int c = i - (GridX + 1) + 1;

				if (pointIndex[a] >= 0 && pointIndex[b] >= 0 && pointIndex[c] >= 0) {
					// Inverted Tris
					triangles.Add(a);
					triangles.Add(b);
					triangles.Add(c);
				}

				a = i;
				b = i - ((GridX + 1) - ((i / (GridX + 1)) % 2));
				c = i + 1;

				if (pointIndex[a] >= 0 && pointIndex[b] >= 0 && pointIndex[c] >= 0) {
					// Tris
					triangles.Add(a);
					triangles.Add(b);
					triangles.Add(c);
				}
			}

			for (int i = 0; i < generalVertices.Num(); ++i) {
				normals.Add(FVector::DownVector);
				vertices.Add(GetTransform().InverseTransformPosition(generalVertices[i]));
			}

			MeshComponent->CreateMeshSection(Meshes.Num(), vertices, triangles, normals, UVs, TArray<FVector2D>(), TArray<FVector2D>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), true);
			MeshComponent->SetMaterial(Meshes.Num(), BottomMaterial);
		}

		if (FillTop) {
			TArray<FVector> vertices;
			TArray<FVector> normals;
			TArray<int32> triangles;

			for (int i = GridX + 1; i <= generalVertices.Num() - 2; ++i) {
				if ((i + 1) % (GridX + 1) == 0) {
					continue;
				}
				int a = ((((i / (GridX + 1)) % 2) * -1) + 1) + i;
				int b = i - (GridX + 1) + 1;
				int c = i - (GridX + 1);

				if (pointIndex[a] >= 0 && pointIndex[b] >= 0 && pointIndex[c] >= 0) {
					// Inverted Tris
					triangles.Add(a);
					triangles.Add(b);
					triangles.Add(c);
				}

				a = i;
				b = i + 1;
				c = i - ((GridX + 1) - ((i / (GridX + 1)) % 2));

				if (pointIndex[a] >= 0 && pointIndex[b] >= 0 && pointIndex[c] >= 0) {
					// Tris
					triangles.Add(a);
					triangles.Add(b);
					triangles.Add(c);
				}
			}

			const float offset = HeightOffset - TopSink;
			for (int i = 0; i < generalVertices.Num(); ++i) {
				normals.Add(FVector::UpVector);
				vertices.Add(GetTransform().InverseTransformPosition(generalVertices[i]));
				vertices[i].Z += offset;
			}

			MeshComponent->CreateMeshSection(Meshes.Num() + 1, vertices, triangles, normals, UVs, TArray<FVector2D>(), TArray<FVector2D>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), true);
			MeshComponent->SetMaterial(Meshes.Num() + 1, TopMaterial);
		}
	}

	int i = 0;
	for (auto Itr = Meshes.CreateConstIterator(); Itr; ++Itr) {
		const auto& Mesh = Itr.Value();
		MeshComponent->CreateMeshSection(i, Mesh.vertices, Mesh.tris, Mesh.normals, Mesh.uvs, TArray<FVector2D>(), TArray<FVector2D>(), TArray<FVector2D>(), TArray<FColor>(), Mesh.tangents, true);
		const auto& DebugMaterial = Itr.Key();
		const auto& FinalMaterial = Materials.Find(DebugMaterial);
		if (FinalMaterial == nullptr || *FinalMaterial == nullptr) {
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

