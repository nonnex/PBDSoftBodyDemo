#include "PBDSoftBodyComponent.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkinWeightVertexBuffer.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimInstance.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"

UPBDSoftBodyComponent::UPBDSoftBodyComponent()
{
    SoftBodyBlendWeight = 0.5f;
    NumClusters = 10;
    bEnableDebugLogging = true;
    bVerboseDebugLogging = false;
    bHasActiveAnimation = false;
    bHasLoggedVertexCount = false;
    bHasLoggedBlending = false;
    bHasLoggedBlendingVerbose = false;
}

void UPBDSoftBodyComponent::InitializeConfig()
{
    FString ConfigPath = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("PBDSoftBodyPlugin/Config/PBDSoftBodyConfig.ini"));
    FString NormalizedConfigPath = FConfigCacheIni::NormalizeConfigIniPath(ConfigPath);

    if (bEnableDebugLogging && bVerboseDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Using normalized config path: %s"), *NormalizedConfigPath);
    }

    if (GConfig)
    {
        if (!GConfig->GetFloat(TEXT("PBDSoftBody"), TEXT("SoftBodyBlendWeight"), SoftBodyBlendWeight, NormalizedConfigPath))
        {
            if (bEnableDebugLogging)
            {
                UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: Failed to load SoftBodyBlendWeight from %s. Using default: %f"), *NormalizedConfigPath, SoftBodyBlendWeight);
            }
        }

        if (!GConfig->GetInt(TEXT("PBDSoftBody"), TEXT("NumClusters"), NumClusters, NormalizedConfigPath))
        {
            if (bEnableDebugLogging)
            {
                UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: Failed to load NumClusters from %s. Using default: %d"), *NormalizedConfigPath, NumClusters);
            }
        }

        SoftBodyBlendWeight = FMath::Clamp(SoftBodyBlendWeight, 0.0f, 1.0f);
        NumClusters = FMath::Max(NumClusters, 1);
    }
    else
    {
        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Error, TEXT("PBDSoftBodyComponent: GConfig unavailable. Using default values for SoftBodyBlendWeight (%f) and NumClusters (%d)"), SoftBodyBlendWeight, NumClusters);
        }
    }
}

void UPBDSoftBodyComponent::BeginPlay()
{
    Super::BeginPlay();
    InitializeConfig();

    if (bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: BeginPlay called for %s."), *GetOwner()->GetName());
        UE_LOG(LogTemp, Log, TEXT("Loaded SoftBodyBlendWeight: %f"), SoftBodyBlendWeight);
        UE_LOG(LogTemp, Log, TEXT("Loaded NumClusters: %d"), NumClusters);
    }

    if (!InitializeSimulationData())
    {
        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: Initialization failed in BeginPlay for %s. Retrying in Tick."), *GetOwner()->GetName());
        }
    }
}

void UPBDSoftBodyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (Velocities.Num() == 0 || SimulatedPositions.Num() == 0)
    {
        if (bEnableDebugLogging && GetOwner())
        {
            UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Retrying initialization in Tick for %s."), *GetOwner()->GetName());
        }
        if (!InitializeSimulationData())
        {
            if (bEnableDebugLogging && GetOwner())
            {
                UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: Initialization still failed in Tick for %s."), *GetOwner()->GetName());
            }
            return;
        }
    }

    UpdateBlendedPositions();
}

