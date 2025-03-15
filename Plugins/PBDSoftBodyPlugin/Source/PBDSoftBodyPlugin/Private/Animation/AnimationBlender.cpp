#include "AnimationBlender.h"
#include "PBDSoftBodyComponent.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkinWeightVertexBuffer.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimInstance.h"
#include "SoftBodyCluster.h"

TArray<FVector> UAnimationBlender::GetVertexPositions(UPBDSoftBodyComponent* Component) const
{
    TArray<FVector> Positions;
    if (!Component)
    {
        return Positions;
    }

    USkeletalMesh* Mesh = Component->GetSkeletalMeshAsset();
    if (!Mesh)
    {
        if (Component->bEnableDebugLogging && Component->GetOwner())
        {
            UE_LOG(LogTemp, Warning, TEXT("AnimationBlender: GetVertexPositions - No SkeletalMesh assigned to %s."), *Component->GetOwner()->GetName());
        }
        return Positions;
    }

    int32 LODIndex = Component->GetPredictedLODLevel();
    if (Component->bVerboseDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("AnimationBlender: Using LOD index %d for %s."), LODIndex, *Mesh->GetName());
    }

    const FSkeletalMeshLODRenderData* LODRenderData = Mesh->GetResourceForRendering()->LODRenderData.IsValidIndex(LODIndex)
        ? &Mesh->GetResourceForRendering()->LODRenderData[LODIndex]
        : nullptr;
    if (!LODRenderData)
    {
        if (Component->bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Warning, TEXT("AnimationBlender: GetVertexPositions - No LODRenderData for %s at LOD %d."), *Mesh->GetName(), LODIndex);
        }
        return Positions;
    }

    const FSkinWeightVertexBuffer* SkinWeightBuffer = &LODRenderData->SkinWeightVertexBuffer;
    if (!SkinWeightBuffer || SkinWeightBuffer->GetNumVertices() == 0)
    {
        if (Component->bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Warning, TEXT("AnimationBlender: GetVertexPositions - Invalid SkinWeightBuffer for %s."), *Mesh->GetName());
        }
        return Positions;
    }

    TArray<FTransform> BoneTransforms = Component->GetComponentSpaceTransforms();
    if (Component->bVerboseDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("AnimationBlender: Retrieved %d bone transforms for %s."), BoneTransforms.Num(), *Mesh->GetName());
    }

    bool bCurrentHasAnimation = (Component->GetAnimInstance() != nullptr && BoneTransforms.Num() > 0);
    if (bCurrentHasAnimation != Component->bHasActiveAnimation)
    {
        Component->bHasActiveAnimation = bCurrentHasAnimation;
        if (Component->bEnableDebugLogging)
        {
            if (!bCurrentHasAnimation)
            {
                UE_LOG(LogTemp, Warning, TEXT("AnimationBlender: GetVertexPositions - No active animation for %s. Falling back to reference pose."), *Mesh->GetName());
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("AnimationBlender: GetVertexPositions - Active animation detected for %s."), *Mesh->GetName());
            }
        }
    }

    if (!bCurrentHasAnimation)
    {
        const FPositionVertexBuffer& PositionBuffer = LODRenderData->StaticVertexBuffers.PositionVertexBuffer;
        int32 NumVertices = PositionBuffer.GetNumVertices();
        if (NumVertices == SkinWeightBuffer->GetNumVertices())
        {
            Positions.SetNum(NumVertices, EAllowShrinking::No);
            for (int32 i = 0; i < NumVertices; i++)
            {
                Positions[i] = FVector(PositionBuffer.VertexPosition(i));
            }
            if (Component->bEnableDebugLogging && !Component->bHasLoggedVertexCount)
            {
                UE_LOG(LogTemp, Log, TEXT("AnimationBlender: Retrieved %d reference pose vertex positions for %s."), Positions.Num(), *Mesh->GetName());
                Component->bHasLoggedVertexCount = true;
            }
        }
        else
        {
            if (Component->bEnableDebugLogging)
            {
                UE_LOG(LogTemp, Warning, TEXT("AnimationBlender: Failed to retrieve reference pose positions for %s. Vertex count mismatch."), *Mesh->GetName());
            }
            return Positions;
        }
        return Positions;
    }

    TArray<FMatrix44f> RefToLocals;
    RefToLocals.SetNum(BoneTransforms.Num());
    for (int32 i = 0; i < BoneTransforms.Num(); i++)
    {
        RefToLocals[i] = FMatrix44f(BoneTransforms[i].ToMatrixWithScale());
    }

    TArray<FVector3f> SkinnedPositions;
    USkeletalMeshComponent::ComputeSkinnedPositions(
        Component,
        SkinnedPositions,
        RefToLocals,
        *LODRenderData,
        *SkinWeightBuffer
    );

    if (SkinnedPositions.Num() > 0)
    {
        Positions.SetNum(SkinnedPositions.Num(), EAllowShrinking::No);
        for (int32 i = 0; i < SkinnedPositions.Num(); i++)
        {
            Positions[i] = FVector(SkinnedPositions[i].X, SkinnedPositions[i].Y, SkinnedPositions[i].Z);
        }
        if (Component->bEnableDebugLogging && !Component->bHasLoggedVertexCount)
        {
            UE_LOG(LogTemp, Log, TEXT("AnimationBlender: Retrieved %d skinned vertex positions for %s."), Positions.Num(), *Mesh->GetName());
            Component->bHasLoggedVertexCount = true;
        }
    }
    else
    {
        if (Component->bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Warning, TEXT("AnimationBlender: Failed to compute skinned positions for %s."), *Mesh->GetName());
        }
    }
    return Positions;
}

