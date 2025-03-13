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
        if (bEnableDebugLogging) {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: No SkeletalMesh assigned to %s."), *GetOwner()->GetName());
        }
        return;
    }

    const FSkeletalMeshLODRenderData* LODRenderData = Mesh->GetResourceForRendering()->LODRenderData.Num() > 0
        ? &Mesh->GetResourceForRendering()->LODRenderData[0]
        : nullptr;
    if (!LODRenderData) {
        if (bEnableDebugLogging) {
            UE_LOG(LogTemp, Error, TEXT("PBDSoftBodyComponent: No LODRenderData available for %s."), *Mesh->GetName());
        }
        return;
    }
    int32 VertexCount = LODRenderData->GetNumVertices();

    if (bEnableDebugLogging) {
        UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Initializing simulation data for %s with %d vertices."), *Mesh->GetName(), VertexCount);
    }

    Velocities.Reset();
    SimulatedPositions.Reset();
    Velocities.Reserve(VertexCount);
    SimulatedPositions.Reserve(VertexCount);

    Velocities.SetNum(VertexCount, EAllowShrinking::No);
    SimulatedPositions.SetNum(VertexCount, EAllowShrinking::No);

    TArray<FVector> InitialPositions = GetVertexPositions();
    if (InitialPositions.Num() == VertexCount) {
        for (int32 i = 0; i < VertexCount; i++) {
            Velocities[i] = FVector::ZeroVector;
            SimulatedPositions[i] = InitialPositions[i];
        }
        if (bEnableDebugLogging) {
            UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Successfully initialized %d vertices for %s."), VertexCount, *Mesh->GetName());
        }
    }
    else {
        if (bEnableDebugLogging) {
            UE_LOG(LogTemp, Error, TEXT("PBDSoftBodyComponent: Vertex position retrieval failed for %s. Expected %d, got %d."),
                *Mesh->GetName(), VertexCount, InitialPositions.Num());
        }
    }
}

TArray<FVector> UPBDSoftBodyComponent::GetVertexPositions() const {
    TArray<FVector> Positions;
    USkeletalMesh* Mesh = GetSkeletalMeshAsset();
    if (!Mesh) {
        if (bEnableDebugLogging) {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: GetVertexPositions - No SkeletalMesh assigned to %s."), *GetOwner()->GetName());
        }
        return Positions;
    }

    const FSkeletalMeshLODRenderData* LODRenderData = Mesh->GetResourceForRendering()->LODRenderData.Num() > 0
        ? &Mesh->GetResourceForRendering()->LODRenderData[0]
        : nullptr;
    if (LODRenderData) {
        int32 NumVertices = LODRenderData->GetNumVertices();
        Positions.SetNumUninitialized(NumVertices);
        for (int32 i = 0; i < NumVertices; i++) {
            FVector3f VertexPos = LODRenderData->StaticVertexBuffers.PositionVertexBuffer.VertexPosition(i);
            Positions[i] = FVector(VertexPos.X, VertexPos.Y, VertexPos.Z);
        }
        if (bEnableDebugLogging) {
            UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Retrieved %d vertex positions for %s."), NumVertices, *Mesh->GetName());
        }
    }
    else {
        if (bEnableDebugLogging) {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: GetVertexPositions - No LODRenderData for %s."), *Mesh->GetName());
        }
    }
    return Positions;
}