bool UPBDSoftBodyComponent::InitializeSimulationData()
{
    USkeletalMesh* Mesh = GetSkeletalMeshAsset();
    if (!Mesh)
    {
        if (bEnableDebugLogging && GetOwner())
        {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: No SkeletalMesh assigned to %s."), *GetOwner()->GetName());
        }
        return false;
    }

    const FSkeletalMeshLODRenderData* LODRenderData = Mesh->GetResourceForRendering()->LODRenderData.Num() > 0
        ? &Mesh->GetResourceForRendering()->LODRenderData[0]
        : nullptr;
    if (!LODRenderData)
    {
        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Error, TEXT("PBDSoftBodyComponent: No LODRenderData available for %s."), *Mesh->GetName());
        }
        return false;
    }
    int32 VertexCount = LODRenderData->GetNumVertices();

    if (bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Initializing simulation data for %s with %d vertices."), *Mesh->GetName(), VertexCount);
    }

    Velocities.Reset();
    SimulatedPositions.Reset();
    Clusters.Reset();

    TArray<FVector> InitialPositions = GetVertexPositions();
    if (InitialPositions.Num() != VertexCount)
    {
        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Error, TEXT("PBDSoftBodyComponent: Vertex position retrieval failed for %s. Expected %d, got %d."),
                *Mesh->GetName(), VertexCount, InitialPositions.Num());
        }
        return false;
    }

    Velocities.SetNum(VertexCount, EAllowShrinking::No);
    SimulatedPositions.SetNum(VertexCount, EAllowShrinking::No);

    for (int32 i = 0; i < VertexCount; i++)
    {
        Velocities[i] = FVector::ZeroVector;
        SimulatedPositions[i] = InitialPositions[i];
    }

    GenerateClusters(InitialPositions);

    if (bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Successfully initialized %d vertices and %d clusters for %s."),
            VertexCount, Clusters.Num(), *Mesh->GetName());
    }
    return true;
}

TArray<FVector> UPBDSoftBodyComponent::GetVertexPositions() const
{
    TArray<FVector> Positions;
    USkeletalMesh* Mesh = GetSkeletalMeshAsset();
    if (!Mesh)
    {
        if (bEnableDebugLogging && GetOwner())
        {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: GetVertexPositions - No SkeletalMesh assigned to %s."), *GetOwner()->GetName());
        }
        return Positions;
    }

    int32 LODIndex = GetPredictedLODLevel();
    if (bVerboseDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Using LOD index %d for %s."), LODIndex, *Mesh->GetName());
    }

    const FSkeletalMeshLODRenderData* LODRenderData = Mesh->GetResourceForRendering()->LODRenderData.IsValidIndex(LODIndex)
        ? &Mesh->GetResourceForRendering()->LODRenderData[LODIndex]
        : nullptr;
    if (!LODRenderData)
    {
        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: GetVertexPositions - No LODRenderData for %s at LOD %d."), *Mesh->GetName(), LODIndex);
        }
        return Positions;
    }

    const FSkinWeightVertexBuffer* SkinWeightBuffer = &LODRenderData->SkinWeightVertexBuffer;
    if (!SkinWeightBuffer || SkinWeightBuffer->GetNumVertices() == 0)
    {
        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: GetVertexPositions - Invalid SkinWeightBuffer for %s."), *Mesh->GetName());
        }
        return Positions;
    }

    TArray<FTransform> BoneTransforms = GetComponentSpaceTransforms();
    if (bVerboseDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Retrieved %d bone transforms for %s."), BoneTransforms.Num(), *Mesh->GetName());
    }

    bool bCurrentHasAnimation = (GetAnimInstance() != nullptr && BoneTransforms.Num() > 0);
    if (bCurrentHasAnimation != bHasActiveAnimation)
    {
        bHasActiveAnimation = bCurrentHasAnimation;
        if (bEnableDebugLogging)
        {
            if (!bHasActiveAnimation)
            {
                UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: GetVertexPositions - No active animation for %s. Falling back to reference pose."), *Mesh->GetName());
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: GetVertexPositions - Active animation detected for %s."), *Mesh->GetName());
            }
        }
    }

    if (!bHasActiveAnimation)
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
            if (bEnableDebugLogging && !bHasLoggedVertexCount)
            {
                UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Retrieved %d reference pose vertex positions for %s."), Positions.Num(), *Mesh->GetName());
                bHasLoggedVertexCount = true;
            }
        }
        else
        {
            if (bEnableDebugLogging)
            {
                UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: Failed to retrieve reference pose positions for %s. Vertex count mismatch."), *Mesh->GetName());
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
        const_cast<UPBDSoftBodyComponent*>(this),
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
        if (bEnableDebugLogging && !bHasLoggedVertexCount)
        {
            UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Retrieved %d skinned vertex positions for %s."), Positions.Num(), *Mesh->GetName());
            bHasLoggedVertexCount = true;
        }
    }
    else
    {
        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: Failed to compute skinned positions for %s."), *Mesh->GetName());
        }
    }
    return Positions;
}

