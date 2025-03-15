#include "PBDSoftBodyComponent.h"
#include "PBDSoftBodyPlugin/Private/Simulation/ClusterManager.h"
#include "PBDSoftBodyPlugin/Private/Rendering/VertexBufferUpdater.h"
#include "PBDSoftBodyPlugin/Private/Animation/AnimationBlender.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "ProfilingDebugging/ScopedTimers.h"

UPBDSoftBodyComponent::UPBDSoftBodyComponent()
{
    SoftBodyBlendWeight = 0.5f;
    NumClusters = 10;
    bEnableDebugLogging = true;
    bVerboseDebugLogging = true;
    bHasActiveAnimation = false;
    bHasLoggedVertexCount = false;
    bHasLoggedBlending = false;
    bHasLoggedBlendingVerbose = false;
    bHasLoggedInvalidObjects = false;

    ClusterManager = nullptr;
    VertexBufferUpdater = nullptr;
    AnimationBlender = nullptr;

    PrimaryComponentTick.bCanEverTick = true;

    if (bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Constructor called."));
    }
}

UPBDSoftBodyComponent::~UPBDSoftBodyComponent()
{
    if (bEnableDebugLogging && bVerboseDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Destructor called for %s."), *GetNameSafe(GetOwner()));
    }
}

void UPBDSoftBodyComponent::InitializeConfig()
{
    FString ConfigPath = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("PBDSoftBodyPlugin/Config/PBDSoftBodyConfig.ini"));
    FString NormalizedConfigPath = FConfigCacheIni::NormalizeConfigIniPath(ConfigPath);

    if (bEnableDebugLogging && bVerboseDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Using normalized config path: %s"), *NormalizedConfigPath);
    }

    if (!GConfig)
    {
        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Error, TEXT("PBDSoftBodyComponent: GConfig unavailable. Using default values."));
        }
        return;
    }

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

    if (bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Config initialized - SoftBodyBlendWeight: %f, NumClusters: %d"), SoftBodyBlendWeight, NumClusters);
    }
}

void UPBDSoftBodyComponent::BeginPlay()
{
    Super::BeginPlay();
    InitializeConfig();

    if (!ClusterManager)
    {
        ClusterManager = NewObject<UClusterManager>(this, NAME_None, RF_NoFlags, nullptr, true);
        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: ClusterManager created: %s"), ClusterManager ? TEXT("Success") : TEXT("Failed"));
        }
    }
    if (!VertexBufferUpdater)
    {
        VertexBufferUpdater = NewObject<UVertexBufferUpdater>(this, NAME_None, RF_NoFlags, nullptr, true);
        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: VertexBufferUpdater created: %s"), VertexBufferUpdater ? TEXT("Success") : TEXT("Failed"));
        }
    }
    if (!AnimationBlender)
    {
        AnimationBlender = NewObject<UAnimationBlender>(this, NAME_None, RF_NoFlags, nullptr, true);
        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: AnimationBlender created: %s"), AnimationBlender ? TEXT("Success") : TEXT("Failed"));
        }
    }

    if (bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: BeginPlay called for %s."), *GetOwner()->GetName());
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

    static int32 TickCount = 0;
    if (bEnableDebugLogging && (TickCount++ % 60 == 0))
    {
        UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: TickComponent called for %s with DeltaTime: %.3f"), *GetOwner()->GetName(), DeltaTime);
    }

    if (!IsValid(GetOwner()))
    {
        if (bEnableDebugLogging && !bHasLoggedInvalidObjects)
        {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: Owner is invalid in Tick."));
            bHasLoggedInvalidObjects = true;
        }
        return;
    }

    if (Velocities.Num() == 0 || SimulatedPositions.Num() == 0)
    {
        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Retrying initialization in Tick for %s."), *GetOwner()->GetName());
        }
        if (!InitializeSimulationData())
        {
            if (bEnableDebugLogging && !bHasLoggedInvalidObjects)
            {
                UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: Initialization still failed in Tick for %s."), *GetOwner()->GetName());
                bHasLoggedInvalidObjects = true;
            }
            return;
        }
    }

    if (!IsValid(AnimationBlender) || !IsValid(VertexBufferUpdater))
    {
        if (bEnableDebugLogging && !bHasLoggedInvalidObjects)
        {
            UE_LOG(LogTemp, Error, TEXT("PBDSoftBodyComponent: Invalid objects for %s - AnimationBlender: %s, VertexBufferUpdater: %s"),
                *GetOwner()->GetName(),
                IsValid(AnimationBlender) ? TEXT("Valid") : TEXT("Invalid"),
                IsValid(VertexBufferUpdater) ? TEXT("Valid") : TEXT("Invalid"));
            bHasLoggedInvalidObjects = true;
        }
        return;
    }

    bHasLoggedInvalidObjects = false;
    AnimationBlender->UpdateBlendedPositions(this);
    VertexBufferUpdater->ApplyPositions(this);

    if (bVerboseDebugLogging && (TickCount % 60 == 0)) // Throttle completion log
    {
        UE_LOG(LogTemp, Log, TEXT("PBDSoftBodyComponent: Tick completed for %s with DeltaTime: %.3f"), *GetOwner()->GetName(), DeltaTime);
    }
}

bool UPBDSoftBodyComponent::InitializeSimulationData()
{
    USkeletalMesh* Mesh = GetSkeletalMeshAsset();
    if (!IsValid(Mesh))
    {
        if (bEnableDebugLogging && IsValid(GetOwner()))
        {
            UE_LOG(LogTemp, Warning, TEXT("PBDSoftBodyComponent: No valid SkeletalMesh assigned to %s."), *GetOwner()->GetName());
        }
        return false;
    }

    FSkeletalMeshRenderData* RenderData = Mesh->GetResourceForRendering();
    if (!RenderData || RenderData->LODRenderData.Num() == 0)
    {
        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Error, TEXT("PBDSoftBodyComponent: No RenderData or LODRenderData for %s."), *Mesh->GetName());
        }
        return false;
    }

    const FSkeletalMeshLODRenderData* LODRenderData = &RenderData->LODRenderData[0];
    int32 VertexCount = LODRenderData->GetNumVertices();
    if (VertexCount <= 0)
    {
        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Error, TEXT("PBDSoftBodyComponent: Invalid vertex count (%d) for %s."), VertexCount, *Mesh->GetName());
        }
        return false;
    }

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

    if (!IsValid(AnimationBlender))
    {
        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Error, TEXT("PBDSoftBodyComponent: AnimationBlender is invalid during initialization for %s."), *Mesh->GetName());
        }
        return false;
    }

    TArray<FVector> InitialPositions = AnimationBlender->GetVertexPositions(this);
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

    if (!IsValid(ClusterManager))
    {
        if (bEnableDebugLogging)
        {
            UE_LOG(LogTemp, Error, TEXT("PBDSoftBodyComponent: ClusterManager is invalid during initialization for %s."), *Mesh->GetName());
        }
        return false;
    }

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

    return true;
}