void UAnimationBlender::UpdateBlendedPositions(UPBDSoftBodyComponent* Component)
{
    if (!Component || Component->Velocities.Num() == 0 || Component->SimulatedPositions.Num() == 0 || Component->Clusters.Num() == 0)
    {
        if (Component && Component->bEnableDebugLogging && Component->GetOwner())
        {
            UE_LOG(LogTemp, Warning, TEXT("AnimationBlender: UpdateBlendedPositions - Simulation data not initialized for %s."), *Component->GetOwner()->GetName());
        }
        return;
    }

    TArray<FVector> AnimatedPositions = GetVertexPositions(Component);
    if (AnimatedPositions.Num() != Component->SimulatedPositions.Num())
    {
        if (Component->bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Warning, TEXT("AnimationBlender: Mismatch in vertex counts - Animated: %d, Simulated: %d."),
                AnimatedPositions.Num(), Component->SimulatedPositions.Num());
        }
        return;
    }

    static int32 FrameCount = 0;
    FrameCount++;

    for (FSoftBodyCluster& Cluster : Component->Clusters)
    {
        FVector AnimatedCentroid = FVector::ZeroVector;
        for (int32 VertexIdx : Cluster.VertexIndices)
        {
            AnimatedCentroid += AnimatedPositions[VertexIdx];
        }
        if (Cluster.VertexIndices.Num() > 0)
        {
            AnimatedCentroid /= Cluster.VertexIndices.Num();
        }
        if (Component->bVerboseDebugLogging && (FrameCount % 60 == 0))
        {
            UE_LOG(LogTemp, Log, TEXT("AnimationBlender: Cluster animated centroid at (%.2f, %.2f, %.2f)."),
                AnimatedCentroid.X, AnimatedCentroid.Y, AnimatedCentroid.Z);
        }
        Cluster.CentroidPosition = FMath::Lerp(AnimatedCentroid, Cluster.CentroidPosition, Component->SoftBodyBlendWeight);
    }

    for (const FSoftBodyCluster& Cluster : Component->Clusters)
    {
        for (int32 i = 0; i < Cluster.VertexIndices.Num(); i++)
        {
            int32 VertexIdx = Cluster.VertexIndices[i];
            Component->SimulatedPositions[VertexIdx] = Cluster.CentroidPosition + Cluster.VertexOffsets[i];
        }
    }

    if (Component->bEnableDebugLogging && !Component->bHasLoggedBlending)
    {
        UE_LOG(LogTemp, Log, TEXT("AnimationBlender: Blended %d vertices across %d clusters with weight %.2f for %s."),
            Component->SimulatedPositions.Num(), Component->Clusters.Num(), Component->SoftBodyBlendWeight, *Component->GetOwner()->GetName());
        Component->bHasLoggedBlending = true;
    }
    if (Component->bVerboseDebugLogging && (FrameCount % 60 == 0))
    {
        UE_LOG(LogTemp, Log, TEXT("AnimationBlender: First vertex position after blending: (%.2f, %.2f, %.2f)."),
            Component->SimulatedPositions[0].X, Component->SimulatedPositions[0].Y, Component->SimulatedPositions[0].Z);
    }
}