void UPBDSoftBodyComponent::GenerateClusters(const TArray<FVector>& VertexPositions)
{
    if (VertexPositions.Num() == 0 || NumClusters <= 0)
    {
        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: GenerateClusters - Invalid input: %d vertices, %d clusters."), VertexPositions.Num(), NumClusters);
        }
        return;
    }

    int32 VerticesPerCluster = VertexPositions.Num() / NumClusters;
    Clusters.SetNum(NumClusters, EAllowShrinking::No);

    if (bVerboseDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Generating %d clusters with ~%d vertices each."), NumClusters, VerticesPerCluster);
    }

    for (int32 ClusterIdx = 0; ClusterIdx < NumClusters; ClusterIdx++)
    {
        FSoftBodyCluster& Cluster = Clusters[ClusterIdx];
        Cluster.VertexIndices.Reset();
        Cluster.VertexOffsets.Reset();

        int32 StartIdx = ClusterIdx * VerticesPerCluster;
        int32 EndIdx = (ClusterIdx == NumClusters - 1) ? VertexPositions.Num() : (ClusterIdx + 1) * VerticesPerCluster;

        for (int32 i = StartIdx; i < EndIdx; i++)
        {
            Cluster.VertexIndices.Add(i);
        }

        FVector Centroid = FVector::ZeroVector;
        for (int32 VertexIdx : Cluster.VertexIndices)
        {
            Centroid += VertexPositions[VertexIdx];
        }
        Centroid /= Cluster.VertexIndices.Num();
        Cluster.CentroidPosition = Centroid;
        Cluster.CentroidVelocity = FVector::ZeroVector;

        Cluster.VertexOffsets.SetNum(Cluster.VertexIndices.Num(), EAllowShrinking::No);
        for (int32 i = 0; i < Cluster.VertexIndices.Num(); i++)
        {
            Cluster.VertexOffsets[i] = VertexPositions[Cluster.VertexIndices[i]] - Centroid;
        }

        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Cluster %d created with %d vertices."), ClusterIdx, Cluster.VertexIndices.Num());
        }
        if (bVerboseDebugLogging)
        {
            UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Cluster %d centroid at (%.2f, %.2f, %.2f)."),
                ClusterIdx, Centroid.X, Centroid.Y, Centroid.Z);
        }
    }
}

