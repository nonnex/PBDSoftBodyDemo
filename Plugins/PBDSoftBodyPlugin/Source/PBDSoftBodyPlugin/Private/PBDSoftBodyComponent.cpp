#include "PBDSoftBodyComponent.h"
#include "Rendering/SkeletalMeshRenderData.h"

void UPBDSoftBodyComponent::BeginPlay() {
    Super::BeginPlay();
    InitializeSimulationData();
}

void UPBDSoftBodyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UPBDSoftBodyComponent::InitializeSimulationData() {
    USkeletalMesh* Mesh = GetSkeletalMeshAsset();
    if (!Mesh) {
        UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: No SkeletalMesh assigned."));
        return;
    }

    // Get vertex count from LOD 0
    const FSkeletalMeshLODRenderData* LODRenderData = Mesh->GetResourceForRendering()->LODRenderData.Num() > 0
        ? &Mesh->GetResourceForRendering()->LODRenderData[0]
        : nullptr;
    if (!LODRenderData) {
        UE_LOG(LogTemp, Error, TEXT("PBDSoftBodyComponent: No LODRenderData available."));
        return;
    }
    int32 VertexCount = LODRenderData->GetNumVertices();

    // Reset and pre-allocate arrays
    Velocities.Reset();
    SimulatedPositions.Reset();
    Velocities.Reserve(VertexCount);
    SimulatedPositions.Reserve(VertexCount);

    // Use EAllowShrinking::NoShrinking for UE 5.5 compatibility
    Velocities.SetNum(VertexCount, EAllowShrinking::No);
    SimulatedPositions.SetNum(VertexCount, EAllowShrinking::No);

    TArray<FVector> InitialPositions = GetVertexPositions();
    if (InitialPositions.Num() == VertexCount) {
        for (int32 i = 0; i < VertexCount; i++) {
            Velocities[i] = FVector::ZeroVector;
            SimulatedPositions[i] = InitialPositions[i];
        }
    }
    else {
        UE_LOG(LogTemp, Error, TEXT("PBDSoftBodyComponent: Vertex position retrieval failed. Expected %d, got %d."), VertexCount, InitialPositions.Num());
    }
}

TArray<FVector> UPBDSoftBodyComponent::GetVertexPositions() const {
    TArray<FVector> Positions;
    USkeletalMesh* Mesh = GetSkeletalMeshAsset();
    if (!Mesh) {
        return Positions;
    }

    const FSkeletalMeshLODRenderData* LODRenderData = Mesh->GetResourceForRendering()->LODRenderData.Num() > 0
        ? &Mesh->GetResourceForRendering()->LODRenderData[0]
        : nullptr;
    if (LODRenderData) {
        int32 NumVertices = LODRenderData->GetNumVertices();
        Positions.SetNumUninitialized(NumVertices);
        for (int32 i = 0; i < NumVertices; i++) {
            // Convert FVector3f to FVector explicitly
            FVector3f VertexPos = LODRenderData->StaticVertexBuffers.PositionVertexBuffer.VertexPosition(i);
            Positions[i] = FVector(VertexPos.X, VertexPos.Y, VertexPos.Z);
        }
    }
    return Positions;
}