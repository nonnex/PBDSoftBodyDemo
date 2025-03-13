#include "PBDSoftBodyComponent.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkinWeightVertexBuffer.h"
#include "Animation/Skeleton.h"

void UPBDSoftBodyComponent::BeginPlay() {
    Super::BeginPlay();
    if (bEnableDebugLogging) {
        UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: BeginPlay called for %s."), *GetOwner()->GetName());
    }
    if (!InitializeSimulationData()) {
        if (bEnableDebugLogging) {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: Initialization failed in BeginPlay for %s. Retrying in Tick."), *GetOwner()->GetName());
        }
    }
}

void UPBDSoftBodyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (Velocities.Num() == 0 || SimulatedPositions.Num() == 0) {
        if (bEnableDebugLogging && GetOwner()) {
            UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Retrying initialization in Tick for %s."), *GetOwner()->GetName());
        }
        if (!InitializeSimulationData()) {
            if (bEnableDebugLogging && GetOwner()) {
                UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: Initialization still failed in Tick for %s."), *GetOwner()->GetName());
            }
            return;
        }
    }
    UpdateBlendedPositions();
}

bool UPBDSoftBodyComponent::InitializeSimulationData() {
    USkeletalMesh* Mesh = GetSkeletalMeshAsset();
    if (!Mesh) {
        if (bEnableDebugLogging && GetOwner()) {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: No SkeletalMesh assigned to %s."), *GetOwner()->GetName());
        }
        return false;
    }

    const FSkeletalMeshLODRenderData* LODRenderData = Mesh->GetResourceForRendering()->LODRenderData.Num() > 0
        ? &Mesh->GetResourceForRendering()->LODRenderData[0]
        : nullptr;
    if (!LODRenderData) {
        if (bEnableDebugLogging) {
            UE_LOG(LogTemp, Error, TEXT("PBDSoftBodyComponent: No LODRenderData available for %s."), *Mesh->GetName());
        }
        return false;
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
        return true;
    }
    else {
        if (bEnableDebugLogging) {
            UE_LOG(LogTemp, Error, TEXT("PBDSoftBodyComponent: Vertex position retrieval failed for %s. Expected %d, got %d."),
                *Mesh->GetName(), VertexCount, InitialPositions.Num());
        }
        return false;
    }
}

TArray<FVector> UPBDSoftBodyComponent::GetVertexPositions() const {
    TArray<FVector> Positions;
    USkeletalMesh* Mesh = GetSkeletalMeshAsset();
    if (!Mesh) {
        if (bEnableDebugLogging && GetOwner()) {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: GetVertexPositions - No SkeletalMesh assigned to %s."), *GetOwner()->GetName());
        }
        return Positions;
    }

    int32 LODIndex = GetPredictedLODLevel();
    const FSkeletalMeshLODRenderData* LODRenderData = Mesh->GetResourceForRendering()->LODRenderData.IsValidIndex(LODIndex)
        ? &Mesh->GetResourceForRendering()->LODRenderData[LODIndex]
        : nullptr;
    if (!LODRenderData) {
        if (bEnableDebugLogging) {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: GetVertexPositions - No LODRenderData for %s at LOD %d."), *Mesh->GetName(), LODIndex);
        }
        return Positions;
    }

    const FSkinWeightVertexBuffer* SkinWeightBuffer = &LODRenderData->SkinWeightVertexBuffer;
    if (!SkinWeightBuffer || SkinWeightBuffer->GetNumVertices() == 0) {
        if (bEnableDebugLogging) {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: GetVertexPositions - Invalid SkinWeightBuffer for %s."), *Mesh->GetName());
        }
        return Positions;
    }

    TArray<FTransform> BoneTransforms = GetComponentSpaceTransforms();
    if (BoneTransforms.Num() == 0) {
        if (bEnableDebugLogging) {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: GetVertexPositions - No bone transforms available for %s."), *Mesh->GetName());
        }
        return Positions;
    }

    TArray<FMatrix44f> RefToLocals;
    RefToLocals.SetNum(BoneTransforms.Num());
    for (int32 i = 0; i < BoneTransforms.Num(); i++) {
        RefToLocals[i] = FMatrix44f(BoneTransforms[i].ToMatrixWithScale());
    }

    TArray<FVector3f> SkinnedPositions;
    USkeletalMeshComponent::ComputeSkinnedPositions(
        const_cast<UPBDSoftBodyComponent*>(this),
        SkinnedPositions,
        RefToLocals,
        *LODRenderData,
        *SkinWeightBuffer
    );

    if (SkinnedPositions.Num() > 0) {
        Positions.SetNum(SkinnedPositions.Num(), EAllowShrinking::No);
        for (int32 i = 0; i < SkinnedPositions.Num(); i++) {
            Positions[i] = FVector(SkinnedPositions[i].X, SkinnedPositions[i].Y, SkinnedPositions[i].Z);
        }
        if (bEnableDebugLogging) {
            UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Retrieved %d skinned vertex positions for %s."), Positions.Num(), *Mesh->GetName());
        }
    }
    else {
        if (bEnableDebugLogging) {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: Failed to compute skinned positions for %s."), *Mesh->GetName());
        }
    }
    return Positions;
}

void UPBDSoftBodyComponent::UpdateBlendedPositions() {
    if (Velocities.Num() == 0 || SimulatedPositions.Num() == 0) {
        if (bEnableDebugLogging && GetOwner()) {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: UpdateBlendedPositions - Simulation data not initialized for %s."), *GetOwner()->GetName());
        }
        return;
    }

    TArray<FVector> AnimatedPositions = GetVertexPositions();
    if (AnimatedPositions.Num() != SimulatedPositions.Num()) {
        if (bEnableDebugLogging) {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: Mismatch in vertex counts - Animated: %d, Simulated: %d."),
                AnimatedPositions.Num(), SimulatedPositions.Num());
        }
        return;
    }

    for (int32 i = 0; i < AnimatedPositions.Num(); i++) {
        SimulatedPositions[i] = FMath::Lerp(AnimatedPositions[i], SimulatedPositions[i], SoftBodyBlendWeight);
    }

    if (bEnableDebugLogging) {
        UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Blended %d vertices with weight %.2f for %s."),
            SimulatedPositions.Num(), SoftBodyBlendWeight, *GetOwner()->GetName());
    }

    // TODO: Apply blended positions to the mesh (deferred to Task 4)
}