void UPBDSoftBodyComponent::UpdateBlendedPositions()
{
    if (Velocities.Num() == 0 || SimulatedPositions.Num() == 0 || Clusters.Num() == 0)
    {
        if (bEnableDebugLogging && GetOwner())
        {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: UpdateBlendedPositions - Simulation data not initialized for %s."), *GetOwner()->GetName());
        }
        return;
    }

    TArray<FVector> AnimatedPositions = GetVertexPositions();
    if (AnimatedPositions.Num() != SimulatedPositions.Num())
    {
        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: Mismatch in vertex counts - Animated: %d, Simulated: %d."),
                AnimatedPositions.Num(), SimulatedPositions.Num());
        }
        return;
    }

    for (FSoftBodyCluster& Cluster : Clusters)
    {
        FVector AnimatedCentroid = FVector::ZeroVector;
        for (int32 VertexIdx : Cluster.VertexIndices)
        {
            AnimatedCentroid += AnimatedPositions[VertexIdx];
        }
        AnimatedCentroid /= Cluster.VertexIndices.Num();
        if (bVerboseDebugLogging)
        {
            UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Cluster animated centroid at (%.2f, %.2f, %.2f)."),
                AnimatedCentroid.X, AnimatedCentroid.Y, AnimatedCentroid.Z);
        }
        Cluster.CentroidPosition = FMath::Lerp(AnimatedCentroid, Cluster.CentroidPosition, SoftBodyBlendWeight);
    }

    for (const FSoftBodyCluster& Cluster : Clusters)
    {
        for (int32 i = 0; i < Cluster.VertexIndices.Num(); i++)
        {
            int32 VertexIdx = Cluster.VertexIndices[i];
            SimulatedPositions[VertexIdx] = Cluster.CentroidPosition + Cluster.VertexOffsets[i];
        }
    }

    USkeletalMesh* Mesh = GetSkeletalMeshAsset();
    if (!Mesh || !Mesh->GetResourceForRendering())
    {
        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: Failed to get skeletal mesh or rendering resource for %s."), *GetOwner()->GetName());
        }
        return;
    }

    FSkeletalMeshLODRenderData* LODRenderData = Mesh->GetResourceForRendering()->LODRenderData.IsValidIndex(0)
        ? &Mesh->GetResourceForRendering()->LODRenderData[0]
        : nullptr;
    if (!LODRenderData)
    {
        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: No LODRenderData for applying positions in %s."), *Mesh->GetName());
        }
        return;
    }

    FPositionVertexBuffer& PositionBuffer = LODRenderData->StaticVertexBuffers.PositionVertexBuffer;
    if (PositionBuffer.GetNumVertices() != SimulatedPositions.Num())
    {
        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: Vertex count mismatch when applying positions. Buffer: %d, Simulated: %d."),
                PositionBuffer.GetNumVertices(), SimulatedPositions.Num());
        }
        return;
    }

    ENQUEUE_RENDER_COMMAND(UpdateSoftBodyPositions)(
        [this, &PositionBuffer](FRHICommandListImmediate& RHICmdList)
        {
            FBufferRHIRef& VertexBuffer = PositionBuffer.VertexBufferRHI;
            if (!VertexBuffer.IsValid())
            {
                if (bEnableDebugLogging)
                {
                    UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: Vertex buffer not valid for %s."), *GetOwner()->GetName());
                }
                return;
            }

            void* VertexData = RHICmdList.LockBuffer(VertexBuffer.GetReference(), 0, PositionBuffer.GetNumVertices() * sizeof(FVector3f), RLM_WriteOnly);
            if (VertexData)
            {
                FVector3f* Positions = static_cast<FVector3f*>(VertexData);
                for (int32 i = 0; i < SimulatedPositions.Num(); i++)
                {
                    Positions[i] = FVector3f(SimulatedPositions[i].X, SimulatedPositions[i].Y, SimulatedPositions[i].Z);
                }
                RHICmdList.UnlockBuffer(VertexBuffer.GetReference());

                if (bEnableDebugLogging && !bHasLoggedBlending && GetSkeletalMeshAsset()->GetName().Contains(TEXT("SKM_Quinn")))
                {
                    UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Successfully applied %d simulated positions to SKM_Quinn."), SimulatedPositions.Num());
                }
            }
            else
            {
                if (bEnableDebugLogging)
                {
                    UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: Failed to lock vertex buffer for %s."), *GetOwner()->GetName());
                }
            }
        });

    MarkRenderStateDirty();

    if (bEnableDebugLogging && !bHasLoggedBlending)
    {
        UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Blended %d vertices across %d clusters with weight %.2f for %s."),
            SimulatedPositions.Num(), Clusters.Num(), SoftBodyBlendWeight, *GetOwner()->GetName());
        bHasLoggedBlending = true;
    }
    if (bVerboseDebugLogging && !bHasLoggedBlendingVerbose)
    {
        UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: First vertex position after blending: (%.2f, %.2f, %.2f)."),
            SimulatedPositions[0].X, SimulatedPositions[0].Y, SimulatedPositions[0].Z);
        bHasLoggedBlendingVerbose = true;
    }
}