#pragma once

#include "CoreMinimal.h"
#include "SoftBodyCluster.h"
#include "Components/SkeletalMeshComponent.h"
#include "PBDSoftBodyComponent.generated.h"

// Forward declarations to avoid circular dependencies
class UClusterManager;
class UVertexBufferUpdater;
class UAnimationBlender;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PBDSOFTBODYPLUGIN_API UPBDSoftBodyComponent : public USkeletalMeshComponent
{
    GENERATED_BODY()

public:
    UPBDSoftBodyComponent();
    virtual ~UPBDSoftBodyComponent() override;

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "PBD Soft Body")
    void InitializeConfig();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PBD Soft Body")
    float SoftBodyBlendWeight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PBD Soft Body")
    int32 NumClusters;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PBD Soft Body")
    TArray<FVector> Velocities;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PBD Soft Body")
    TArray<FVector> SimulatedPositions;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PBD Soft Body")
    TArray<FSoftBodyCluster> Clusters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PBD Soft Body")
    bool bEnableDebugLogging;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PBD Soft Body")
    bool bVerboseDebugLogging;

protected:
    bool InitializeSimulationData();

private:
    UPROPERTY(Instanced, Transient)
    UClusterManager* ClusterManager;

    UPROPERTY(Instanced, Transient)
    UVertexBufferUpdater* VertexBufferUpdater;

    UPROPERTY(Instanced, Transient)
    UAnimationBlender* AnimationBlender;

    bool bHasActiveAnimation;
    mutable bool bHasLoggedVertexCount;
    bool bHasLoggedBlending;
    bool bHasLoggedBlendingVerbose;
    bool bHasLoggedInvalidObjects; // New flag to throttle logging

    friend class UAnimationBlender;
    friend class UVertexBufferUpdater;
};