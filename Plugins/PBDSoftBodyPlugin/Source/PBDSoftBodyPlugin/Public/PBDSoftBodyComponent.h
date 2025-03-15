#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "SoftBodyCluster.h"
#include "PBDSoftBodyComponent.generated.h"

class UClusterManager;
class UVertexBufferUpdater;
class UAnimationBlender;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PBDSOFTBODYPLUGIN_API UPBDSoftBodyComponent : public USkeletalMeshComponent
{
    GENERATED_BODY()

public:
    UPBDSoftBodyComponent();

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
    UPROPERTY(Transient)
    UClusterManager* ClusterManager;

    UPROPERTY(Transient)
    UVertexBufferUpdater* VertexBufferUpdater;

    UPROPERTY(Transient)
    UAnimationBlender* AnimationBlender;

    bool bHasActiveAnimation;
    mutable bool bHasLoggedVertexCount;
    bool bHasLoggedBlending;
    bool bHasLoggedBlendingVerbose;
};