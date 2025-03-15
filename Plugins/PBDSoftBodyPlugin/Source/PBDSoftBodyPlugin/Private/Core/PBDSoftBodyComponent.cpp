#include "PBDSoftBodyComponent.h"
#include "Simulation/ClusterManager.h"
#include "Rendering/VertexBufferUpdater.h"
#include "Animation/AnimationBlender.h"
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

    ClusterManager = nullptr;
    VertexBufferUpdater = nullptr;
    AnimationBlender = nullptr;
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

    ClusterManager = NewObject<UClusterManager>(this);
    VertexBufferUpdater = NewObject<UVertexBufferUpdater>(this);
    AnimationBlender = NewObject<UAnimationBlender>(this);

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

    if (AnimationBlender && VertexBufferUpdater)
    {
        AnimationBlender->UpdateBlendedPositions(this);
        VertexBufferUpdater->ApplyPositions(this);
    }
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

    const int32 MinClusters = 1;
    const int32 MaxClusters = 100;
    NumClusters = FMath::Clamp(VertexCount / 1000, MinClusters, MaxClusters);
    if (bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Initializing simulation data for %s with %d vertices. Calculated NumClusters: %d."),
            *Mesh->GetName(), VertexCount, NumClusters);
    }

    Velocities.Reset();
    SimulatedPositions.Reset();
    Clusters.Reset();

    TArray<FVector> InitialPositions = AnimationBlender ? AnimationBlender->GetVertexPositions(this) : TArray<FVector>();
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

    if (ClusterManager)
    {
        double ClusteringTimeMs = 0.0;
        {
            FScopedDurationTimer ClusteringTimer(ClusteringTimeMs);
            ClusterManager->GenerateClusters(this, InitialPositions);
        }
        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Clustering completed for %s in %.3f ms with %d clusters."),
                *Mesh->GetName(), ClusteringTimeMs, Clusters.Num());
            if (ClusteringTimeMs > 1.0 && VertexCount >= 450000)
            {
                UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: Clustering time %.3f ms exceeds 1 ms target for %d vertices."),
                    ClusteringTimeMs, VertexCount);
            }
        }

        if (Clusters.Num() == 0)
        {
            if (bEnableDebugLogging)
            {
                UE_LOG(LogTemp, Error, TEXT("PBDSoftBodyComponent: Cluster generation failed for %s."), *Mesh->GetName());
            }
            return false;
        }

        if (bEnableDebugLogging && bVerboseDebugLogging)
        {
            UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Scalability test - VertexCount: %d, NumClusters: %d, Clusters Generated: %d."),
                VertexCount, NumClusters, Clusters.Num());
        }
    }

    return true;
}